typedef union {
  char    *c;
  int      i;
  unsigned int u;
  double   d;
  double   v[5];
  Shape    s;
  List_T  *l;
} YYSTYPE;
#define	tDOUBLE	257
#define	tSTRING	258
#define	tBIGSTR	259
#define	tEND	260
#define	tAFFECT	261
#define	tDOTS	262
#define	tPi	263
#define	tExp	264
#define	tLog	265
#define	tLog10	266
#define	tSqrt	267
#define	tSin	268
#define	tAsin	269
#define	tCos	270
#define	tAcos	271
#define	tTan	272
#define	tAtan	273
#define	tAtan2	274
#define	tSinh	275
#define	tCosh	276
#define	tTanh	277
#define	tFabs	278
#define	tFloor	279
#define	tCeil	280
#define	tFmod	281
#define	tModulo	282
#define	tHypot	283
#define	tPrintf	284
#define	tPoint	285
#define	tCircle	286
#define	tEllipsis	287
#define	tLine	288
#define	tSurface	289
#define	tSpline	290
#define	tVolume	291
#define	tCharacteristic	292
#define	tLength	293
#define	tParametric	294
#define	tElliptic	295
#define	tPlane	296
#define	tRuled	297
#define	tTransfinite	298
#define	tComplex	299
#define	tPhysical	300
#define	tUsing	301
#define	tBump	302
#define	tProgression	303
#define	tRotate	304
#define	tTranslate	305
#define	tSymmetry	306
#define	tDilate	307
#define	tExtrude	308
#define	tDuplicata	309
#define	tLoop	310
#define	tInclude	311
#define	tRecombine	312
#define	tDelete	313
#define	tCoherence	314
#define	tView	315
#define	tAttractor	316
#define	tLayers	317
#define	tScalarTetrahedron	318
#define	tVectorTetrahedron	319
#define	tTensorTetrahedron	320
#define	tScalarTriangle	321
#define	tVectorTriangle	322
#define	tTensorTriangle	323
#define	tScalarLine	324
#define	tVectorLine	325
#define	tTensorLine	326
#define	tScalarPoint	327
#define	tVectorPoint	328
#define	tTensorPoint	329
#define	tBSpline	330
#define	tNurbs	331
#define	tOrder	332
#define	tWith	333
#define	tBounds	334
#define	tKnots	335
#define	tColor	336
#define	tOptions	337
#define	tFor	338
#define	tEndFor	339
#define	tScript	340
#define	tExit	341
#define	tMerge	342
#define	tB_SPLINE_SURFACE_WITH_KNOTS	343
#define	tB_SPLINE_CURVE_WITH_KNOTS	344
#define	tCARTESIAN_POINT	345
#define	tTRUE	346
#define	tFALSE	347
#define	tUNSPECIFIED	348
#define	tU	349
#define	tV	350
#define	tEDGE_CURVE	351
#define	tVERTEX_POINT	352
#define	tORIENTED_EDGE	353
#define	tPLANE	354
#define	tFACE_OUTER_BOUND	355
#define	tEDGE_LOOP	356
#define	tADVANCED_FACE	357
#define	tVECTOR	358
#define	tDIRECTION	359
#define	tAXIS2_PLACEMENT_3D	360
#define	tISO	361
#define	tENDISO	362
#define	tENDSEC	363
#define	tDATA	364
#define	tHEADER	365
#define	tFILE_DESCRIPTION	366
#define	tFILE_SCHEMA	367
#define	tFILE_NAME	368
#define	tMANIFOLD_SOLID_BREP	369
#define	tCLOSED_SHELL	370
#define	tADVANCED_BREP_SHAPE_REPRESENTATION	371
#define	tFACE_BOUND	372
#define	tCYLINDRICAL_SURFACE	373
#define	tCONICAL_SURFACE	374
#define	tCIRCLE	375
#define	tTRIMMED_CURVE	376
#define	tGEOMETRIC_SET	377
#define	tCOMPOSITE_CURVE_SEGMENT	378
#define	tCONTINUOUS	379
#define	tCOMPOSITE_CURVE	380
#define	tTOROIDAL_SURFACE	381
#define	tPRODUCT_DEFINITION	382
#define	tPRODUCT_DEFINITION_SHAPE	383
#define	tSHAPE_DEFINITION_REPRESENTATION	384
#define	tELLIPSE	385
#define	tTrimmed	386
#define	tSolid	387
#define	tEndSolid	388
#define	tVertex	389
#define	tFacet	390
#define	tNormal	391
#define	tOuter	392
#define	tLoopSTL	393
#define	tEndLoop	394
#define	tEndFacet	395
#define	tAFFECTPLUS	396
#define	tAFFECTMINUS	397
#define	tAFFECTTIMES	398
#define	tAFFECTDIVIDE	399
#define	tAND	400
#define	tOR	401
#define	tNOTEQUAL	402
#define	tEQUAL	403
#define	tAPPROXEQUAL	404
#define	tLESSOREQUAL	405
#define	tGREATEROREQUAL	406
#define	tCROSSPRODUCT	407
#define	UNARYPREC	408
#define	tPLUSPLUS	409
#define	tMINUSMINUS	410


extern YYSTYPE yylval;
