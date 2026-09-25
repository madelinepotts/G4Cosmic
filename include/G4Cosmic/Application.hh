#pragma once

#include <functional>

class G4VUserDetectorConstruction;

namespace G4Cosmic {

/**
 * \brief Small application wrapper that wires a detector into the G4Cosmic framework.
 *
 * Application owns the high-level executable flow: construct the Geant4 run
 * manager, install the selected detector, register framework actions, execute
 * the user macro, and write run metadata.  Downstream examples normally only
 * need to provide a detector factory and then call Run().
 */
class Application {
public:
  /// Factory type used to create a fresh detector construction for Geant4.
  using DetectorFactory = std::function<G4VUserDetectorConstruction*()>;

  Application();
  explicit Application(DetectorFactory detectorFactory);
  ~Application() = default;

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  /// Convenience helper for examples with a default-constructible detector type.
  template <typename DetectorT>
  void SetDetector() {
    detectorFactory_ = []() -> G4VUserDetectorConstruction* {
      return new DetectorT();
    };
  }

  /// Execute the application using the command-line arguments from main().
  int Run(int argc, char** argv) const;

private:
  static int RequestedThreads(int argc, char** argv);

  DetectorFactory detectorFactory_;
};

}  // namespace G4Cosmic
