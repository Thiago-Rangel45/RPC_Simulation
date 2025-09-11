#ifndef GarfieldAnalysis_h
#define GarfieldAnalysis_h 1

#include "G4Version.hh"

#if (G4VERSION_NUMBER < 1100)
  #include "g4root.hh"
  //#include "g4xml.hh"
#else
  #include "G4AnalysisManager.hh"
#endif 

#endif