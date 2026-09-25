#pragma once

#include "G4VUserDetectorConstruction.hh"

#include <vector>

class G4LogicalVolume;
class G4VPhysicalVolume;
class G4VSensitiveDetector;

namespace G4Cosmic {

class JsonWriter;

/**
 * \brief Framework base class for downstream detector geometries.
 *
 * Detector projects override BuildGeometry() with normal Geant4 C++ geometry
 * code. Volumes that should emit hit records are registered from that geometry
 * code with RegisterSensitiveVolume(). By default, registered volumes use
 * G4Cosmic::GenericSensitiveDetector. A detector example may override
 * CreateSensitiveDetector() when it needs detector-specific channel metadata.
 */
class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  DetectorConstruction() = default;
  ~DetectorConstruction() override = default;

  G4VPhysicalVolume* Construct() final;
  void ConstructSDandField() override;

  /**
   * \brief Append detector-specific provenance into the JSON sidecar.
   *
   * The core calls this inside the top-level `detector` object.  Downstream
   * projects can use it to record geometry versions, calibration tags, detector
   * dimensions, or other metadata without adding those concepts to G4Cosmic.
   */
  virtual void AppendMetadata(JsonWriter& json) const;

protected:
  virtual G4VPhysicalVolume* BuildGeometry() = 0;
  virtual G4VSensitiveDetector* CreateSensitiveDetector();

  void RegisterSensitiveVolume(G4LogicalVolume* volume);

private:
  std::vector<G4LogicalVolume*> sensitiveVolumes_;
};

}  // namespace G4Cosmic
