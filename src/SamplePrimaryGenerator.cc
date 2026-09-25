#include "G4Cosmic/SamplePrimaryGenerator.hh"

#include "PrimaryRecord.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4LogicalVolume.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4TransportationManager.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"
#include "G4ios.hh"
#include "Randomize.hh"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>

namespace {

bool WildcardMatches(const std::string& pattern, const std::string& text)
{
    std::size_t p = 0;
    std::size_t t = 0;
    std::size_t star = std::string::npos;
    std::size_t match = 0;

    while (t < text.size()) {
        if (p < pattern.size() &&
            (pattern[p] == '?' || pattern[p] == text[t])) {
            ++p;
            ++t;
        }
        else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            match = t;
        }
        else if (star != std::string::npos) {
            p = star + 1;
            t = ++match;
        }
        else {
            return false;
        }
    }

    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }

    return p == pattern.size();
}

G4double UniformBetween(G4double lo, G4double hi)
{
    return lo + G4UniformRand() * (hi - lo);
}

} // namespace

namespace G4Cosmic {

SamplePrimaryGenerator::SamplePrimaryGenerator()
    : gun_(std::make_unique<G4ParticleGun>(1))
{
    maxTheta_ = 30.0 * deg;
    ConfigureMessenger();
}

SamplePrimaryGenerator::~SamplePrimaryGenerator() = default;

void SamplePrimaryGenerator::ConfigureMessenger()
{
    messenger_ = std::make_unique<G4GenericMessenger>(
        this, "/g4cosmic/sample/", "Randomized single-primary source controls");
    messenger_->DeclareMethod("particle", &SamplePrimaryGenerator::SetParticle,
                              "Particle name: mu-, mu+, proton, neutron, etc.");
    messenger_->DeclarePropertyWithUnit("minEnergy", "MeV", minEnergy_,
                                        "Minimum kinetic energy.");
    messenger_->DeclarePropertyWithUnit("maxEnergy", "MeV", maxEnergy_,
                                        "Maximum kinetic energy.");
    messenger_->DeclarePropertyWithUnit("maxTheta", "deg", maxTheta_,
                                        "Maximum zenith angle from vertically downward.");
    messenger_->DeclareProperty("logEnergy", logEnergy_,
                                "Sample kinetic energy log-uniformly (1) or uniformly (0).");
    messenger_->DeclareMethod("positionMode", &SamplePrimaryGenerator::SetPositionMode,
                              "Source-position mode inside the selected logical volume: volume or surface.");
    messenger_->DeclareMethod("sourceVolume", &SamplePrimaryGenerator::SetSourceVolume,
                              "Logical-volume name or wildcard to sample. Required for /g4cosmic/source sample.");
    messenger_->DeclareProperty("maxPositionTrials", maxPositionTrials_,
                                "Maximum rejection-sampling attempts for each sampled source position.");
    messenger_->DeclareProperty("maxVolumeTrials", maxPositionTrials_,
                                "Deprecated alias for maxPositionTrials.");
}

void SamplePrimaryGenerator::SetParticle(const G4String& particle)
{
    auto* definition = G4ParticleTable::GetParticleTable()->FindParticle(particle);
    if (definition == nullptr) {
        G4cout << "G4Cosmic: unknown sample particle '" << particle << "'." << G4endl;
        return;
    }
    particle_ = particle;
}

void SamplePrimaryGenerator::SetPositionMode(const G4String& mode)
{
    if (mode != "volume" && mode != "surface") {
        G4cout << "G4Cosmic: /g4cosmic/sample/positionMode must be 'volume' or 'surface'." << G4endl;
        return;
    }
    positionMode_ = mode;
}

void SamplePrimaryGenerator::SetSourceVolume(const G4String& logicalVolumeName)
{
    sourceVolume_ = logicalVolumeName;
    cachedSourceVolume_ = "";
    sourcePlacements_.clear();
}

void SamplePrimaryGenerator::RefreshSourceVolumes() const
{
    if (sourceVolume_.empty()) {
        throw std::runtime_error(
            "/g4cosmic/source sample requires /g4cosmic/sample/sourceVolume. "
            "Use a logical-volume name such as SourceButtonLV or a wildcard pattern.");
    }

    if (cachedSourceVolume_ == sourceVolume_ && !sourcePlacements_.empty()) {
        return;
    }

    sourcePlacements_.clear();
    cachedSourceVolume_ = sourceVolume_;

    auto* navigator =
        G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
    auto* world = navigator ? navigator->GetWorldVolume() : nullptr;

    if (world == nullptr) {
        throw std::runtime_error(
            "Unable to resolve Geant4 world volume for sample/sourceVolume lookup. "
            "Make sure /run/initialize has been called before /run/beamOn.");
    }

    const std::string pattern = sourceVolume_;

    std::function<void(const G4VPhysicalVolume*,
                       const G4RotationMatrix&,
                       const G4ThreeVector&)> visit;

    visit = [&](const G4VPhysicalVolume* physicalVolume,
                const G4RotationMatrix& localToWorldRotation,
                const G4ThreeVector& localToWorldTranslation) {
        if (physicalVolume == nullptr) {
            return;
        }

        const auto* logicalVolume = physicalVolume->GetLogicalVolume();
        if (logicalVolume == nullptr) {
            return;
        }

        if (WildcardMatches(pattern, logicalVolume->GetName())) {
            const auto* solid = logicalVolume->GetSolid();
            if (solid != nullptr) {
                SourcePlacement placement;
                placement.physicalVolumeName = physicalVolume->GetName();
                placement.logicalVolumeName = logicalVolume->GetName();
                placement.solid = solid;
                placement.localToWorldRotation = localToWorldRotation;
                placement.localToWorldTranslation = localToWorldTranslation;
                solid->BoundingLimits(placement.localMin, placement.localMax);
                sourcePlacements_.push_back(placement);
            }
        }

        const auto daughterCount = logicalVolume->GetNoDaughters();
        for (G4int i = 0; i < daughterCount; ++i) {
            const auto* daughter = logicalVolume->GetDaughter(i);
            if (daughter == nullptr) {
                continue;
            }

            G4RotationMatrix daughterRotation = localToWorldRotation;
            if (const auto* objectRotation = daughter->GetObjectRotation()) {
                daughterRotation *= *objectRotation;
            }

            const G4ThreeVector daughterTranslation =
                localToWorldRotation * daughter->GetObjectTranslation() +
                localToWorldTranslation;

            visit(daughter, daughterRotation, daughterTranslation);
        }
    };

    visit(world, G4RotationMatrix(), G4ThreeVector());

    if (sourcePlacements_.empty()) {
        throw std::runtime_error(
            "No physical placements were found for sample source logical volume pattern '" +
            std::string(sourceVolume_) +
            "'. Use an existing G4LogicalVolume name, such as a button/source volume, "
            "or a wildcard pattern.");
    }

    G4cout << "G4Cosmic: sample source uses "
           << sourcePlacements_.size()
           << " physical placement(s) matching logical volume pattern '"
           << sourceVolume_ << "' with positionMode '"
           << positionMode_ << "'." << G4endl;
}

G4ThreeVector SamplePrimaryGenerator::SampleLocalPointInside(
    const SourcePlacement& placement) const
{
    for (G4int trial = 0; trial < maxPositionTrials_; ++trial) {
        const G4ThreeVector local(
            UniformBetween(placement.localMin.x(), placement.localMax.x()),
            UniformBetween(placement.localMin.y(), placement.localMax.y()),
            UniformBetween(placement.localMin.z(), placement.localMax.z()));

        if (placement.solid->Inside(local) != kOutside) {
            return local;
        }
    }

    throw std::runtime_error(
        "Failed to sample a point inside logical volume '" +
        std::string(placement.logicalVolumeName) +
        "'. Increase /g4cosmic/sample/maxPositionTrials or choose a less pathological source solid.");
}

G4ThreeVector SamplePrimaryGenerator::SampleIsotropicDirection() const
{
    const double z = 2.0 * G4UniformRand() - 1.0;
    const double phi = 2.0 * CLHEP::pi * G4UniformRand();
    const double r = std::sqrt(std::max(0.0, 1.0 - z * z));
    return G4ThreeVector(r * std::cos(phi), r * std::sin(phi), z);
}

G4ThreeVector SamplePrimaryGenerator::SampleLocalSurfacePoint(
    const SourcePlacement& placement) const
{
    const G4ThreeVector inside = SampleLocalPointInside(placement);
    const G4ThreeVector direction = SampleIsotropicDirection();

    const G4double dx = direction.x();
    const G4double dy = direction.y();
    const G4double dz = direction.z();
    G4double tMax = DBL_MAX;

    if (dx > 0.0) {
        tMax = std::min(tMax, (placement.localMax.x() - inside.x()) / dx);
    } else if (dx < 0.0) {
        tMax = std::min(tMax, (placement.localMin.x() - inside.x()) / dx);
    }

    if (dy > 0.0) {
        tMax = std::min(tMax, (placement.localMax.y() - inside.y()) / dy);
    } else if (dy < 0.0) {
        tMax = std::min(tMax, (placement.localMin.y() - inside.y()) / dy);
    }

    if (dz > 0.0) {
        tMax = std::min(tMax, (placement.localMax.z() - inside.z()) / dz);
    } else if (dz < 0.0) {
        tMax = std::min(tMax, (placement.localMin.z() - inside.z()) / dz);
    }

    if (tMax <= 0.0 || tMax == DBL_MAX) {
        return inside;
    }

    G4double lo = 0.0;
    G4double hi = tMax;

    // The bounding-box endpoint may still be inside for unusual solids. Expand
    // very slightly until Geant4 reports outside, then bisect to the boundary.
    G4ThreeVector outside = inside + hi * direction;
    for (G4int expand = 0; expand < 8 && placement.solid->Inside(outside) != kOutside; ++expand) {
        hi *= 1.25;
        outside = inside + hi * direction;
    }

    for (G4int iter = 0; iter < 64; ++iter) {
        const G4double mid = 0.5 * (lo + hi);
        const G4ThreeVector point = inside + mid * direction;
        if (placement.solid->Inside(point) == kOutside) {
            hi = mid;
        } else {
            lo = mid;
        }
    }

    return inside + lo * direction;
}

G4ThreeVector SamplePrimaryGenerator::SampleVolumePosition() const
{
    RefreshSourceVolumes();

    if (maxPositionTrials_ <= 0) {
        throw std::runtime_error("/g4cosmic/sample/maxPositionTrials must be > 0.");
    }

    const auto placementIndex = static_cast<std::size_t>(
        std::min<G4int>(
            static_cast<G4int>(sourcePlacements_.size() - 1),
            static_cast<G4int>(G4UniformRand() * sourcePlacements_.size())));

    const auto& placement = sourcePlacements_.at(placementIndex);

    const G4ThreeVector local =
        (positionMode_ == "surface")
            ? SampleLocalSurfacePoint(placement)
            : SampleLocalPointInside(placement);

    return placement.localToWorldRotation * local +
           placement.localToWorldTranslation;
}

G4ThreeVector SamplePrimaryGenerator::SampleDirection() const
{
    if (maxTheta_ < 0.0 || maxTheta_ >= 90.0 * deg) {
        throw std::runtime_error("Invalid /g4cosmic/sample/maxTheta angular range.");
    }

    const double sinMax = std::sin(maxTheta_);
    const double sinTheta = std::sqrt(G4UniformRand()) * sinMax;
    const double theta = std::asin(sinTheta);
    const double phi = 2.0 * CLHEP::pi * G4UniformRand();

    return G4ThreeVector(
        std::sin(theta) * std::cos(phi),
        std::sin(theta) * std::sin(phi),
        -std::cos(theta));
}

G4double SamplePrimaryGenerator::SampleKineticEnergy() const
{
    if (minEnergy_ <= 0.0 || maxEnergy_ < minEnergy_) {
        throw std::runtime_error("Invalid /g4cosmic/sample energy range.");
    }

    if (logEnergy_ != 0 && maxEnergy_ > minEnergy_) {
        const double logMin = std::log(minEnergy_);
        const double logMax = std::log(maxEnergy_);
        return std::exp(logMin + G4UniformRand() * (logMax - logMin));
    }

    return minEnergy_ + G4UniformRand() * (maxEnergy_ - minEnergy_);
}

void SamplePrimaryGenerator::GeneratePrimaries(G4Event* event, RunAction* runAction)
{
    auto* definition =
        G4ParticleTable::GetParticleTable()->FindParticle(particle_);
    if (definition == nullptr) {
        throw std::runtime_error("Unknown /g4cosmic/sample particle.");
    }

    const G4double kineticEnergy = SampleKineticEnergy();
    const G4ThreeVector position = SampleVolumePosition();
    const G4ThreeVector direction = SampleDirection();

    gun_->SetParticleDefinition(definition);
    gun_->SetParticleEnergy(kineticEnergy);
    gun_->SetParticlePosition(position);
    gun_->SetParticleMomentumDirection(direction);
    gun_->SetParticleTime(0.0);

    if (runAction != nullptr) {
        PrimaryRecord primary;
        primary.eventID = event->GetEventID();
        primary.primaryIndex = 0;
        primary.pdg = definition->GetPDGEncoding();
        primary.particleName = definition->GetParticleName();
        primary.kineticEnergy = kineticEnergy;
        primary.time = 0.0;
        primary.position = position;
        primary.direction = direction;
        runAction->WritePrimary(primary);
    }

    gun_->GeneratePrimaryVertex(event);
}

} // namespace G4Cosmic
