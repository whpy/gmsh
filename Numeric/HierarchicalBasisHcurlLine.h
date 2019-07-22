// Gmsh - Copyright (C) 1997-2019 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// issues on https://gitlab.onelab.info/gmsh/gmsh/issues.
//
// Contributed by Ismail Badia.
// Reference :  "Higher-Order Finite Element  Methods"; Pavel Solin, Karel
// Segeth ,
//                 Ivo Dolezel , Chapman and Hall/CRC; Edition : Har/Cdr (2003).

#ifndef HIERARCHICAL_BASIS_HCURL_LINE_H
#define HIERARCHICAL_BASIS_HCURL_LINE_H

#include <algorithm>
#include <vector>

#include "HierarchicalBasisHcurl.h"
/*
 *
 *
 *
 *          |
 *   0      |     1
 * --+------+-----+---> u
 *
 *
 *
 *
 *
 *
 *
 */
class HierarchicalBasisHcurlLine : public HierarchicalBasisHcurl {
public:
  HierarchicalBasisHcurlLine(int order);
  virtual ~HierarchicalBasisHcurlLine();

  virtual void generateBasis(double const &u, double const &v, double const &w,
                             std::vector<std::vector<double> > &vertexBasis,
                             std::vector<std::vector<double> > &edgeBasis,
                             std::vector<std::vector<double> > &faceBasis,
                             std::vector<std::vector<double> > &bubbleBasis,
                             std::string typeFunction)
  {
    if(typeFunction == "HcurlLegendre") {
      generateHcurlBasis(u, v, w, edgeBasis, faceBasis, bubbleBasis);
    }
    else if("CurlHcurlLegendre" == typeFunction) {
      generateCurlBasis(u, v, w, edgeBasis, faceBasis, bubbleBasis);
    }
    else {
      throw std::string("unknown typeFunction");
    }
  };
  virtual void orientEdge(int const &flagOrientation, int const &edgeNumber,
                          std::vector<std::vector<double> > &edgeBasis);
  virtual void orientFace(double const &u, double const &v, double const &w,
                          int const &flag1, int const &flag2, int const &flag3,
                          int const &faceNumber,
                          std::vector<std::vector<double> > &faceFunctions,
                          std::string typeFunction){};

private:
  int _pe; //  edge function order in  direction u
  static double
  _affineCoordinate(int j,
                    double u); // affine coordinate lambdaj j={1,2}
  virtual void
  generateHcurlBasis(double const &u, double const &v, double const &w,
                     std::vector<std::vector<double> > &edgeBasis,
                     std::vector<std::vector<double> > &faceBasis,
                     std::vector<std::vector<double> > &bubbleBasis);
  virtual void
  generateCurlBasis(double const &u, double const &v, double const &w,
                    std::vector<std::vector<double> > &edgeBasis,
                    std::vector<std::vector<double> > &faceBasis,
                    std::vector<std::vector<double> > &bubbleBasis);

  static double dotProduct(const std::vector<double> &u,
                           const std::vector<double> &v);
};

#endif
