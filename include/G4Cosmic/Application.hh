#pragma once

namespace G4Cosmic {

class Application {
public:
  Application() = default;
  ~Application() = default;

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  int Run(int argc, char** argv) const;

private:
  static int RequestedThreads(int argc, char** argv);
};

}  // namespace G4Cosmic
