#pragma once

#include "globals.hh"

#include <vector>

class OutputConfig {
public:
    static void SetFileName(const G4String& fileName);
    static const G4String& GetFileName();

    static void SetWriteTrackEnd(G4bool enabled);
    static G4bool GetWriteTrackEnd();

    static void SetWriteReducedHits(G4bool enabled);
    static G4bool GetWriteReducedHits();

    // Supported values: copyNo, particleCopyNo, physicalVolume.
    // Aliases accepted by SetReducedHitMode(): particleTypeCopyNo.
    static void SetReducedHitMode(const G4String& mode);
    static const G4String& GetReducedHitMode();

    // Registered sensitive logical-volume names become ROOT hit-tree names.
    // DetectorConstruction clears this list during geometry construction and
    // RegisterSensitiveVolume() appends to it. RunAction creates one hit ntuple
    // per unique name before the output file is opened.
    static void ClearSensitiveVolumeNames();
    static void RegisterSensitiveVolumeName(const G4String& logicalVolumeName);
    static const std::vector<G4String>& GetSensitiveVolumeNames();
private:
    static G4String fileName_;
    static G4bool writeTrackEnd_;
    static G4bool writeReducedHits_;
    static G4String reducedHitMode_;
    static std::vector<G4String> sensitiveVolumeNames_;
};
