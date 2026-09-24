#pragma once

#include "G4Cosmic/DetectorConstruction.hh"

class G4VPhysicalVolume;
class G4VSensitiveDetector;

namespace WarpTrackExample {

class DetectorConstruction : public G4Cosmic::DetectorConstruction {
public:
  DetectorConstruction() = default;
  ~DetectorConstruction() override = default;

protected:
  G4VPhysicalVolume* BuildGeometry() override;
  G4VSensitiveDetector* CreateSensitiveDetector() override;
};

}  // namespace WarpTrackExample
