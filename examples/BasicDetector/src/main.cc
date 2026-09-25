#include "G4Cosmic/Application.hh"
#include "BasicDetectorConstruction.hh"

int main(int argc, char** argv) {
  G4Cosmic::Application app;
  app.SetDetector<BasicDetectorExample::DetectorConstruction>();
  return app.Run(argc, argv);
}
