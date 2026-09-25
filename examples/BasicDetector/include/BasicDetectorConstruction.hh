#pragma once

#include "G4Cosmic/DetectorConstruction.hh"

class G4VPhysicalVolume;

namespace BasicDetectorExample {

class DetectorConstruction : public G4Cosmic::DetectorConstruction {
public:
  DetectorConstruction() = default;
  ~DetectorConstruction() override = default;

protected:
  G4VPhysicalVolume* BuildGeometry() override;
};

}  // namespace BasicDetectorExample
