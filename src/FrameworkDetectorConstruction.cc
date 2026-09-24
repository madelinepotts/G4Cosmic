#include "G4Cosmic/DetectorConstruction.hh"

#include "G4Cosmic/GenericSensitiveDetector.hh"

#include "G4LogicalVolume.hh"
#include "G4SDManager.hh"

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

void DetectorConstruction::ConstructSDandField() {
  if (sensitiveVolumes_.empty()) {
    return;
  }

  auto* sdManager = G4SDManager::GetSDMpointer();
  auto* detector = new GenericSensitiveDetector("G4CosmicGenericSD");
  sdManager->AddNewDetector(detector);

  for (auto* volume : sensitiveVolumes_) {
    if (volume != nullptr) {
      volume->SetSensitiveDetector(detector);
    }
  }
}

}  // namespace G4Cosmic
