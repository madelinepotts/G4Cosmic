#pragma once

#include "G4Cosmic/JsonWriter.hh"

#include <chrono>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace G4Cosmic {

// Collects run provenance and writes the JSON sidecar next to each ROOT file.
// Detector/source/application code may append metadata without the core knowing
// detector-specific details.
class RunMetadata {
public:
  using MetadataCallback = std::function<void(JsonWriter&)>;

  struct MacroFileRecord {
    std::string path;
    std::string sha256;
    std::vector<std::string> commands;
  };

  static RunMetadata& Instance();

  void Begin(int argc, char** argv, int requestedThreads);
  void SetMacroFile(const std::string& macroFile);

  void SetPrintMacroCommands(bool enabled);
  bool GetPrintMacroCommands() const;
  void PrintMacroCommands(std::ostream& out) const;

  void SetSourceMetadataContributor(MetadataCallback callback);
  void SetDetectorMetadataContributor(MetadataCallback callback);
  void AddUserMetadataContributor(MetadataCallback callback);

  void WriteSidecar(const std::string& rootFileName);

  static std::string JsonFileNameForRootFile(const std::string& rootFileName);
  static std::string Sha256File(const std::string& path);

private:
  RunMetadata() = default;

  void LoadMacroFiles();
  void LoadMacroFileRecursive(const std::string& path,
                              std::vector<std::string>& includeStack);

  std::string commandLine_;
  std::string workingDirectory_;
  std::string macroFile_;
  std::vector<MacroFileRecord> macroFiles_;

  int requestedThreads_ = 1;
  bool printMacroCommands_ = false;
  bool began_ = false;

  std::chrono::system_clock::time_point startTime_;
  std::chrono::system_clock::time_point endTime_;

  MetadataCallback sourceMetadataContributor_;
  MetadataCallback detectorMetadataContributor_;
  std::vector<MetadataCallback> userMetadataContributors_;
};

}  // namespace G4Cosmic
