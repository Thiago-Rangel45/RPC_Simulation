#include "Physics.hh"
#include "Analysis.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/AvalancheMicroscopic.hh"

GarfieldPhysics* GarfieldPhysics::fGarfieldPhysics = nullptr;

namespace {
  // Espessura do gap gasoso [cm]
  constexpr double kGap   = 0.2; 
  // Dimensões transversas do volume ativo [cm] - Eixos X e Z
  constexpr double kHalfX = 128.5 / 2.0;
  constexpr double kHalfZ = 165.0 / 2.0;
  // Tensão aplicada entre as placas [V]
  constexpr double kHV    = 6000.0;
  // Campo aproximado [V/cm]
  constexpr double kEy    = kHV / kGap;
}

GarfieldPhysics* GarfieldPhysics::GetInstance() {
  if (!fGarfieldPhysics) fGarfieldPhysics = new GarfieldPhysics();
  return fGarfieldPhysics;
}

void GarfieldPhysics::Dispose() {
  delete fGarfieldPhysics;
  fGarfieldPhysics = nullptr;
}

GarfieldPhysics::~GarfieldPhysics() {
  delete fMediumMagboltz;
  delete fSensor;
  delete fComponentAnalyticField;
  delete fTrackHeed;

  std::cout << "Deconstructor GarfieldPhysics" << std::endl;
}

std::string GarfieldPhysics::GetIonizationModel() { return fIonizationModel; }

void GarfieldPhysics::SetIonizationModel(std::string model, bool useDefaults) {
  if (model != "PAIPhot" && model != "PAI" && model != "Heed") {
    std::cout << "Unknown ionization model " << model << std::endl;
    std::cout << "Using PAIPhot as default model!" << std::endl;
    model = "Heed";
  }
  fIonizationModel = model;

  if (fIonizationModel == "PAIPhot" || fIonizationModel == "PAI") {
    if (useDefaults) {
      this->AddParticleName("e-", 1e-6, 1e-3, "garfield");
      this->AddParticleName("gamma", 1e-6, 1e+8, "garfield");

      this->AddParticleName("e-", 0, 1e+8, "geant4");
      this->AddParticleName("e+", 0, 1e+8, "geant4");
      this->AddParticleName("mu-", 0, 1e+8, "geant4");
      this->AddParticleName("mu+", 0, 1e+8, "geant4");
      this->AddParticleName("proton", 0, 1e+8, "geant4");
      this->AddParticleName("pi+", 0, 1e+8, "geant4");
      this->AddParticleName("pi-", 0, 1e+8, "geant4");
      this->AddParticleName("alpha", 0, 1e+8, "geant4");
      this->AddParticleName("He3", 0, 1e+8, "geant4");
      this->AddParticleName("GenericIon", 0, 1e+8, "geant4");
    }

  } else if (fIonizationModel == "Heed") {
    if (useDefaults) {
      this->AddParticleName("gamma", 1e-6, 1e+8, "garfield");
      this->AddParticleName("e-", 6e-2, 1e+7, "garfield");
      this->AddParticleName("e+", 6e-2, 1e+7, "garfield");
      this->AddParticleName("mu-", 1e+1, 1e+8, "garfield");
      this->AddParticleName("mu+", 1e+1, 1e+8, "garfield");
      this->AddParticleName("pi-", 2e+1, 1e+8, "garfield");
      this->AddParticleName("pi+", 2e+1, 1e+8, "garfield");
      this->AddParticleName("kaon-", 1e+1, 1e+8, "garfield");
      this->AddParticleName("kaon+", 1e+1, 1e+8, "garfield");
      this->AddParticleName("proton", 9.e+1, 1e+8, "garfield");
      this->AddParticleName("anti_proton", 9.e+1, 1e+8, "garfield");
      this->AddParticleName("deuteron", 2.e+2, 1e+8, "garfield");
      this->AddParticleName("alpha", 4.e+2, 1e+8, "garfield");
    }
  }
}

void GarfieldPhysics::AddParticleName(const std::string particleName,
                                      double ekin_min_MeV, double ekin_max_MeV,
                                      std::string program) {
  if (ekin_min_MeV >= ekin_max_MeV) {
    std::cout << "Ekin_min=" << ekin_min_MeV
              << " keV is larger than Ekin_max=" << ekin_max_MeV << " keV"
              << std::endl;
    return;
  }

  if (program == "garfield") {
    std::cout << "Garfield model (Heed) is applicable for G4Particle "
              << particleName << " between " << ekin_min_MeV << " MeV and "
              << ekin_max_MeV << " MeV" << std::endl;

    fMapParticlesEnergyGarfield.insert(std::make_pair(
        particleName, std::make_pair(ekin_min_MeV, ekin_max_MeV)));
  } else {
    std::cout << fIonizationModel << " is applicable for G4Particle "
              << particleName << " between " << ekin_min_MeV << " MeV and "
              << ekin_max_MeV << " MeV" << std::endl;
    fMapParticlesEnergyGeant4.insert(std::make_pair(
        particleName, std::make_pair(ekin_min_MeV, ekin_max_MeV)));
  }
}

bool GarfieldPhysics::FindParticleName(std::string name, std::string program) {
  if (program == "garfield") {
    auto it = fMapParticlesEnergyGarfield.find(name);
    if (it != fMapParticlesEnergyGarfield.end()) return true;
  } else {
    auto it = fMapParticlesEnergyGeant4.find(name);
    if (it != fMapParticlesEnergyGeant4.end()) return true;
  }
  return false;
}

bool GarfieldPhysics::FindParticleNameEnergy(std::string name, double ekin_MeV,
                                             std::string program) {
  if (program == "garfield") {
    auto it = fMapParticlesEnergyGarfield.find(name);
    if (it != fMapParticlesEnergyGarfield.end()) {
      EnergyRange_MeV range = it->second;
      if (range.first <= ekin_MeV && range.second >= ekin_MeV) {
        return true;
      }
    }
  } else {
    auto it = fMapParticlesEnergyGeant4.find(name);
    if (it != fMapParticlesEnergyGeant4.end()) {
      EnergyRange_MeV range = it->second;
      if (range.first <= ekin_MeV && range.second >= ekin_MeV) {
        return true;
      }
    }
  }
  return false;
}

double GarfieldPhysics::GetMinEnergyMeVParticle(std::string name,
                                                std::string program) {
  if (program == "garfield") {
    auto it = fMapParticlesEnergyGarfield.find(name);
    if (it != fMapParticlesEnergyGarfield.end()) {
      EnergyRange_MeV range = it->second;
      return range.first;
    }
  } else {
    auto it = fMapParticlesEnergyGeant4.find(name);
    if (it != fMapParticlesEnergyGeant4.end()) {
      EnergyRange_MeV range = it->second;
      return range.first;
    }
  }
  return -1;
}

double GarfieldPhysics::GetMaxEnergyMeVParticle(std::string name,
                                                std::string program) {
  if (program == "garfield") {
    auto it = fMapParticlesEnergyGarfield.find(name);
    if (it != fMapParticlesEnergyGarfield.end()) {
      EnergyRange_MeV range = it->second;
      return range.second;
    }
  } else {
    auto it = fMapParticlesEnergyGeant4.find(name);
    if (it != fMapParticlesEnergyGeant4.end()) {
      EnergyRange_MeV range = it->second;
      return range.second;
    }
  }
  return -1;
}

void GarfieldPhysics::InitializePhysics(){

    fMediumMagboltz = new Garfield::MediumMagboltz("ar", 70., "co2", 30.);
    fMediumMagboltz->SetTemperature(293.15);
    fMediumMagboltz->SetPressure(760.);
    fMediumMagboltz->EnableDrift();
    fMediumMagboltz->Initialise(true);
    // Verifique se este arquivo .gas existe no seu diretório de execução
    fMediumMagboltz->LoadGasFile("ar_70_co2_30_1000mbar.gas");

    fComponentAnalyticField = new Garfield::ComponentAnalyticField();
    fComponentAnalyticField->SetMedium(fMediumMagboltz);
    // Aplicando o campo no eixo Y
    fComponentAnalyticField->AddPlaneY(-0.5 * kGap,    0., "anode");
    fComponentAnalyticField->AddPlaneY(+0.5 * kGap, -kHV, "cathode");

    fSensor = new Garfield::Sensor();
    fSensor->AddComponent(fComponentAnalyticField);
    // Ajustando a área do sensor para a nova orientação (X, Y, Z)
    fSensor->SetArea(-kHalfX, -0.5 * kGap, -kHalfZ,
                      kHalfX,  0.5 * kGap,  kHalfZ);

    fTrackHeed = new Garfield::TrackHeed(fSensor);
    fTrackHeed->EnableDeltaElectronTransport();
}

void GarfieldPhysics::DoIt(std::string particleName, double ekin_MeV,
                           double time, double x_cm, double y_cm, double z_cm,
                           double dx, double dy, double dz) {
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  fEnergyDeposit = 0;
  fSecondaryParticles.clear();
  fAvalancheSize = 0;
  nsum = 0;

  Garfield::AvalancheMC drift(fSensor);
  drift.SetDistanceSteps(1.e-4);
  Garfield::AvalancheMicroscopic avalanche(fSensor);

  constexpr double yMin = -0.5 * kGap;
  constexpr double yMax = +0.5 * kGap;

  double eKin_eV = ekin_MeV * 1e+6;

  fEnergyDeposit = 0;
  if (fIonizationModel != "Heed" || particleName == "gamma") {
    Garfield::TrackHeed::Cluster cl;
    if (particleName == "gamma") {
      cl = fTrackHeed->TransportPhoton(x_cm, y_cm, z_cm, time, eKin_eV,
                                       dx, dy, dz);
    } else {
      cl = fTrackHeed->TransportDeltaElectron(x_cm, y_cm, z_cm, time, eKin_eV,
                                              dx, dy, dz);
      fEnergyDeposit = eKin_eV;
    }
    for (const auto& electron : cl.electrons) {
      if (electron.y < yMin || electron.y > yMax) continue;
      if (std::abs(electron.x) > kHalfX || std::abs(electron.z) > kHalfZ) continue;
      nsum++;
      if (particleName == "gamma") {
        fEnergyDeposit += fTrackHeed->GetW();
      }
      analysisManager->FillH3(1, electron.y * 10, electron.x * 10, electron.z * 10);
      if (createSecondariesInGeant4) {
        double newTime = electron.t;
        if (newTime < time) newTime += time;
        fSecondaryParticles.emplace_back(GarfieldParticle(
            "e-", electron.e, newTime, electron.x, electron.y, electron.z,
            electron.dx, electron.dy, electron.dz));
      }

      drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
      // Checa se o elétron alcançou o anodo
      if (!drift.GetElectrons().empty()) {
          const auto& p1 = drift.GetElectrons().front().path.back();
          const double e1 = 0.1; // eV
          avalanche.AvalancheElectron(p1.x, p1.y, p1.z, p1.t, e1, 0, 0, 0);

          int ne = 0, ni = 0;
          avalanche.GetAvalancheSize(ne, ni);
          fAvalancheSize += ne;
      }
    }
  } else {
    // Modo Heed "completo"
    fTrackHeed->SetParticle(particleName);
    fTrackHeed->SetKineticEnergy(eKin_eV);
    fTrackHeed->NewTrack(x_cm, y_cm, z_cm, time, dx, dy, dz);
    for (const auto& cluster : fTrackHeed->GetClusters()) {
      if (cluster.y < yMin || cluster.y > yMax) continue;
      if (std::abs(cluster.x) > kHalfX || std::abs(cluster.z) > kHalfZ) continue;
      nsum += cluster.electrons.size();
      fEnergyDeposit += cluster.energy;
      for (const auto& electron : cluster.electrons) {
        if (electron.y < yMin || electron.y > yMax) continue;
        if (std::abs(electron.x) > kHalfX || std::abs(electron.z) > kHalfZ) continue;

        analysisManager->FillH3(1, electron.y * 10, electron.x * 10, electron.z * 10);
        if (createSecondariesInGeant4) {
          double newTime = electron.t;
          if (newTime < time) newTime += time;
          fSecondaryParticles.emplace_back(GarfieldParticle(
              "e-", electron.e, newTime, electron.x, electron.y, electron.z,
              electron.dx, electron.dy, electron.dz));
        }

        drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
        // Checa se o elétron alcançou o anodo
        if (!drift.GetElectrons().empty()){
            const auto& p1 = drift.GetElectrons().front().path.back();
            const double e1 = 0.1; // eV
            avalanche.AvalancheElectron(p1.x, p1.y, p1.z, p1.t, e1, 0, 0, 0);

            int ne = 0, ni = 0;
            avalanche.GetAvalancheSize(ne, ni);
            fAvalancheSize += ne;
        }
      }
    }
  }
  fGain = (nsum > 0) ? (static_cast<double>(fAvalancheSize) / nsum) : 0.0;
}