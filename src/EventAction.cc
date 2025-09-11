#include "EventAction.hh"
#include <iomanip>
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "Analysis.hh"
#include "Physics.hh"
#include "RunAction.hh"
#include "Randomize.hh"


EventAction::~EventAction()
{
}

void EventAction::BeginOfEventAction(const G4Event* /*event*/) {
  fEnergyAbs = 0.;
  fEnergyGas = 0.;
  fTrackLAbs = 0.;
  fAvalancheSize = 0.;
  fGain = 0.;

  GarfieldPhysics* garfieldPhysics = GarfieldPhysics::GetInstance();
  garfieldPhysics->Clear();
}

void EventAction::EndOfEventAction(const G4Event* event) {

  GarfieldPhysics* garfieldPhysics = GarfieldPhysics::GetInstance();
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  fAvalancheSize = garfieldPhysics->GetAvalancheSize();
  fGain = garfieldPhysics->GetGain();

  analysisManager->FillH1(1, fEnergyAbs);
  analysisManager->FillH1(2, fTrackLAbs);
  analysisManager->FillH1(3, fEnergyGas);
  analysisManager->FillH1(4, fAvalancheSize);
  analysisManager->FillH1(5, fGain);

  analysisManager->FillNtupleDColumn(0, fEnergyAbs);
  analysisManager->FillNtupleDColumn(1, fTrackLAbs);
  analysisManager->FillNtupleDColumn(2, fEnergyGas);
  analysisManager->FillNtupleDColumn(3, fAvalancheSize);
  analysisManager->FillNtupleDColumn(4, fGain);

  G4int eventID = event->GetEventID();
  G4int printModulo = G4RunManager::GetRunManager()->GetPrintProgress();
  if ((printModulo > 0) && (eventID % printModulo == 0)) {
    G4cout << "---> End of event: " << eventID << G4endl;

    G4cout << "   Absorber: total energy: " << std::setw(7)
           << G4BestUnit(fEnergyAbs, "Energy")
           << "       total track length: " << std::setw(7)
           << G4BestUnit(fTrackLAbs, "Length") << G4endl;

    G4cout << "        Gas: total energy: " << std::setw(7)
           << G4BestUnit(fEnergyGas, "Energy")
           << "       avalanche size: " << fAvalancheSize
           << "       gain: " << fGain << G4endl;
  }
}