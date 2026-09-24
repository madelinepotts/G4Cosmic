#pragma once

#include "G4VUserDetectorConstruction.hh"

#include <vector>

class G4LogicalVolume;
class G4VPhysicalVolume;

namespace G4Cosmic {

// Framework base class for downstream detector geometries.
//
// Detector projects should override BuildGeometry() with normal Geant4 C++
// geometry code. Any logical volumes that should produce raw hit records can
// be registered from inside that geometry code with RegisterSensitiveVolume().
class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  DetectorConstruction() = default;
  ~DetectorConstruction() override = default;

  G4VPhysicalVolume* Construct() final;
  void ConstructSDandField() override;

protected:
  virtual G4VPhysicalVolume* BuildGeometry() = 0;
  void RegisterSensitiveVolume(G4LogicalVolume* volume);

private:
  std::vector<G4LogicalVolume*> sensitiveVolumes_;
};

}  // namespace G4Cosmic
