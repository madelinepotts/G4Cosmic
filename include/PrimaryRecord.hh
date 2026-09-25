#pragma once

#include "G4String.hh"
#include "G4ThreeVector.hh"

struct PrimaryRecord {
  int eventID = -1;
  int primaryIndex = -1;
  int pdg = 0;
  G4String particleName;

  double kineticEnergy = 0.0;
  double time = 0.0;
  G4ThreeVector position;
  G4ThreeVector direction;
};
