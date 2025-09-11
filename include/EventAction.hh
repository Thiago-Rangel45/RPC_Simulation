#ifndef EventAction_h
#define EventAction_h 

#include "G4UserEventAction.hh"
#include "globals.hh"

class EventAction : public G4UserEventAction {
 public:
  EventAction() = default;
  virtual ~EventAction();

  void BeginOfEventAction(const G4Event* event) override;
  void EndOfEventAction(const G4Event* event) override;

  void AddAbs(G4double de, G4double dl);
  void AddGas(G4double de);

 private:
  G4double fEnergyAbs = 0.;
  G4double fEnergyGas = 0.;
  G4double fTrackLAbs = 0.;
  G4double fAvalancheSize = 0.;
  G4double fGain = 0.;
};

inline void EventAction::AddAbs(G4double de, G4double dl) {
  fEnergyAbs += de;
  fTrackLAbs += dl;
}
inline void EventAction::AddGas(G4double de) { fEnergyGas += de; }

#endif
