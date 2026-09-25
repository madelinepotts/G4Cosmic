#include "RunAction.hh"
#include "OutputConfig.hh"

#include "G4AnalysisManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <cmath>
#include <cctype>
#include <limits>
#include <sstream>
#include <string>

namespace {

G4String SanitizeNtupleName(const G4String& name) {
  std::string out;
  out.reserve(name.size());

  for (const char c : name) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (std::isalnum(uc) || c == '_') {
      out.push_back(c);
    } else {
      out.push_back('_');
    }
  }

  if (out.empty()) {
    out = "sensitive_volume";
  }

  if (std::isdigit(static_cast<unsigned char>(out.front()))) {
    out.insert(out.begin(), '_');
  }

  return G4String(out);
}

void CreateHitColumns(G4AnalysisManager* a, G4int ntupleId) {
  a->CreateNtupleIColumn(ntupleId, "event_id");
  a->CreateNtupleIColumn(ntupleId, "track_id");
  a->CreateNtupleIColumn(ntupleId, "parent_id");
  a->CreateNtupleIColumn(ntupleId, "pdg");
  a->CreateNtupleSColumn(ntupleId, "particle_name");
  a->CreateNtupleIColumn(ntupleId, "copy_no");
  a->CreateNtupleDColumn(ntupleId, "edep_MeV");
  a->CreateNtupleDColumn(ntupleId, "time_ns");
  a->CreateNtupleDColumn(ntupleId, "x_mm");
  a->CreateNtupleDColumn(ntupleId, "y_mm");
  a->CreateNtupleDColumn(ntupleId, "z_mm");
  a->CreateNtupleSColumn(ntupleId, "physical_volume");
  a->CreateNtupleSColumn(ntupleId, "logical_volume");
}

void CreateReducedHitColumns(G4AnalysisManager* a, G4int ntupleId) {
  a->CreateNtupleIColumn(ntupleId, "event_id");
  a->CreateNtupleSColumn(ntupleId, "reduction_mode");
  a->CreateNtupleIColumn(ntupleId, "copy_no");
  a->CreateNtupleIColumn(ntupleId, "pdg");
  a->CreateNtupleSColumn(ntupleId, "particle_name");
  a->CreateNtupleSColumn(ntupleId, "physical_volume");
  a->CreateNtupleSColumn(ntupleId, "logical_volume");
  a->CreateNtupleIColumn(ntupleId, "n_steps");
  a->CreateNtupleIColumn(ntupleId, "n_tracks");
  a->CreateNtupleDColumn(ntupleId, "total_edep_MeV");
  a->CreateNtupleDColumn(ntupleId, "first_time_ns");
  a->CreateNtupleDColumn(ntupleId, "last_time_ns");
  a->CreateNtupleDColumn(ntupleId, "edep_weighted_x_mm");
  a->CreateNtupleDColumn(ntupleId, "edep_weighted_y_mm");
  a->CreateNtupleDColumn(ntupleId, "edep_weighted_z_mm");
}

}  // namespace

RunAction::RunAction() {
  auto* a = G4AnalysisManager::Instance();
  a->SetDefaultFileType("root");
  a->SetVerboseLevel(0);
  a->SetNtupleMerging(true);

  CreatePrimaryNtuple();
}

void RunAction::CreatePrimaryNtuple() {
  if (primaryNtupleId_ >= 0) {
    return;
  }

  auto* a = G4AnalysisManager::Instance();
  primaryNtupleId_ = a->CreateNtuple("primaries", "G4Cosmic generated primary-particle truth");
  a->CreateNtupleIColumn(primaryNtupleId_, "event_id");
  a->CreateNtupleIColumn(primaryNtupleId_, "primary_index");
  a->CreateNtupleIColumn(primaryNtupleId_, "pdg");
  a->CreateNtupleSColumn(primaryNtupleId_, "particle_name");
  a->CreateNtupleDColumn(primaryNtupleId_, "kinetic_energy_MeV");
  a->CreateNtupleDColumn(primaryNtupleId_, "time_s");
  a->CreateNtupleDColumn(primaryNtupleId_, "x_m");
  a->CreateNtupleDColumn(primaryNtupleId_, "y_m");
  a->CreateNtupleDColumn(primaryNtupleId_, "z_m");
  // Unit momentum direction vector. These are not momentum components.
  a->CreateNtupleDColumn(primaryNtupleId_, "momentum_unit_x");
  a->CreateNtupleDColumn(primaryNtupleId_, "momentum_unit_y");
  a->CreateNtupleDColumn(primaryNtupleId_, "momentum_unit_z");
  // Reconstructed momentum components in MeV/c, derived from PDG mass,
  // kinetic energy, and the unit momentum direction.
  a->CreateNtupleDColumn(primaryNtupleId_, "px_MeV_c");
  a->CreateNtupleDColumn(primaryNtupleId_, "py_MeV_c");
  a->CreateNtupleDColumn(primaryNtupleId_, "pz_MeV_c");
  a->FinishNtuple(primaryNtupleId_);
}

void RunAction::CreateHitNtupleForLogicalVolume(const G4String& logicalVolumeName) {
  if (logicalVolumeName.empty()) {
    return;
  }

  if (hitNtupleIdsByLogicalVolume_.find(logicalVolumeName) !=
      hitNtupleIdsByLogicalVolume_.end()) {
    return;
  }

  auto* a = G4AnalysisManager::Instance();
  const auto treeName = SanitizeNtupleName(logicalVolumeName);
  const auto ntupleId = a->CreateNtuple(
      treeName,
      "G4Cosmic sensitive-volume energy-deposition steps for logical volume " +
          logicalVolumeName);
  CreateHitColumns(a, ntupleId);
  a->FinishNtuple(ntupleId);
  hitNtupleIdsByLogicalVolume_[logicalVolumeName] = ntupleId;
}

void RunAction::CreateSensitiveVolumeHitNtuples() {
  for (const auto& logicalVolumeName : OutputConfig::GetSensitiveVolumeNames()) {
    CreateHitNtupleForLogicalVolume(logicalVolumeName);
  }
}

void RunAction::CreateReducedHitNtupleForLogicalVolume(const G4String& logicalVolumeName) {
  if (logicalVolumeName.empty()) {
    return;
  }

  if (reducedNtupleIdsByLogicalVolume_.find(logicalVolumeName) !=
      reducedNtupleIdsByLogicalVolume_.end()) {
    return;
  }

  auto* a = G4AnalysisManager::Instance();
  const auto treeName = SanitizeNtupleName(logicalVolumeName) + "_reduced";
  const auto ntupleId = a->CreateNtuple(
      treeName,
      "G4Cosmic reduced sensitive-volume hits for logical volume " +
          logicalVolumeName);
  CreateReducedHitColumns(a, ntupleId);
  a->FinishNtuple(ntupleId);
  reducedNtupleIdsByLogicalVolume_[logicalVolumeName] = ntupleId;
}

void RunAction::CreateReducedHitNtuples() {
  if (!OutputConfig::GetWriteReducedHits()) {
    return;
  }

  for (const auto& logicalVolumeName : OutputConfig::GetSensitiveVolumeNames()) {
    CreateReducedHitNtupleForLogicalVolume(logicalVolumeName);
  }
}

void RunAction::CreateTrackEndNtuple() {
  if (trackEndNtupleId_ >= 0) {
    return;
  }

  auto* a = G4AnalysisManager::Instance();
  trackEndNtupleId_ = a->CreateNtuple("track_end", "Optional generic Geant4 track termination truth");
  a->CreateNtupleIColumn(trackEndNtupleId_, "event_id");
  a->CreateNtupleIColumn(trackEndNtupleId_, "track_id");
  a->CreateNtupleIColumn(trackEndNtupleId_, "parent_id");
  a->CreateNtupleIColumn(trackEndNtupleId_, "pdg");
  a->CreateNtupleSColumn(trackEndNtupleId_, "particle_name");
  a->CreateNtupleDColumn(trackEndNtupleId_, "start_kinetic_energy_MeV");
  a->CreateNtupleDColumn(trackEndNtupleId_, "end_kinetic_energy_MeV");
  a->CreateNtupleDColumn(trackEndNtupleId_, "x_mm");
  a->CreateNtupleDColumn(trackEndNtupleId_, "y_mm");
  a->CreateNtupleDColumn(trackEndNtupleId_, "z_mm");
  a->CreateNtupleDColumn(trackEndNtupleId_, "track_length_mm");
  a->CreateNtupleDColumn(trackEndNtupleId_, "global_time_ns");
  a->CreateNtupleSColumn(trackEndNtupleId_, "end_process");
  a->CreateNtupleSColumn(trackEndNtupleId_, "material");
  a->CreateNtupleSColumn(trackEndNtupleId_, "physical_volume");
  a->CreateNtupleSColumn(trackEndNtupleId_, "logical_volume");
  a->CreateNtupleIColumn(trackEndNtupleId_, "stopped");
  a->FinishNtuple(trackEndNtupleId_);
}

void RunAction::BeginOfRunAction(const G4Run*) {
  auto* a = G4AnalysisManager::Instance();

  // Macro commands are executed after Geant4 action initialization but before
  // /run/beamOn. DetectorConstruction has also registered sensitive logical
  // volumes by this point, so create one hit tree per registered volume before
  // opening the output file.
  CreateSensitiveVolumeHitNtuples();
  CreateReducedHitNtuples();

  if (OutputConfig::GetWriteTrackEnd()) {
    CreateTrackEndNtuple();
  }

  a->SetFileName(OutputConfig::GetFileName());
  a->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*) {
  FlushReducedHits();
  auto* a = G4AnalysisManager::Instance();
  a->Write();
  a->CloseFile();
}

void RunAction::WriteHit(const HitRecord& h) const {
  auto it = hitNtupleIdsByLogicalVolume_.find(h.logicalVolumeName);
  if (it == hitNtupleIdsByLogicalVolume_.end()) {
    G4cerr << "G4Cosmic warning: no hit tree is registered for logical volume '"
           << h.logicalVolumeName << "'. Did you call RegisterSensitiveVolume() for it?"
           << G4endl;
    return;
  }

  const auto ntupleId = it->second;
  auto* a = G4AnalysisManager::Instance();
  a->FillNtupleIColumn(ntupleId, 0, h.eventID);
  a->FillNtupleIColumn(ntupleId, 1, h.trackID);
  a->FillNtupleIColumn(ntupleId, 2, h.parentID);
  a->FillNtupleIColumn(ntupleId, 3, h.pdg);
  a->FillNtupleSColumn(ntupleId, 4, h.particleName);
  a->FillNtupleIColumn(ntupleId, 5, h.copyNo);
  a->FillNtupleDColumn(ntupleId, 6, h.edep / MeV);
  a->FillNtupleDColumn(ntupleId, 7, h.time / ns);
  a->FillNtupleDColumn(ntupleId, 8, h.position.x() / mm);
  a->FillNtupleDColumn(ntupleId, 9, h.position.y() / mm);
  a->FillNtupleDColumn(ntupleId, 10, h.position.z() / mm);
  a->FillNtupleSColumn(ntupleId, 11, h.physicalVolumeName);
  a->FillNtupleSColumn(ntupleId, 12, h.logicalVolumeName);
  a->AddNtupleRow(ntupleId);

  AccumulateReducedHit(h);
}

G4String RunAction::MakeReducedHitKey(const HitRecord& h) const {
  const auto& mode = OutputConfig::GetReducedHitMode();
  std::ostringstream key;

  if (mode == "particleCopyNo") {
    key << h.copyNo << "|" << h.pdg;
    return key.str();
  }

  if (mode == "physicalVolume") {
    key << h.physicalVolumeName << "|" << h.copyNo;
    return key.str();
  }

  key << h.copyNo;
  return key.str();
}

void RunAction::AccumulateReducedHit(const HitRecord& h) const {
  if (!OutputConfig::GetWriteReducedHits()) {
    return;
  }

  if (reducedNtupleIdsByLogicalVolume_.find(h.logicalVolumeName) ==
      reducedNtupleIdsByLogicalVolume_.end()) {
    return;
  }

  auto& reducedForVolume = reducedHitsByLogicalVolume_[h.logicalVolumeName];
  auto& acc = reducedForVolume[MakeReducedHitKey(h)];

  if (acc.nSteps == 0) {
    acc.eventID = h.eventID;
    acc.copyNo = h.copyNo;
    acc.pdg = 0;
    acc.particleName = "";
    acc.physicalVolumeName = "";
    acc.logicalVolumeName = h.logicalVolumeName;
    acc.firstTime = h.time;
    acc.lastTime = h.time;

    const auto& mode = OutputConfig::GetReducedHitMode();
    if (mode == "particleCopyNo") {
      acc.pdg = h.pdg;
      acc.particleName = h.particleName;
    }
    if (mode == "physicalVolume") {
      acc.physicalVolumeName = h.physicalVolumeName;
    }
  }

  acc.nSteps += 1;
  acc.trackIDs.insert(h.trackID);
  acc.totalEdep += h.edep;
  acc.edepWeightedPositionSum += h.edep * h.position;
  if (h.time < acc.firstTime) {
    acc.firstTime = h.time;
  }
  if (h.time > acc.lastTime) {
    acc.lastTime = h.time;
  }
}

void RunAction::WriteReducedHitRow(G4int ntupleId, const ReducedHitAccumulator& acc) const {
  auto* a = G4AnalysisManager::Instance();
  const auto weightedPosition =
      acc.totalEdep > 0.0 ? acc.edepWeightedPositionSum / acc.totalEdep : G4ThreeVector();

  a->FillNtupleIColumn(ntupleId, 0, acc.eventID);
  a->FillNtupleSColumn(ntupleId, 1, OutputConfig::GetReducedHitMode());
  a->FillNtupleIColumn(ntupleId, 2, acc.copyNo);
  a->FillNtupleIColumn(ntupleId, 3, acc.pdg);
  a->FillNtupleSColumn(ntupleId, 4, acc.particleName);
  a->FillNtupleSColumn(ntupleId, 5, acc.physicalVolumeName);
  a->FillNtupleSColumn(ntupleId, 6, acc.logicalVolumeName);
  a->FillNtupleIColumn(ntupleId, 7, acc.nSteps);
  a->FillNtupleIColumn(ntupleId, 8, static_cast<G4int>(acc.trackIDs.size()));
  a->FillNtupleDColumn(ntupleId, 9, acc.totalEdep / MeV);
  a->FillNtupleDColumn(ntupleId, 10, acc.firstTime / ns);
  a->FillNtupleDColumn(ntupleId, 11, acc.lastTime / ns);
  a->FillNtupleDColumn(ntupleId, 12, weightedPosition.x() / mm);
  a->FillNtupleDColumn(ntupleId, 13, weightedPosition.y() / mm);
  a->FillNtupleDColumn(ntupleId, 14, weightedPosition.z() / mm);
  a->AddNtupleRow(ntupleId);
}

void RunAction::FlushReducedHits() const {
  if (!OutputConfig::GetWriteReducedHits()) {
    reducedHitsByLogicalVolume_.clear();
    return;
  }

  for (const auto& volumePair : reducedHitsByLogicalVolume_) {
    const auto ntupleIt = reducedNtupleIdsByLogicalVolume_.find(volumePair.first);
    if (ntupleIt == reducedNtupleIdsByLogicalVolume_.end()) {
      continue;
    }

    for (const auto& hitPair : volumePair.second) {
      WriteReducedHitRow(ntupleIt->second, hitPair.second);
    }
  }

  reducedHitsByLogicalVolume_.clear();
}

void RunAction::WritePrimary(const PrimaryRecord& p) {
  if (primaryNtupleId_ < 0) {
    return;
  }

  auto* a = G4AnalysisManager::Instance();
  a->FillNtupleIColumn(primaryNtupleId_, 0, p.eventID);
  a->FillNtupleIColumn(primaryNtupleId_, 1, p.primaryIndex);
  a->FillNtupleIColumn(primaryNtupleId_, 2, p.pdg);
  a->FillNtupleSColumn(primaryNtupleId_, 3, p.particleName);
  a->FillNtupleDColumn(primaryNtupleId_, 4, p.kineticEnergy / MeV);
  a->FillNtupleDColumn(primaryNtupleId_, 5, p.time / s);
  a->FillNtupleDColumn(primaryNtupleId_, 6, p.position.x() / m);
  a->FillNtupleDColumn(primaryNtupleId_, 7, p.position.y() / m);
  a->FillNtupleDColumn(primaryNtupleId_, 8, p.position.z() / m);
  const auto unitMomentum = p.direction.unit();

  G4double momentumMagnitude = 0.0;
  if (auto* definition = G4ParticleTable::GetParticleTable()->FindParticle(p.pdg)) {
    const G4double mass = definition->GetPDGMass();
    const G4double totalEnergy = p.kineticEnergy + mass;
    const G4double p2 = totalEnergy * totalEnergy - mass * mass;
    momentumMagnitude = p2 > 0.0 ? std::sqrt(p2) : 0.0;
  }

  const auto momentum = momentumMagnitude * unitMomentum;

  a->FillNtupleDColumn(primaryNtupleId_, 9, unitMomentum.x());
  a->FillNtupleDColumn(primaryNtupleId_, 10, unitMomentum.y());
  a->FillNtupleDColumn(primaryNtupleId_, 11, unitMomentum.z());
  a->FillNtupleDColumn(primaryNtupleId_, 12, momentum.x() / MeV);
  a->FillNtupleDColumn(primaryNtupleId_, 13, momentum.y() / MeV);
  a->FillNtupleDColumn(primaryNtupleId_, 14, momentum.z() / MeV);
  a->AddNtupleRow(primaryNtupleId_);
}

void RunAction::RecordTrackEnd(const TrackEndRecord& r) {
  if (!OutputConfig::GetWriteTrackEnd() || trackEndNtupleId_ < 0) {
    return;
  }

  auto* a = G4AnalysisManager::Instance();
  a->FillNtupleIColumn(trackEndNtupleId_, 0, r.eventID);
  a->FillNtupleIColumn(trackEndNtupleId_, 1, r.trackID);
  a->FillNtupleIColumn(trackEndNtupleId_, 2, r.parentID);
  a->FillNtupleIColumn(trackEndNtupleId_, 3, r.pdg);
  a->FillNtupleSColumn(trackEndNtupleId_, 4, r.particleName);
  a->FillNtupleDColumn(trackEndNtupleId_, 5, r.startKineticEnergyMeV);
  a->FillNtupleDColumn(trackEndNtupleId_, 6, r.endKineticEnergyMeV);
  a->FillNtupleDColumn(trackEndNtupleId_, 7, r.xMm);
  a->FillNtupleDColumn(trackEndNtupleId_, 8, r.yMm);
  a->FillNtupleDColumn(trackEndNtupleId_, 9, r.zMm);
  a->FillNtupleDColumn(trackEndNtupleId_, 10, r.trackLengthMm);
  a->FillNtupleDColumn(trackEndNtupleId_, 11, r.globalTimeNs);
  a->FillNtupleSColumn(trackEndNtupleId_, 12, r.endProcess);
  a->FillNtupleSColumn(trackEndNtupleId_, 13, r.material);
  a->FillNtupleSColumn(trackEndNtupleId_, 14, r.physicalVolume);
  a->FillNtupleSColumn(trackEndNtupleId_, 15, r.logicalVolume);
  a->FillNtupleIColumn(trackEndNtupleId_, 16, r.stopped ? 1 : 0);
  a->AddNtupleRow(trackEndNtupleId_);
}
