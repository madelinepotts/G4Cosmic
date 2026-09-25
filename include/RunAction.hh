#pragma once

#include "G4UserRunAction.hh"
#include "HitRecord.hh"
#include "PrimaryRecord.hh"
#include "TrackEndRecord.hh"

#include <map>

class RunAction : public G4UserRunAction {
public:
  RunAction();
  ~RunAction() override = default;

  void BeginOfRunAction(const G4Run*) override;
  void EndOfRunAction(const G4Run*) override;

  void WriteHit(const HitRecord&) const;
  void WritePrimary(const PrimaryRecord&);
  void RecordTrackEnd(const TrackEndRecord&);

private:
  void CreatePrimaryNtuple();
  void CreateSensitiveVolumeHitNtuples();
  void CreateHitNtupleForLogicalVolume(const G4String& logicalVolumeName);
  void CreateTrackEndNtuple();

  G4int primaryNtupleId_ = -1;
  G4int trackEndNtupleId_ = -1;
  std::map<G4String, G4int> hitNtupleIdsByLogicalVolume_;
};
