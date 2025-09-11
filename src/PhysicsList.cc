#include "PhysicsList.hh" 
#include "G4EmConfigurator.hh"
#include "G4FastSimulationManagerProcess.hh"
#include "G4LossTableManager.hh"
#include "G4PAIModel.hh"
#include "G4PAIPhotModel.hh"
#include "G4ProcessManager.hh"
#include "G4ProductionCuts.hh"
#include "G4RegionStore.hh"
#include "QGSP_BERT_HP.hh"
#include "G4SystemOfUnits.hh"
#include "Physics.hh" 

PhysicsList::PhysicsList() : G4VModularPhysicsList() {
    SetVerboseLevel(0);
    defaultCutValue = 1 * CLHEP::mm;

    QGSP_BERT_HP* physicsList = new QGSP_BERT_HP;
    for (G4int i = 0;; ++i) {
        G4VPhysicsConstructor* elem =
            const_cast<G4VPhysicsConstructor*>(physicsList->GetPhysics(i));
        if (!elem) break;
        RegisterPhysics(elem);
    }

    // ==============================================================================
    // >> CORREÇÃO FINAL <<
    // Inicializa o singleton do Garfield e define o modelo de ionização.
    // Isso garante que a lista de partículas seja preenchida ANTES da física ser construída.
    GarfieldPhysics* garfieldPhysics = GarfieldPhysics::GetInstance();
    garfieldPhysics->SetIonizationModel("PAIPhot");
    // ==============================================================================
}

PhysicsList::~PhysicsList() {}

void PhysicsList::AddParameterisation() {
    G4cout << "[LOG] PhysicsList::AddParameterisation -> Setting up Garfield parameterisation..." << G4endl;
    GarfieldPhysics* garfieldPhysics = GarfieldPhysics::GetInstance();
    std::string ionizationModel = garfieldPhysics->GetIonizationModel();

    auto fastSimProcess_garfield = new G4FastSimulationManagerProcess("G4FSMP_garfield");

    auto theParticleIterator = GetParticleIterator();
    theParticleIterator->reset();

    while ((*theParticleIterator)()) {
        G4ParticleDefinition* particle = theParticleIterator->value();
        G4ProcessManager* pmanager = particle->GetProcessManager();
        G4EmConfigurator* config = G4LossTableManager::Instance()->EmConfigurator();
        
        auto particleName = particle->GetParticleName();

        if (garfieldPhysics->FindParticleName(particleName, "garfield")) {
            G4cout << "[LOG] PhysicsList -> Attaching Garfield FastSim process to: " << particleName << G4endl;
            pmanager->AddDiscreteProcess(fastSimProcess_garfield);
        }

        if (garfieldPhysics->FindParticleName(particleName, "geant4")) {
            double eMin = MeV * garfieldPhysics->GetMinEnergyMeVParticle(particleName, "geant4");
            double eMax = MeV * garfieldPhysics->GetMaxEnergyMeVParticle(particleName, "geant4");

            if (ionizationModel == "PAI") {
                G4cout << "[LOG] PhysicsList -> Attaching PAI model to: " << particleName << G4endl;
                G4PAIModel* pai = new G4PAIModel(particle, "G4PAIModel");
                if (particleName == "e-" || particleName == "e+") {
                    config->SetExtraEmModel(particleName, "eIoni", pai, "RegionGarfield", eMin, eMax, pai);
                } else if (particleName == "mu-" || particleName == "mu+") {
                    config->SetExtraEmModel(particleName, "muIoni", pai, "RegionGarfield", eMin, eMax, pai);
                } else if (particleName == "proton" || particleName == "pi+" || particleName == "pi-") {
                    config->SetExtraEmModel(particleName, "hIoni", pai, "RegionGarfield", eMin, eMax, pai);
                } else if (particleName == "alpha" || particleName == "He3" || particleName == "GenericIon") {
                    config->SetExtraEmModel(particleName, "ionIoni", pai, "RegionGarfield", eMin, eMax, pai);
                }
            } else if (ionizationModel == "PAIPhot") {
                G4cout << "[LOG] PhysicsList -> Attaching PAIPhot model to: " << particleName << G4endl;
                G4PAIPhotModel* paiPhot = new G4PAIPhotModel(particle, "G4PAIModel");
                if (particleName == "e-" || particleName == "e+") {
                    config->SetExtraEmModel(particleName, "eIoni", paiPhot, "RegionGarfield", eMin, eMax, paiPhot);
                } else if (particleName == "mu-" || particleName == "mu+") {
                    config->SetExtraEmModel(particleName, "muIoni", paiPhot, "RegionGarfield", eMin, eMax, paiPhot);
                } else if (particleName == "proton" || particleName == "pi+" || particleName == "pi-") {
                    config->SetExtraEmModel(particleName, "hIoni", paiPhot, "RegionGarfield", eMin, eMax, paiPhot);
                } else if (particleName == "alpha" || particleName == "He3" || particleName == "GenericIon") {
                    config->SetExtraEmModel(particleName, "ionIoni", paiPhot, "RegionGarfield", eMin, eMax, paiPhot);
                }
            }
        }
    }
}

void PhysicsList::SetCuts() {
    G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(100. * eV, 100. * TeV);
    SetCutsWithDefault();
    G4Region* region = G4RegionStore::GetInstance()->GetRegion("RegionGarfield");
    G4ProductionCuts* cuts = new G4ProductionCuts();
    cuts->SetProductionCut(1 * um, G4ProductionCuts::GetIndex("gamma"));
    cuts->SetProductionCut(1 * um, G4ProductionCuts::GetIndex("e-"));
    cuts->SetProductionCut(1 * um, G4ProductionCuts::GetIndex("e+"));
    if (region) {
        G4cout << "[LOG] PhysicsList::SetCuts -> Applying special production cuts to RegionGarfield." << G4endl;
        region->SetProductionCuts(cuts);
    } else {
        G4cout << "[LOG] PhysicsList::SetCuts -> ERRO: RegionGarfield não encontrada!" << G4endl;
    }
    DumpCutValuesTable();
}

void PhysicsList::ConstructParticle() {
    G4VModularPhysicsList::ConstructParticle();
}

void PhysicsList::ConstructProcess() {
    G4VModularPhysicsList::ConstructProcess();
    AddParameterisation();
}