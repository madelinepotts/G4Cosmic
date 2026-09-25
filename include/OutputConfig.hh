#pragma once

#include "globals.hh"

#include <vector>

class OutputConfig {
public:
    static void SetFileName(const G4String& fileName);
    static const G4String& GetFileName();

    static void SetWriteTrackEnd(G4bool enabled);
    static G4bool GetWriteTrackEnd();

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
    static std::vector<G4String> sensitiveVolumeNames_;
};
