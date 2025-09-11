#include "FastSimulationModel.hh"
#include <iostream>
#include "G4Electron.hh"
#include "G4GDMLParser.hh"
#include "G4Gamma.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"
#include "G4coutDestination.hh" 

FastSimulationModel::FastSimulationModel(G4String modelName, G4Region* envelope) : G4VFastSimulationModel(modelName, envelope) {
  G4cout << "[LOG] FastSimulationModel -> Constructor called for model: " << modelName << G4endl;
  fGarfieldPhysics = GarfieldPhysics::GetInstance();
  fGarfieldPhysics->InitializePhysics();
}

FastSimulationModel::FastSimulationModel(G4String modelName) : G4VFastSimulationModel(modelName) {
  G4cout << "[LOG] FastSimulationModel -> Constructor called for model: " << modelName << G4endl;
  fGarfieldPhysics = GarfieldPhysics::GetInstance();
  fGarfieldPhysics->InitializePhysics();
}

FastSimulationModel::~FastSimulationModel() {}

G4bool FastSimulationModel::IsApplicable(const G4ParticleDefinition& particleType) {
  G4String particleName = particleType.GetParticleName();
  bool result = fGarfieldPhysics->FindParticleName(particleName, "garfield");
  G4cout << "[LOG] FastSimulationModel::IsApplicable -> Particle: " << particleName << " -> Applicable? " << (result ? "Yes" : "No") << G4endl;
  return result;
}

G4bool FastSimulationModel::ModelTrigger(const G4FastTrack& fastTrack) {
  double ekin_MeV = fastTrack.GetPrimaryTrack()->GetKineticEnergy() / MeV;
  G4String particleName =
      fastTrack.GetPrimaryTrack()->GetParticleDefinition()->GetParticleName();
  bool result = fGarfieldPhysics->FindParticleNameEnergy(particleName, ekin_MeV, "garfield");
  G4cout << "[LOG] FastSimulationModel::ModelTrigger -> Particle: " << particleName << ", E_kin: " << ekin_MeV << " MeV. Trigger? " << (result ? "Yes" : "No") << G4endl;
  return result;
}

void FastSimulationModel::DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) {
  G4cout << "\n========================================================================" << G4endl;
  G4cout << "[LOG] FastSimulationModel::DoIt -> TRIGGERED! Handing track to GarfieldPhysics." << G4endl;

  G4ThreeVector localdir = fastTrack.GetPrimaryTrackLocalDirection();
  G4ThreeVector localpos = fastTrack.GetPrimaryTrackLocalPosition();

  double ekin_MeV = fastTrack.GetPrimaryTrack()->GetKineticEnergy() / MeV;
  double globalTime = fastTrack.GetPrimaryTrack()->GetGlobalTime();

  G4String particleName =
      fastTrack.GetPrimaryTrack()->GetParticleDefinition()->GetParticleName();
      
  G4cout << "    -> Particle: " << particleName << ", E_kin: " << ekin_MeV << " MeV" << G4endl;
  G4cout << "    -> Position (cm): (" << localpos.x() / CLHEP::cm << ", " << localpos.y() / CLHEP::cm << ", " << localpos.z() / CLHEP::cm << ")" << G4endl;
  G4cout << "========================================================================\n" << G4endl;


  fastStep.KillPrimaryTrack();
  fastStep.ProposePrimaryTrackPathLength(0.0);

  if (particleName == "kaon+") {
    particleName = "K+";
  } else if (particleName == "kaon-") {
    particleName = "K-";
  } else if (particleName == "anti_proton") {
    particleName = "anti-proton";
  }

  fGarfieldPhysics->DoIt(
      particleName, ekin_MeV, globalTime, localpos.x() / CLHEP::cm,
      localpos.y() / CLHEP::cm, localpos.z() / CLHEP::cm,
      localdir.x(), localdir.y(), localdir.z());

  fastStep.ProposeTotalEnergyDeposited(fGarfieldPhysics->GetEnergyDeposit_MeV());

  if (!fGarfieldPhysics->GetCreateSecondariesInGeant4()) return;
  const auto& secondaryParticles = fGarfieldPhysics->GetSecondaryParticles();

  if (secondaryParticles.empty()) return;
  fastStep.SetNumberOfSecondaryTracks(secondaryParticles.size());

  G4double totalEnergySecondaries_MeV = 0.;

  for (const auto& sp : secondaryParticles) {
    G4double eKin_MeV = sp.getEkin_MeV();
    G4double time = sp.getTime();
    G4ThreeVector momentumDirection(sp.getDX(), sp.getDY(), sp.getDZ());
    G4ThreeVector position(sp.getX_mm(), sp.getY_mm(), sp.getZ_mm());
    if (sp.getParticleName() == "e-") {
      G4DynamicParticle particle(G4Electron::ElectronDefinition(),
                                 momentumDirection, eKin_MeV);
      fastStep.CreateSecondaryTrack(particle, position, time, true);
      totalEnergySecondaries_MeV += eKin_MeV;
    } else if (sp.getParticleName() == "gamma") {
       G4DynamicParticle particle(G4Gamma::GammaDefinition(),
                                  momentumDirection, eKin_MeV);
      fastStep.CreateSecondaryTrack(particle, position, time, true);
      totalEnergySecondaries_MeV += eKin_MeV;
    }
  }
}