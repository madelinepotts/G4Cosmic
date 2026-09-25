#pragma once

#include "G4String.hh"
#include "G4ThreeVector.hh"
#include "G4UserRunAction.hh"
#include "HitRecord.hh"
#include "PrimaryRecord.hh"
#include "TrackEndRecord.hh"
#include "globals.hh"

#include <map>
#include <set>

class RunAction : public G4UserRunAction {
public:
  RunAction();
  ~RunAction() override = default;

  void BeginOfRunAction(const G4Run*) override;
  void EndOfRunAction(const G4Run*) override;

  void WriteHit(const HitRecord&) const;
  void WritePrimary(const PrimaryRecord&);
  void RecordTrackEnd(const TrackEndRecord&);

  // Called by EventAction at the end of each event. EndOfRunAction also calls
  // this once as a safety net in case no EventAction ran.
  void FlushReducedHits() const;

private:
  struct ReducedHitAccumulator {
    G4int eventID = -1;
    G4int copyNo = -1;
    G4int pdg = 0;
    G4String particleName;
    G4String physicalVolumeName;
    G4String logicalVolumeName;

    G4int nSteps = 0;
    std::set<G4int> trackIDs;
    G4double totalEdep = 0.0;
    G4double firstTime = 0.0;
    G4double lastTime = 0.0;
    G4ThreeVector edepWeightedPositionSum;
  };

  void CreatePrimaryNtuple();
  void CreateSensitiveVolumeHitNtuples();
  void CreateHitNtupleForLogicalVolume(const G4String& logicalVolumeName);
  void CreateReducedHitNtuples();
  void CreateReducedHitNtupleForLogicalVolume(const G4String& logicalVolumeName);
  void CreateTrackEndNtuple();

  void AccumulateReducedHit(const HitRecord&) const;
  G4String MakeReducedHitKey(const HitRecord&) const;
  void WriteReducedHitRow(G4int ntupleId, const ReducedHitAccumulator& acc) const;

  G4int primaryNtupleId_ = -1;
  G4int trackEndNtupleId_ = -1;
  std::map<G4String, G4int> hitNtupleIdsByLogicalVolume_;
  std::map<G4String, G4int> reducedNtupleIdsByLogicalVolume_;
  mutable std::map<G4String, std::map<G4String, ReducedHitAccumulator>> reducedHitsByLogicalVolume_;
};
