#pragma once

#include "G4String.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

struct HitRecord {
  G4int eventID = -1;
  G4int trackID = -1;
  G4int parentID = -1;
  G4int pdg = 0;
  G4String particleName;

  // Leaf physical-volume copy number at the hit point.  Detector-specific
  // channel mapping belongs in the downstream detector example or analysis,
  // not in the framework record.
  G4int copyNo = -1;

  G4double edep = 0.0;
  G4double time = 0.0;
  G4ThreeVector position;

  G4String physicalVolumeName;
  G4String logicalVolumeName;
};
