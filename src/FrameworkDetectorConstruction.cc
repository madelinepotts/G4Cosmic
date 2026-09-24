#include "G4Cosmic/DetectorConstruction.hh"

#include "G4Cosmic/GenericSensitiveDetector.hh"

#include "G4LogicalVolume.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"

namespace G4Cosmic {

G4VPhysicalVolume* DetectorConstruction::Construct() {
  sensitiveVolumes_.clear();
  return BuildGeometry();
}

void DetectorConstruction::RegisterSensitiveVolume(G4LogicalVolume* volume) {
  if (volume != nullptr) {
    sensitiveVolumes_.push_back(volume);
  }
}

G4VSensitiveDetector* DetectorConstruction::CreateSensitiveDetector() {
  return new GenericSensitiveDetector("G4CosmicGenericSD");
}

void DetectorConstruction::ConstructSDandField() {
  if (sensitiveVolumes_.empty()) {
    return;
  }

  auto* detector = CreateSensitiveDetector();
  if (detector == nullptr) {
    return;
  }

  auto* sdManager = G4SDManager::GetSDMpointer();
  sdManager->AddNewDetector(detector);

  for (auto* volume : sensitiveVolumes_) {
    if (volume != nullptr) {
      SetSensitiveDetector(volume, detector);
    }
  }
}

}  // namespace G4Cosmic
