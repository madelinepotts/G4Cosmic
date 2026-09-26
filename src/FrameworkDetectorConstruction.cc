#include "G4Cosmic/DetectorConstruction.hh"

#include "G4Cosmic/GenericSensitiveDetector.hh"
#include "G4Cosmic/JsonWriter.hh"

#include "G4LogicalVolume.hh"
#include "OutputConfig.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"

#include <set>
#include <string>

namespace G4Cosmic {


void DetectorConstruction::AppendMetadata(JsonWriter& json) const {
  json.Write("type", "G4Cosmic::DetectorConstruction");
  json.Write("metadata_source", "detector_construction");
  AppendSensitiveVolumeMetadata(json);
}

void DetectorConstruction::AppendSensitiveVolumeMetadata(JsonWriter& json) const {
  std::set<std::string> names;
  for (const auto* volume : sensitiveVolumes_) {
    if (volume != nullptr) {
      names.insert(volume->GetName());
    }
  }

  json.BeginArray("sensitive_logical_volumes");
  for (const auto& name : names) {
    json.WriteValue(name);
  }
  json.EndArray();
}

G4VPhysicalVolume* DetectorConstruction::Construct() {
  sensitiveVolumes_.clear();
  OutputConfig::ClearSensitiveVolumeNames();
  return BuildGeometry();
}

void DetectorConstruction::RegisterSensitiveVolume(G4LogicalVolume* volume) {
  if (volume != nullptr) {
    sensitiveVolumes_.push_back(volume);
    OutputConfig::RegisterSensitiveVolumeName(volume->GetName());
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
