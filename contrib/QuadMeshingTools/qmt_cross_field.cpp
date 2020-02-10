// Gmsh - Copyright (C) 1997-2020 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// issues on https://gitlab.onelab.info/gmsh/gmsh/issues.
//
// Author: Maxence Reberol

#include "quad_meshing_tools.h"

#include "qmt_types.hpp"
#include "qmt_utils.hpp"
#include "qmt_linalg_solver.h"

#include <cfloat>
#include <map>
#include <unordered_map>
#include <queue>
#include <algorithm>

#include <string>
#include <sstream>
#include <iostream>

#include "gmsh.h"

/* - Shortcuts for loops */
#define F(_VAR,_NB) for(size_t _VAR = 0; _VAR < (size_t) _NB; ++_VAR)
#define FC(_VAR,_NB,_COND) for(size_t _VAR = 0; _VAR < (size_t) _NB; ++_VAR) if (_COND)

using std::vector;
using std::map;
using std::unordered_map;
using std::array;
using std::pair;
using gmsh::vectorpair;

constexpr double EPS = 1.e-14; /* to detect 0 */

namespace QMT_CF_Utils {
  using namespace QMT;
  using namespace QMT_Utils;
  /************************************/
  /* Formatting and Logging functions */
  template <typename... Args>
    void error(const char* format, const Args & ... args) {
      std::ostringstream stream;
      sformat(stream, format, args...);
      gmsh::logger::write("QMT | Cross Field | " + stream.str(), "error");
    }

  template <typename... Args>
    void warn(const char* format, const Args & ... args) {
      std::ostringstream stream;
      sformat(stream, format, args...);
      gmsh::logger::write("QMT | Cross Field | " + stream.str(), "warning");
    }

  template <typename... Args>
    void info(const char* format, const Args & ... args) {
      std::ostringstream stream;
      sformat(stream, format, args...);
      gmsh::logger::write("QMT | Cross Field | " + stream.str(), "info");
    }
  /************************************/

  inline id2 sorted(id v1, id v2) { if (v1 < v2) { return {v1,v2}; } else { return {v2,v1}; } }

  struct id2Hash {
    size_t operator()(id2 p) const noexcept {
      return size_t(p[0]) << 32 | p[1];
    }
  };

}


namespace QMT {
  using namespace QMT_Utils;
  using namespace QMT_CF_Utils;

  constexpr bool DBG_VERBOSE = false;

  bool compute_triangle_adjacencies(
      const vector<id3>& triangles,
      vector<sid3>& triangle_neighbors,
      vector<vector<id>>& nm_triangle_neighbors,
      vector<id2>& uIEdges,
      vector<id>& old2IEdge,
      vector<vector<id>>& uIEdgeToOld
      ) {
    triangle_neighbors.clear();
    triangle_neighbors.resize(triangles.size(),{NO_ID,NO_ID,NO_ID});
    nm_triangle_neighbors.clear();

    constexpr size_t NBF = 3;

    /* Store element 'faces', with duplicates for further 'equality test' */
    std::vector<id2> faces;
    for (size_t i = 0; i < triangles.size(); ++i) {
      for (size_t lf = 0; lf < NBF; ++lf) {
        id2 face = {triangles[i][lf],triangles[i][(lf+1)%NBF]};
        std::sort(face.begin(),face.end());
        faces.push_back(face);
      }
    }

    /* Reduce 'duplicated faces' to 'unique faces', keeping the 'old2new' mapping */
    size_t nbUniques = sort_unique_with_perm(faces, uIEdges, old2IEdge);

    /* Build the 'unique face to elements' mapping */
    uIEdgeToOld.resize(nbUniques);
    for (size_t i = 0; i < triangles.size(); ++i) {
      for (size_t lf = 0; lf < NBF; ++lf) {
        id facePos = NBF*i+lf;
        uIEdgeToOld[old2IEdge[facePos]].push_back(facePos);
      }
    }

    /* Loop over 'unique face to elements' and set the element adjacencies */
    constexpr id2 NO_FACE = {NO_ID,NO_ID};
    for (size_t i = 0; i < nbUniques; ++i) {
      if (uIEdges[i] == NO_FACE) continue;
      if(uIEdges[i][0] == NO_ID) return false;
      if(uIEdges[i][1] == NO_ID) return false;
      if (uIEdgeToOld[i].size() == 1) { /* boundary */
        id eltId = uIEdgeToOld[i][0] / NBF;
        id lf = uIEdgeToOld[i][0] % NBF;
        triangle_neighbors[eltId][lf] = NO_ID;
      } else if (uIEdgeToOld[i].size() == 2) { /* regular face */
        id e1 = uIEdgeToOld[i][0] / NBF;
        id lf1 = uIEdgeToOld[i][0] % NBF;
        id e2 = uIEdgeToOld[i][1] / NBF;
        id lf2 = uIEdgeToOld[i][1] % NBF;
        triangle_neighbors[e1][lf1] = NBF*e2+lf2;
        triangle_neighbors[e2][lf2] = NBF*e1+lf1;
      } else if (uIEdgeToOld[i].size() > 2) { /* non manifold face */
        for (size_t j = 0; j < uIEdgeToOld[i].size(); ++j) {
          id e = uIEdgeToOld[i][j] / NBF;
          id lf = uIEdgeToOld[i][j] % NBF;
          std::vector<id> neighs;
          for (size_t k = 0; k < uIEdgeToOld[i].size(); ++k) {
            if (uIEdgeToOld[i][k] != uIEdgeToOld[i][j]) {
              neighs.push_back(uIEdgeToOld[i][k]);
            }
          }
          neighs.shrink_to_fit();
          id pos = nm_triangle_neighbors.size();
          nm_triangle_neighbors.push_back(neighs);
          triangle_neighbors[e][lf] = -((sid) pos + 1);
        }
      }
    }

    /* Reduce memory consumption */
    triangle_neighbors.shrink_to_fit();
    nm_triangle_neighbors.shrink_to_fit();

    return true;
  }

  bool import_TMesh_from_gmsh(const std::string& meshName, TMesh& M) {
    // TODO: meshName not used
    
    { /* vertices */
      std::vector<std::size_t> nodeTags;
      std::vector<double> coord;
      std::vector<double> parametricCoords;
      gmsh::model::mesh::getNodes(nodeTags, coord, parametricCoords);
      M.points.resize(nodeTags.size());
      F(i,nodeTags.size()) {
        id v = nodeTags[i];
        if (v >= M.points.size()) M.points.resize(v+1);
        M.points[v] = {coord[3*i+0],coord[3*i+1],coord[3*i+2]};
      }
    }
    { /* elements */
      std::vector<int> elementTypes;
      std::vector<std::vector<size_t>> elementTags;
      std::vector<std::vector<size_t>> nodeTags;
      gmsh::model::mesh::getElements(elementTypes,elementTags,nodeTags,-1,-1);
      F(i,elementTypes.size()) {
        if (elementTypes[i] == 1) { /* lines */
          F(j,elementTags[i].size()) {
            M.lines.push_back({id(nodeTags[i][2*j+0]),id(nodeTags[i][2*j+1])});
          }
        } else if (elementTypes[i] == 2) { /* triangles */
          F(j,elementTags[i].size()) {
            M.triangles.push_back({id(nodeTags[i][3*j+0]),id(nodeTags[i][3*j+1]),id(nodeTags[i][3*j+2])});
          }
        }
      }
    }

    return true;
  }

  inline double triangle_area(const TMesh& M, id t) {
    vec3 N = cross(M.points[M.triangles[t][2]]-M.points[M.triangles[t][0]],
        M.points[M.triangles[t][1]]-M.points[M.triangles[t][0]]);
    return length(N) / 2.;
  }

  inline vec3 triangle_normal(const TMesh& M, id t) {
    vec3 N = cross(M.points[M.triangles[t][2]]-M.points[M.triangles[t][0]],
        M.points[M.triangles[t][1]]-M.points[M.triangles[t][0]]);
    if (length(N) < EPS) {
      error("triangle {}: normal too small, length = {}", t, length(N));
      return {DBL_MAX,DBL_MAX,DBL_MAX};
    }
    N = normalize(N);
    return N;
  }

  struct IJV {
    id2 ij;
    double val;
    bool operator<(const IJV& b) const { return ij[0] < b.ij[0] || (ij[0] == b.ij[0] && ij[1] < b.ij[1]); }
  };

  struct IV {
    id i;
    double val;
    bool operator<(const IV& b) const { return i < b.i; }
  };

  /* two unknowns (x_2i, x_2i+1) for each edge */
  bool stiffness_coefficient(const TMesh& M, 
      id e, 
      const vector<id2>& uIEdges,
      const vector<id>& old2IEdge,
      const vector<vector<id>>& uIEdgeToOld,
      vector<IV>& iv,
      vector<IJV>& ijv) {
    if (uIEdgeToOld[e].size() != 2) {
      error("assembly, edge {}: uIEdgeToOld[e] = {}", e, uIEdgeToOld[e]);
      return false;
    }
    /* edge vertices */
    id v1 = uIEdges[e][0];
    id v2 = uIEdges[e][1];
    vec3 e_x = M.points[v2] - M.points[v1];
    if (DBG_VERBOSE) {info("-"); info("stiffness, e={} ({}->{}), p1={}, p2={}",e, v1,v2, M.points[v1], M.points[v2]);}
    double lenr = length(e_x);
    if (lenr < EPS) {
      error("edge too small: v1={}, v2={}, length = {}", v1, v2, lenr);
      return false;
    }
    e_x = (1./lenr) * e_x;
    /* four neighbor edges */
    id bvars[4] = {NO_ID,NO_ID,NO_ID,NO_ID};
    double alpha[4] = {0.,0.,0.,0.};
    double CR_weight[4] = {-1./4,-1./4,-1./4,-1./4};
    vec3 prevN;
    F(s,2) { /* two triangles */
      id oe = uIEdgeToOld[e][s];
      id t = oe / 3;
      id le = oe % 3;
      vec3 N = triangle_normal(M,t);
      if (s == 1 && dot(prevN,N) < 0.)  N = -1. * N;
      prevN = N;
      // N = {0,0,1};
      vec3 e_y = cross(N,e_x);
      if (length(e_y) < EPS) {
        error("length(e_y) = {}", length(e_y));
        return false;
      }
      e_y = normalize(e_y);

      F(k,2) { /* two other edges in triangle t */
        id aoe = 3*t+(le+1+k)%3;
        id ae = old2IEdge[aoe];
        id2 aedge = uIEdges[ae];

        bvars[2*s+k] = ae;
        vec3 edg = M.points[aedge[1]]-M.points[aedge[0]];
        double len = length(edg);
        if (len < EPS) {
          error("edge too small: t={}, k = {}, length = {}", t, k, len);
          return false;
        }
        edg = (1./len) * edg;

        /* 360deg angle (used for the angle rotation) */
        double cx = dot(edg,e_x);
        double cy = dot(edg,e_y);
        alpha[2*s+k] = atan2(cy,cx);
        if (alpha[2*s+k] < 0.) alpha[2*s+k] += 2.*M_PI;
        if (DBG_VERBOSE) info("  e={} | t={}, k={} <-> ae={} ({}->{}) | angle(e{},e{})={}", e, t, k, ae, aedge[0],aedge[1], e, ae, 180./M_PI*alpha[2*s+k]);

        /* 180deg edge-edge angle (used for the Crouzeix Raviart) */
        double agl = 0.;
        if (aedge[0] == v1) {
          agl = angle_nvectors(edg,e_x);
        } else if (aedge[1] == v1) {
          agl = angle_nvectors(edg, -1. * e_x);
        } else if (aedge[0] == v2) {
          agl = angle_nvectors(-1. * edg, e_x);
        } else if (aedge[1] == v2) {
          agl = angle_nvectors(-1. * edg, -1. * e_x);
        } else {
          error("should not happen");
          return false;
        }
        CR_weight[2*s+k] = -2. / tan(agl);
        if (DBG_VERBOSE) info("   agl = {} -> CR_weight = {}", agl*180./M_PI, CR_weight[2*s+k]);
      }
    }
    /* compute coefficients */
    double isum = -1. * (CR_weight[0] + CR_weight[1] + CR_weight[2] + CR_weight[3]);
    F(k,4) CR_weight[k] = CR_weight[k] / isum;

    iv.clear();
    ijv.clear();
    id x_i = 2*e;
    id y_i = 2*e+1;
    iv.push_back({x_i,1.});
    iv.push_back({y_i,1.});
    F(j,4) {
      id x_j = 2*bvars[j];
      id y_j = 2*bvars[j]+1;
      ijv.push_back({{x_i,x_j},CR_weight[j]    *cos(4.*alpha[j])});
      ijv.push_back({{x_i,y_j},CR_weight[j]*-1.*sin(4.*alpha[j])});
      ijv.push_back({{y_i,x_j},CR_weight[j]    *sin(4.*alpha[j])});
      ijv.push_back({{y_i,y_j},CR_weight[j]    *cos(4.*alpha[j])});
    }

    return true;
  }

  bool prepare_system(
      const vector<IV>& K_diag,
      const vector<IJV>& K_coefs,
      vector<vector<size_t>>& columns,
      vector<vector<double>>& values) {
    vector<IJV> coefs = K_coefs;
    coefs.resize(coefs.size()+K_diag.size());
    F(i,K_diag.size()) coefs[K_coefs.size()+i] = {{id(K_diag[i].i),id(K_diag[i].i)},K_diag[i].val};
    std::sort(coefs.begin(),coefs.end());

    size_t cur_i = coefs[0].ij[0];
    size_t cur_j = coefs[0].ij[1];
    double acc_val = coefs[0].val;
    for (size_t k = 1; k < coefs.size(); ++k) {
      id i = coefs[k].ij[0];
      id j = coefs[k].ij[1];
      double v = coefs[k].val;
      if (i != cur_i) {
        if (std::abs(acc_val) > EPS) {
          columns[cur_i].push_back(cur_j);
          values[cur_i].push_back(acc_val);
        }
        cur_i = i;
        acc_val = v;
        cur_j = j;
      } else if (j != cur_j) {
        if (std::abs(acc_val) > EPS) {
          columns[cur_i].push_back(cur_j);
          values[cur_i].push_back(acc_val);
        }
        cur_i = i;
        acc_val = v;
        cur_j = j;
      } else {
        acc_val += v;
      }
    }
    if (std::abs(acc_val) > EPS) {
      columns[cur_i].push_back(cur_j);
      values[cur_i].push_back(acc_val);
    }

    return true;
  }

  bool create_view_with_crosses(
      const std::string& name,
      const TMesh& M,
      const vector<id2>& uIEdges,
      const vector<vector<id>>& uIEdgeToOld,
      const vector<double>& x) {
    vector<vec3> vertAvg(M.points.size(),{0.,0.,0.});
    vector<double> vsum(M.points.size(),0.);

    std::vector<double> data_VP;
    std::vector<double> data_VP_rep;
    vec3 rep_ex = {1.,0.,0.};
    vec3 rep_ey = {0.,1.,0.};
    F(e,uIEdges.size()) {
      /* Average normal */
      vec3 N = triangle_normal(M,uIEdgeToOld[e][0]/3);
      if (uIEdgeToOld[e].size() == 2) {
        N = 0.5 * (N + triangle_normal(M,uIEdgeToOld[e][1]/3));
      }

      vec3 edg = normalize(M.points[uIEdges[e][1]]-M.points[uIEdges[e][0]]);
      vec3 edgo = normalize(cross(N,edg));
      vec3 p = 0.5*(M.points[uIEdges[e][0]]+M.points[uIEdges[e][1]]);
      double len = std::sqrt(x[2*e]*x[2*e]+x[2*e+1]*x[2*e+1]);
      if (len > EPS) {
        double theta = 1./4*atan2(x[2*e+1]/len,x[2*e]/len);
        vec3 cross1 = cos(theta) * edg + sin(theta) * edgo;
        cross1 = len * cross1;
        vec3 cross2 = cross(N,cross1);
        F(d,3) data_VP.push_back(p[d]);
        F(d,3) data_VP.push_back(cross1[d]);
        F(d,3) data_VP.push_back(p[d]);
        F(d,3) data_VP.push_back(cross2[d]);

        { /* representation vector averaging (code only in planar case) */
          vec3 cr1 = cos(theta) * edg + sin(theta) * edgo;
          double rep_theta = atan2(dot(cr1,rep_ey),dot(cr1,rep_ex));
          vec3 vrep = cos(4.*rep_theta) * rep_ex + sin(4.*rep_theta) * rep_ey;
          // F(d,3) data_VP_rep.push_back(p[d]);
          // F(d,3) data_VP_rep.push_back(vrep[d]);
          F(lv,2) {
            id v = uIEdges[e][lv];
            vsum[v] += 1.;
            vertAvg[v] = vertAvg[v] + vrep;
          }
        }

      }
    }
    int view = gmsh::view::add(name);
    gmsh::view::addListData(view, "VP", data_VP.size()/6, data_VP);

    FC(v,M.points.size(),vsum[v] > 0.) {
      vec3 p = M.points[v];
      vec3 rep = (1./vsum[v]) * vertAvg[v];
      F(d,3) data_VP_rep.push_back(p[d]);
      F(d,3) data_VP_rep.push_back(rep[d]);
    }
    int viewv = gmsh::view::add(name+"_rep_planar");
    gmsh::view::addListData(viewv, "VP", data_VP_rep.size()/6, data_VP_rep);

    return true;
  }

  bool compute_cross_field_with_heat(
      const std::string& meshName,
      int& crossFieldTag,
      int nbIter,
      std::map<std::pair<size_t,size_t>,double>* edge_to_angle) {
    gmsh::initialize(0, 0, false);
    info("compute cross field with successive heat diffusion and projection ...");

    TMesh M;
    bool oki = import_TMesh_from_gmsh(meshName, M);
    if (!oki) {
      error("failed to get mesh grom gmsh API");
      return false;
    }

    vector<id2> uIEdges;
    vector<id> old2IEdge;
    vector<vector<id>> uIEdgeToOld;
    bool oka = compute_triangle_adjacencies(M.triangles, M.triangle_neighbors, M.nm_triangle_neighbors, uIEdges, old2IEdge, uIEdgeToOld);
    if (!oka) {
      error("failed to compute mesh adjacencies");
      return false;
    }

    info("input: {} points, {} lines, {} triangles, {} internal edges", M.points.size(),M.lines.size(),M.triangles.size(),uIEdges.size());
    if (uIEdges.size() == 0) {
      error("no internal edges");
      return false;
    }

    size_t nbc = 0;
    vector<bool> dirichletEdge(uIEdges.size(),false);
    FC(e,uIEdges.size(),uIEdgeToOld[e].size() != 2) {
      dirichletEdge[e] = true;
      nbc += 1;
    }
    F(l,M.lines.size()) { /* mark the lines as boundary conditions */
      id2 edge = sorted(M.lines[l][0],M.lines[l][1]);
      auto it = std::find(uIEdges.begin(),uIEdges.end(),edge);
      if (it != uIEdges.end()) {
        id e = id(it - uIEdges.begin());
        dirichletEdge[e] = true;
        nbc += 1;
      }
    }
    info("boundary conditions: {} crosses fixed on edges", nbc);

    info("compute stiffness matrix coefficients (Crouzeix-Raviart) ...");
    vector<IV> K_diag;
    vector<IJV> K_coefs;
    vector<double> rhs(2*uIEdges.size(),0.);
    vector<double> Mass(2*uIEdges.size(),1.); /* diagonal for Crouzeix-Raviart */
    F(e,uIEdges.size()) {
      if (!dirichletEdge[e]) {
        vector<IV> iv;
        vector<IJV> ijv;
        bool oks = stiffness_coefficient(M, e, uIEdges, old2IEdge, uIEdgeToOld, iv, ijv);
        if (!oks) {
          error("failed to compute stiffness matrix coefficients for e = {}", e);
          return false;
        }
        F(i,iv.size()) K_diag.push_back(iv[i]);
        F(i,ijv.size()) K_coefs.push_back(ijv[i]);
        double area1 = triangle_area(M, uIEdgeToOld[e][0]/3);
        double area2 = triangle_area(M, uIEdgeToOld[e][1]/3);
        Mass[2*e+0] = 1./3 * (area1 + area2);
        Mass[2*e+1] = 1./3 * (area1 + area2);
      } else { /* Dirichlet BC */
        /* theta_e = 0 => (cos4t/sin4t) = (1,0) */
        K_diag.push_back({id(2*e+0),1.});
        rhs[2*e+0] = 1.;
        K_diag.push_back({id(2*e+1),1.});
        rhs[2*e+1] = 0.;
        if (DBG_VERBOSE) {
          info(" dirichlet: x[{}]={} (edge {}, p1={}, p2 = {}])",2*e,1.,e,M.points[uIEdges[e][0]],M.points[uIEdges[e][1]]);
          info(" dirichlet: x[{}]={} (edge {})",2*e+1,0.,e);
        }
      }
    }

    double eavg = 0.;
    double emin = DBL_MAX;
    double emax = -DBL_MAX;
    F(e,uIEdges.size()) {
      double len = length(M.points[uIEdges[e][1]]-M.points[uIEdges[e][0]]);
      eavg += len;
      emin = std::min(len,emin);
      emax = std::max(len,emax);
    }
    eavg /= uIEdges.size();

    info("edge size: min={}, avg={}, max={}",emin,eavg,emax);


    /* prepare system */
    vector<vector<size_t>> K_columns(2*uIEdges.size());
    vector<vector<double>> K_values(2*uIEdges.size());
    vector<double> x(2*uIEdges.size(),0.);
    {
      bool okp = prepare_system(K_diag, K_coefs, K_columns, K_values);
      if (!okp) {
        error("failed to prepare system");
        return false;
      }
      if (DBG_VERBOSE) {
        F(i,K_columns.size()) {
          info("-");
          info("row {}, j: {}", i, K_columns[i]);
          info("row {}, v: {}", i, K_values[i]);
        }
        info("rhs: {}",rhs);
      }
    }
    
    info("heat diffusion and projection loop ({} iterations, {} unknowns) ...", nbIter, 2*uIEdges.size());
    double dtFinal = emin*emin;
    double dtInitial = emax*emax;
    x = rhs;
    nbIter = 10;
    F(iter,nbIter) {
      double dt = dtInitial + (dtFinal-dtInitial) * (iter/(nbIter-1));
      const vector<vector<size_t>>& Acol = K_columns;
      vector<vector<double>> Aval = K_values;
      vector<double> B = x;
      F(i,Aval.size()) {
        if (!dirichletEdge[i/2]) {
          F(j,Acol[i].size()) {
            if (Acol[i][j] == i) { /* diagonal term */
              Aval[i][j] = 1. + dt / Mass[i] * K_values[i][j];
            } else {
              Aval[i][j] = dt / Mass[i] * K_values[i][j];
            }
          }
        } else {
          Aval[i] = {1.};
        }
      }

      info("  iter {}/{} | dt = {}, solving linear system ...", iter+1, nbIter, dt);
      bool oks = solve_sparse_linear_system(Acol, Aval, B, x);
      if (!oks) {
        error("failed to solve linear system");
        return false;
      }
      if (DBG_VERBOSE) {
        info("  -> x: {}",x);
        create_view_with_crosses("crosses_"+std::to_string(iter),M, uIEdges, uIEdgeToOld, x);
      }
      if (DBG_VERBOSE) info("  iter {}/{} | normalize crosses ...", iter+1,nbIter);
      vector<double> norms(uIEdges.size(),0.);
      F(e,uIEdges.size()) {
        norms[e] = std::sqrt(x[2*e]*x[2*e]+x[2*e+1]*x[2*e+1]);
        if (!dirichletEdge[e] && norms[e] > EPS) {
          x[2*e+0] /= norms[e];
          x[2*e+1] /= norms[e];
        }
      }
      if (DBG_VERBOSE) info("  -> norms: {}",norms);
    }

    {
      info("create visualization view with crosses");
      create_view_with_crosses("crosses", M, uIEdges, uIEdgeToOld, x);
    }

    if (edge_to_angle) {
      info("fill the map edge_to_angle");
      F(e,uIEdges.size()) {
        double len = std::sqrt(x[2*e]*x[2*e]+x[2*e+1]*x[2*e+1]);
        double theta = (len > EPS) ? 1./4*atan2(x[2*e+1]/len,x[2*e]/len) : 0.;
        (*edge_to_angle)[std::make_pair(uIEdges[e][0],uIEdges[e][1])] = theta;
      }
    }

    info("... done");

    return true;
  }
}
