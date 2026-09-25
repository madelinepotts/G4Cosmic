#pragma once

#include "G4String.hh"
#include "globals.hh"

struct TrackEndRecord {
  G4int eventID = -1;
  G4int trackID = -1;
  G4int parentID = -1;
  G4int pdg = 0;
  G4String particleName;

  G4double startKineticEnergyMeV = 0.0;
  G4double endKineticEnergyMeV = 0.0;

  G4double xMm = 0.0;
  G4double yMm = 0.0;
  G4double zMm = 0.0;

  G4double trackLengthMm = 0.0;
  G4double globalTimeNs = 0.0;

  G4String endProcess;
  G4String material;
  G4String physicalVolume;
  G4String logicalVolume;

  G4bool stopped = false;
};
