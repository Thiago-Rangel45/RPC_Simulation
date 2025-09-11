// Crie este arquivo em: src/ActionInitialization.hh

#ifndef ActionInitialization_h
#define ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

// Esta classe é responsável por criar e registrar todas as outras
// classes de "Ação" (PrimaryGenerator, Run, Event, etc.)
// para cada thread de trabalho no Geant4.

class ActionInitialization : public G4VUserActionInitialization
{
  public:
    ActionInitialization();
    virtual ~ActionInitialization();

    // Este método é chamado para construir as ações para as threads de trabalho.
    virtual void Build() const;

    // (Opcional) Este método é chamado para a thread principal (master).
    // Geralmente usado para ações que não devem ser duplicadas, como o RunAction.
    virtual void BuildForMaster() const;
};

#endif