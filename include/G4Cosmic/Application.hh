#pragma once

#include <functional>

class G4VUserDetectorConstruction;

namespace G4Cosmic {

class Application {
public:
  using DetectorFactory = std::function<G4VUserDetectorConstruction*()>;

  Application();
  explicit Application(DetectorFactory detectorFactory);
  ~Application() = default;

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  template <typename DetectorT>
  void SetDetector() {
    detectorFactory_ = []() -> G4VUserDetectorConstruction* {
      return new DetectorT();
    };
  }

  int Run(int argc, char** argv) const;

private:
  static int RequestedThreads(int argc, char** argv);

  DetectorFactory detectorFactory_;
};

}  // namespace G4Cosmic
