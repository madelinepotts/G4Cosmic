#include "G4Cosmic/RunMetadata.hh"

#include "G4Cosmic/BuildInfo.hh"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string Trim(const std::string& text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const auto last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

std::string StripComment(const std::string& line) {
  const auto hash = line.find('#');
  return Trim(hash == std::string::npos ? line : line.substr(0, hash));
}

bool StartsWith(const std::string& value, const std::string& prefix) {
  return value.size() >= prefix.size() &&
         value.compare(0, prefix.size(), prefix) == 0;
}

std::string QuoteIfNeeded(const std::string& value) {
  if (value.find_first_of(" \t\"'") == std::string::npos) {
    return value;
  }

  std::string quoted = "\"";
  for (char c : value) {
    if (c == '"' || c == '\\') quoted.push_back('\\');
    quoted.push_back(c);
  }
  quoted.push_back('"');
  return quoted;
}

std::string BuildCommandLine(int argc, char** argv) {
  std::ostringstream command;
  for (int i = 0; i < argc; ++i) {
    if (i != 0) command << ' ';
    command << QuoteIfNeeded(argv[i] == nullptr ? "" : argv[i]);
  }
  return command.str();
}

std::string UtcTimestamp(std::chrono::system_clock::time_point timePoint) {
  const auto time = std::chrono::system_clock::to_time_t(timePoint);
  std::tm utc{};
#ifdef _WIN32
  gmtime_s(&utc, &time);
#else
  gmtime_r(&time, &utc);
#endif

  std::ostringstream out;
  out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

std::string NormalizePath(const std::filesystem::path& path) {
  return path.lexically_normal().generic_string();
}

std::filesystem::path ResolvePath(const std::string& path,
                                  const std::filesystem::path& baseDir) {
  std::filesystem::path p(path);
  if (p.is_absolute()) {
    return p.lexically_normal();
  }
  return (baseDir / p).lexically_normal();
}

std::string FirstArgumentAfterCommand(const std::string& command,
                                      const std::string& commandName) {
  if (!StartsWith(command, commandName)) return "";
  std::string rest = Trim(command.substr(commandName.size()));
  if (rest.empty()) return "";

  if (rest.front() == '"') {
    std::string result;
    bool escaped = false;
    for (std::size_t i = 1; i < rest.size(); ++i) {
      const char c = rest[i];
      if (escaped) {
        result.push_back(c);
        escaped = false;
        continue;
      }
      if (c == '\\') {
        escaped = true;
        continue;
      }
      if (c == '"') {
        return result;
      }
      result.push_back(c);
    }
    return result;
  }

  std::istringstream input(rest);
  std::string result;
  input >> result;
  return result;
}

// Minimal SHA-256 implementation for file fingerprints. This avoids adding a
// crypto dependency solely for metadata provenance.
class Sha256 {
public:
  void Update(const std::uint8_t* data, std::size_t length) {
    totalBytes_ += length;
    while (length > 0) {
      const std::size_t toCopy = std::min<std::size_t>(length, 64 - bufferSize_);
      std::copy(data, data + toCopy, buffer_.begin() + static_cast<std::ptrdiff_t>(bufferSize_));
      bufferSize_ += toCopy;
      data += toCopy;
      length -= toCopy;
      if (bufferSize_ == 64) {
        Transform(buffer_.data());
        bufferSize_ = 0;
      }
    }
  }

  std::string FinalHex() {
    const std::uint64_t totalBits = static_cast<std::uint64_t>(totalBytes_) * 8ULL;

    buffer_[bufferSize_++] = 0x80;
    if (bufferSize_ > 56) {
      while (bufferSize_ < 64) buffer_[bufferSize_++] = 0;
      Transform(buffer_.data());
      bufferSize_ = 0;
    }
    while (bufferSize_ < 56) buffer_[bufferSize_++] = 0;

    for (int i = 7; i >= 0; --i) {
      buffer_[bufferSize_++] = static_cast<std::uint8_t>((totalBits >> (i * 8)) & 0xffU);
    }
    Transform(buffer_.data());

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto word : state_) {
      out << std::setw(8) << word;
    }
    return out.str();
  }

private:
  static std::uint32_t RotR(std::uint32_t x, std::uint32_t n) {
    return (x >> n) | (x << (32U - n));
  }

  static std::uint32_t Ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (~x & z);
  }

  static std::uint32_t Maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
  }

  static std::uint32_t BigSigma0(std::uint32_t x) {
    return RotR(x, 2) ^ RotR(x, 13) ^ RotR(x, 22);
  }

  static std::uint32_t BigSigma1(std::uint32_t x) {
    return RotR(x, 6) ^ RotR(x, 11) ^ RotR(x, 25);
  }

  static std::uint32_t SmallSigma0(std::uint32_t x) {
    return RotR(x, 7) ^ RotR(x, 18) ^ (x >> 3);
  }

  static std::uint32_t SmallSigma1(std::uint32_t x) {
    return RotR(x, 17) ^ RotR(x, 19) ^ (x >> 10);
  }

  void Transform(const std::uint8_t* chunk) {
    static constexpr std::array<std::uint32_t, 64> k = {{
      0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
      0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
      0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
      0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
      0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
      0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
      0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
      0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
      0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
      0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
      0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
      0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
      0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
      0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
      0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
      0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
    }};

    std::array<std::uint32_t, 64> w{};
    for (int i = 0; i < 16; ++i) {
      w[i] = (static_cast<std::uint32_t>(chunk[i * 4]) << 24) |
             (static_cast<std::uint32_t>(chunk[i * 4 + 1]) << 16) |
             (static_cast<std::uint32_t>(chunk[i * 4 + 2]) << 8) |
             static_cast<std::uint32_t>(chunk[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
      w[i] = SmallSigma1(w[i - 2]) + w[i - 7] + SmallSigma0(w[i - 15]) + w[i - 16];
    }

    std::uint32_t a = state_[0];
    std::uint32_t b = state_[1];
    std::uint32_t c = state_[2];
    std::uint32_t d = state_[3];
    std::uint32_t e = state_[4];
    std::uint32_t f = state_[5];
    std::uint32_t g = state_[6];
    std::uint32_t h = state_[7];

    for (int i = 0; i < 64; ++i) {
      const std::uint32_t t1 = h + BigSigma1(e) + Ch(e, f, g) + k[i] + w[i];
      const std::uint32_t t2 = BigSigma0(a) + Maj(a, b, c);
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
  }

  std::array<std::uint32_t, 8> state_ = {{
    0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
    0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
  }};
  std::array<std::uint8_t, 64> buffer_{};
  std::size_t bufferSize_ = 0;
  std::size_t totalBytes_ = 0;
};

}  // namespace

namespace G4Cosmic {

RunMetadata& RunMetadata::Instance() {
  static RunMetadata metadata;
  return metadata;
}

void RunMetadata::Begin(int argc, char** argv, int requestedThreads) {
  commandLine_ = BuildCommandLine(argc, argv);
  requestedThreads_ = requestedThreads;
  startTime_ = std::chrono::system_clock::now();
  endTime_ = startTime_;
  began_ = true;

  try {
    workingDirectory_ = NormalizePath(std::filesystem::current_path());
  } catch (...) {
    workingDirectory_.clear();
  }

  macroFile_.clear();
  macroFiles_.clear();
  sourceMetadataContributor_ = nullptr;
  detectorMetadataContributor_ = nullptr;
  userMetadataContributors_.clear();
}

void RunMetadata::SetMacroFile(const std::string& macroFile) {
  macroFile_ = macroFile;
  LoadMacroFiles();
}

void RunMetadata::SetPrintMacroCommands(bool enabled) {
  printMacroCommands_ = enabled;
}

bool RunMetadata::GetPrintMacroCommands() const {
  return printMacroCommands_;
}

void RunMetadata::SetSourceMetadataContributor(MetadataCallback callback) {
  sourceMetadataContributor_ = std::move(callback);
}

void RunMetadata::SetDetectorMetadataContributor(MetadataCallback callback) {
  detectorMetadataContributor_ = std::move(callback);
}

void RunMetadata::AddUserMetadataContributor(MetadataCallback callback) {
  if (callback) {
    userMetadataContributors_.push_back(std::move(callback));
  }
}

void RunMetadata::PrintMacroCommands(std::ostream& out) const {
  if (macroFiles_.empty()) {
    out << "G4Cosmic metadata: no macro commands were recorded.\n";
    return;
  }

  out << "G4Cosmic macro commands";
  if (!macroFile_.empty()) {
    out << " from " << macroFile_;
  }
  out << ":\n";

  for (const auto& file : macroFiles_) {
    out << "  " << file.path << "\n";
    for (const auto& command : file.commands) {
      out << "    " << command << "\n";
    }
  }
}

void RunMetadata::LoadMacroFiles() {
  macroFiles_.clear();
  if (macroFile_.empty()) {
    return;
  }

  std::vector<std::string> includeStack;
  LoadMacroFileRecursive(macroFile_, includeStack);
}

void RunMetadata::LoadMacroFileRecursive(const std::string& path,
                                         std::vector<std::string>& includeStack) {
  const auto baseDir = includeStack.empty()
      ? std::filesystem::current_path()
      : std::filesystem::path(includeStack.back()).parent_path();
  const auto resolved = ResolvePath(path, baseDir);
  const auto normalized = NormalizePath(resolved);

  for (const auto& active : includeStack) {
    if (active == normalized) {
      return;
    }
  }

  for (const auto& file : macroFiles_) {
    if (file.path == normalized) {
      return;
    }
  }

  MacroFileRecord record;
  record.path = normalized;
  record.sha256 = Sha256File(normalized);

  std::ifstream input(resolved);
  if (!input) {
    macroFiles_.push_back(record);
    return;
  }

  includeStack.push_back(normalized);

  std::vector<std::string> includedMacros;
  std::string line;
  while (std::getline(input, line)) {
    const auto command = StripComment(line);
    if (command.empty()) {
      continue;
    }

    record.commands.push_back(command);

    const auto includePath = FirstArgumentAfterCommand(command, "/control/execute");
    if (!includePath.empty()) {
      includedMacros.push_back(includePath);
    }
  }

  macroFiles_.push_back(std::move(record));
  for (const auto& includePath : includedMacros) {
    LoadMacroFileRecursive(includePath, includeStack);
  }
  includeStack.pop_back();
}

std::string RunMetadata::JsonFileNameForRootFile(const std::string& rootFileName) {
  if (rootFileName.size() >= 5) {
    const auto suffix = rootFileName.substr(rootFileName.size() - 5);
    std::string lower = suffix;
    for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower == ".root") {
      return rootFileName.substr(0, rootFileName.size() - 5) + ".json";
    }
  }
  return rootFileName + ".json";
}

std::string RunMetadata::Sha256File(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return "";
  }

  Sha256 sha;
  std::array<char, 32768> buffer{};
  while (input) {
    input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto count = input.gcount();
    if (count > 0) {
      sha.Update(reinterpret_cast<const std::uint8_t*>(buffer.data()),
                 static_cast<std::size_t>(count));
    }
  }
  return sha.FinalHex();
}

void RunMetadata::WriteSidecar(const std::string& rootFileName) {
  if (!began_) {
    startTime_ = std::chrono::system_clock::now();
  }
  endTime_ = std::chrono::system_clock::now();
  const auto jsonFileName = JsonFileNameForRootFile(rootFileName);

  std::ofstream output(jsonFileName);
  if (!output) {
    std::cerr << "G4Cosmic warning: unable to write metadata JSON sidecar: "
              << jsonFileName << "\n";
    return;
  }

  const std::chrono::duration<double> duration = endTime_ - startTime_;

  JsonWriter json(output);
  json.BeginObject();
  json.Write("schema_version", 1);
  json.Write("g4cosmic_version", G4COSMIC_VERSION);

  json.BeginObject("run");
  json.Write("start_time_utc", UtcTimestamp(startTime_));
  json.Write("end_time_utc", UtcTimestamp(endTime_));
  json.Write("duration_seconds", duration.count());
  json.Write("command_line", commandLine_);
  json.Write("working_directory", workingDirectory_);
  json.Write("threads", requestedThreads_);
  if (!macroFile_.empty()) {
    json.Write("macro_file", macroFile_);
  }
  json.EndObject();

  json.BeginObject("output");
  json.Write("root_file", rootFileName);
  json.Write("json_file", jsonFileName);
  json.EndObject();

  json.BeginObject("build");
  json.Write("git_commit", G4COSMIC_GIT_COMMIT);
  json.Write("git_branch", G4COSMIC_GIT_BRANCH);
  json.Write("git_tag", G4COSMIC_GIT_TAG);
  json.Write("git_dirty", std::string(G4COSMIC_GIT_DIRTY) == "1");
  json.Write("compiler_id", G4COSMIC_COMPILER_ID);
  json.Write("compiler_version", G4COSMIC_COMPILER_VERSION);
  json.Write("geant4_version", G4COSMIC_GEANT4_VERSION);
  json.EndObject();

  json.BeginObject("macro");
  json.Write("entry_file", macroFile_);
  json.BeginArray("files");
  for (const auto& file : macroFiles_) {
    json.BeginObject();
    json.Write("path", file.path);
    json.Write("sha256", file.sha256);
    json.BeginArray("commands");
    for (const auto& command : file.commands) {
      json.WriteValue(command);
    }
    json.EndArray();
    json.EndObject();
  }
  json.EndArray();
  json.EndObject();

  if (sourceMetadataContributor_) {
    json.BeginObject("source");
    sourceMetadataContributor_(json);
    json.EndObject();
  }

  if (detectorMetadataContributor_) {
    json.BeginObject("detector");
    detectorMetadataContributor_(json);
    json.EndObject();
  }

  if (!userMetadataContributors_.empty()) {
    json.BeginObject("user_metadata");
    for (const auto& contributor : userMetadataContributors_) {
      if (contributor) {
        contributor(json);
      }
    }
    json.EndObject();
  }

  json.EndObject();
  output << '\n';
}

}  // namespace G4Cosmic
