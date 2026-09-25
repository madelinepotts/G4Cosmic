#include "G4Cosmic/Application.hh"
#include "WarpTrackDetectorConstruction.hh"

int main(int argc, char** argv) {
  G4Cosmic::Application app;
  app.SetDetector<WarpTrackExample::DetectorConstruction>();
  return app.Run(argc, argv);
}
