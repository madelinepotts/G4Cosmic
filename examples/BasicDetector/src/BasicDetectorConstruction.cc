#include "BasicDetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4VisAttributes.hh"

namespace BasicDetectorExample {

G4VPhysicalVolume* DetectorConstruction::BuildGeometry() {
  auto* nist = G4NistManager::Instance();
  auto* air = nist->FindOrBuildMaterial("G4_AIR");
  auto* scintillator = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");

  auto* worldSolid = new G4Box("WorldSolid", 1.0 * m, 1.0 * m, 1.0 * m);
  auto* worldLV = new G4LogicalVolume(worldSolid, air, "WorldLV");
  auto* worldPV = new G4PVPlacement(
      nullptr,
      {},
      worldLV,
      "WorldPV",
      nullptr,
      false,
      0,
      false);

  worldLV->SetVisAttributes(G4VisAttributes::GetInvisible());

  auto* detectorSolid = new G4Box(
      "BasicScintillatorSolid",
      25.0 * cm,
      25.0 * cm,
      2.5 * cm);

  auto* detectorLV = new G4LogicalVolume(
      detectorSolid,
      scintillator,
      "BasicScintillatorLV");

  auto* detectorVis = new G4VisAttributes(G4Colour(0.2, 0.7, 1.0));
  detectorVis->SetForceSolid(true);
  detectorLV->SetVisAttributes(detectorVis);

  new G4PVPlacement(
      nullptr,
      G4ThreeVector(0.0, 0.0, 0.0),
      detectorLV,
      "BasicScintillatorPV",
      worldLV,
      false,
      0,
      false);

  // The framework handles the sensitive detector implementation. The example
  // only declares which logical volume is sensitive.
  RegisterSensitiveVolume(detectorLV);

  return worldPV;
}

}  // namespace BasicDetectorExample
