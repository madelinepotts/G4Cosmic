#pragma once

#include "G4Cosmic/DetectorConstruction.hh"

class G4VPhysicalVolume;
class G4VSensitiveDetector;

namespace G4Cosmic { class JsonWriter; }

namespace WarpTrackExample {

class DetectorConstruction : public G4Cosmic::DetectorConstruction {
public:
  DetectorConstruction() = default;
  ~DetectorConstruction() override = default;

  void AppendMetadata(G4Cosmic::JsonWriter& json) const override;

protected:
  G4VPhysicalVolume* BuildGeometry() override;
  G4VSensitiveDetector* CreateSensitiveDetector() override;
};

}  // namespace WarpTrackExample
