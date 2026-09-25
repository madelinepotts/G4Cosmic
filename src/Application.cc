#include "G4Cosmic/Application.hh"

#include "ActionInitialization.hh"
#include "OutputConfig.hh"
#include "OutputMessenger.hh"
#include "G4Cosmic/DetectorConstruction.hh"
#include "G4Cosmic/RunMetadata.hh"

#include "FTFP_BERT.hh"
#include "G4VModularPhysicsList.hh"
#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4UImanager.hh"
#include "G4EmParameters.hh"
#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#endif

#ifdef G4COSMIC_ENABLE_UIVIS
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#endif

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

namespace G4Cosmic {

Application::Application() = default;

Application::Application(DetectorFactory detectorFactory)
    : detectorFactory_(std::move(detectorFactory)) {}

int Application::RequestedThreads(int argc, char** argv) {
  if (argc > 2) {
    return std::max(1, std::atoi(argv[2]));
  }

  if (const char* env = std::getenv("G4COSMIC_THREADS")) {
    const int value = std::atoi(env);
    if (value > 0) return value;
  }

  const unsigned int hw = std::thread::hardware_concurrency();
  return static_cast<int>(hw > 1 ? hw - 1 : 1);
}

int Application::Run(int argc, char** argv) const {
  OutputMessenger outputMessenger;

#ifdef G4MULTITHREADED
  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::MT);
  auto* mt = dynamic_cast<G4MTRunManager*>(runManager);
  const int threads = RequestedThreads(argc, argv);
  if (mt != nullptr) mt->SetNumberOfThreads(threads);
  std::cout << "G4Cosmic Geant4 worker threads: " << threads << '\n';
#else
  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  std::cout << "G4Cosmic: Geant4 was built without multithreading; using serial mode.\n";
#endif

  G4Cosmic::RunMetadata::Instance().Begin(argc, argv, RequestedThreads(argc, argv));
  if (argc > 1) {
    G4Cosmic::RunMetadata::Instance().SetMacroFile(argv[1]);
  }

  if (!detectorFactory_) {
    std::cerr << "G4Cosmic: no detector construction was configured.\n";
    delete runManager;
    return 1;
  }

  auto* detector = detectorFactory_();
  if (auto* g4cosmicDetector = dynamic_cast<G4Cosmic::DetectorConstruction*>(detector)) {
    G4Cosmic::RunMetadata::Instance().SetDetectorMetadataContributor(
        [g4cosmicDetector](G4Cosmic::JsonWriter& json) {
          g4cosmicDetector->AppendMetadata(json);
        });
  }
  runManager->SetUserInitialization(detector);
  G4EmParameters::Instance()->SetVerbose(0);
  G4EmParameters::Instance()->SetWorkerVerbose(0);

  auto* physicsList = new FTFP_BERT();
  physicsList->SetVerboseLevel(0);
  runManager->SetUserInitialization(physicsList);
  runManager->SetUserInitialization(new ::ActionInitialization());
  runManager->Initialize();

  auto* uiManager = G4UImanager::GetUIpointer();
  if (argc > 1) {
    uiManager->ApplyCommand(G4String("/control/execute ") + argv[1]);
  } else {
#ifdef G4COSMIC_ENABLE_UIVIS
    auto* visManager = new G4VisExecutive();
    visManager->Initialize();
    auto* ui = new G4UIExecutive(argc, argv);
    uiManager->ApplyCommand("/control/execute macros/vis.mac");
    ui->SessionStart();
    delete ui;
    delete visManager;
#else
    std::cerr
        << "G4Cosmic was built with G4COSMIC_ENABLE_UIVIS=OFF. "
        << "Run with a macro file, for example: g4cosmic macros/quick.mac\n";
    delete runManager;
    return 1;
#endif
  }

  if (G4Cosmic::RunMetadata::Instance().GetPrintMacroCommands()) {
    G4Cosmic::RunMetadata::Instance().PrintMacroCommands(std::cout);
  }
  G4Cosmic::RunMetadata::Instance().WriteSidecar(std::string(OutputConfig::GetFileName()));

  delete runManager;
  return 0;
}

}  // namespace G4Cosmic
