#include "FastSimulationModel.hh"
#include <iostream>
#include "G4Electron.hh"
#include "G4GDMLParser.hh"
#include "G4Gamma.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"


FastSimulationModel::FastSimulationModel(G4String modelName, G4Region* envelope) : G4VFastSimulationModel(modelName, envelope) {
  fGarfieldPhysics = GarfieldPhysics::GetInstance();
  fGarfieldPhysics->InitializePhysics();
}

FastSimulationModel::FastSimulationModel(G4String modelName) : G4VFastSimulationModel(modelName) {
  fGarfieldPhysics = GarfieldPhysics::GetInstance();
  fGarfieldPhysics->InitializePhysics();
}

FastSimulationModel::~FastSimulationModel() 
{
}

void FastSimulationModel::WriteGeometryToGDML(G4VPhysicalVolume* physicalVolume) {
  G4GDMLParser* parser = new G4GDMLParser();
  remove("garfieldGeometry.gdml");
  parser->Write("garfieldGeometry.gdml", physicalVolume, false);
  delete parser;
}

G4bool FastSimulationModel::IsApplicable(const G4ParticleDefinition& particleType) {
  G4String particleName = particleType.GetParticleName();
  if (fGarfieldPhysics->FindParticleName(particleName, "garfield")) {
    return true;
  }
  return false;
}

G4bool FastSimulationModel::ModelTrigger(const G4FastTrack& fastTrack) {
  double ekin_MeV = fastTrack.GetPrimaryTrack()->GetKineticEnergy() / MeV;
  G4String particleName =
      fastTrack.GetPrimaryTrack()->GetParticleDefinition()->GetParticleName();
  if (fGarfieldPhysics->FindParticleNameEnergy(particleName, ekin_MeV,
                                               "garfield")) {
    return true;
  }
  return false;
}

void FastSimulationModel::DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) {

  G4ThreeVector localdir = fastTrack.GetPrimaryTrackLocalDirection();
  G4ThreeVector localpos = fastTrack.GetPrimaryTrackLocalPosition();

  double ekin_MeV = fastTrack.GetPrimaryTrack()->GetKineticEnergy() / MeV;
  double globalTime = fastTrack.GetPrimaryTrack()->GetGlobalTime();

  G4String particleName =
      fastTrack.GetPrimaryTrack()->GetParticleDefinition()->GetParticleName();

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