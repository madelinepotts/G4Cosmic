#include "OutputConfig.hh"

#include <algorithm>
#include <cctype>
#include <string>

namespace {

G4String NormalizeMode(const G4String& mode) {
    std::string m(mode);
    std::string compact;
    compact.reserve(m.size());

    for (char c : m) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) {
            compact.push_back(static_cast<char>(std::tolower(uc)));
        }
    }

    if (compact == "copyno" || compact == "copy") {
        return "copyNo";
    }
    if (compact == "particlecopyno" || compact == "particletypecopyno" ||
        compact == "pdgcopyno") {
        return "particleCopyNo";
    }
    if (compact == "physicalvolume" || compact == "physicalvolumecopyno" ||
        compact == "volume") {
        return "physicalVolume";
    }

    return "";
}

}  // namespace

G4String OutputConfig::fileName_ = "g4cosmic.root";
G4bool OutputConfig::writeTrackEnd_ = false;
G4bool OutputConfig::writeReducedHits_ = false;
G4String OutputConfig::reducedHitMode_ = "copyNo";
std::vector<G4String> OutputConfig::sensitiveVolumeNames_;

void OutputConfig::SetFileName(const G4String& fileName) {
    if (!fileName.empty()) fileName_ = fileName;
}
const G4String& OutputConfig::GetFileName() { return fileName_; }

void OutputConfig::SetWriteTrackEnd(G4bool enabled) { writeTrackEnd_ = enabled; }
G4bool OutputConfig::GetWriteTrackEnd() { return writeTrackEnd_; }

void OutputConfig::SetWriteReducedHits(G4bool enabled) { writeReducedHits_ = enabled; }
G4bool OutputConfig::GetWriteReducedHits() { return writeReducedHits_; }

void OutputConfig::SetReducedHitMode(const G4String& mode) {
    const auto normalized = NormalizeMode(mode);
    if (!normalized.empty()) {
        reducedHitMode_ = normalized;
    }
}
const G4String& OutputConfig::GetReducedHitMode() { return reducedHitMode_; }

void OutputConfig::ClearSensitiveVolumeNames() {
    sensitiveVolumeNames_.clear();
}

void OutputConfig::RegisterSensitiveVolumeName(const G4String& logicalVolumeName) {
    if (logicalVolumeName.empty()) {
        return;
    }

    const auto it = std::find(
        sensitiveVolumeNames_.begin(),
        sensitiveVolumeNames_.end(),
        logicalVolumeName);
    if (it == sensitiveVolumeNames_.end()) {
        sensitiveVolumeNames_.push_back(logicalVolumeName);
    }
}

const std::vector<G4String>& OutputConfig::GetSensitiveVolumeNames() {
    return sensitiveVolumeNames_;
}
