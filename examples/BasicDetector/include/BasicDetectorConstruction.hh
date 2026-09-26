#pragma once

#include "G4Cosmic/DetectorConstruction.hh"

class G4VPhysicalVolume;

namespace G4Cosmic { class JsonWriter; }

namespace BasicDetectorExample {

class DetectorConstruction : public G4Cosmic::DetectorConstruction {
public:
  DetectorConstruction() = default;
  ~DetectorConstruction() override = default;

  void AppendMetadata(G4Cosmic::JsonWriter& json) const override;

protected:
  G4VPhysicalVolume* BuildGeometry() override;
};

}  // namespace BasicDetectorExample
