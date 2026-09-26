#pragma once

#include "G4Version.hh"

#ifndef G4VERSION_NUMBER
#error "G4VERSION_NUMBER was not defined by Geant4. Unsupported Geant4 installation."
#endif

#if G4VERSION_NUMBER < 1000
#error "G4Cosmic supports Geant4 10.x and 11.x."
#endif

#define G4COSMIC_GEANT4_VERSION_NUMBER G4VERSION_NUMBER

#if G4VERSION_NUMBER >= 1100
#define G4COSMIC_GEANT4_MAJOR 11
#else
#define G4COSMIC_GEANT4_MAJOR 10
#endif
