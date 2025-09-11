// Crie este arquivo em: src/ActionInitialization.cc

#include "ActionInitialization.hh"

// Inclua os headers das suas classes de ação que já existem
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
// Se você tivesse um SteppingAction, incluiria aqui também.

ActionInitialization::ActionInitialization()
 : G4VUserActionInitialization()
{}

ActionInitialization::~ActionInitialization()
{}

// Este método é executado apenas uma vez, na thread principal.
void ActionInitialization::BuildForMaster() const
{
    // Normalmente, o RunAction é registrado aqui para que ele tenha uma
    // visão global de todo o "run".
    SetUserAction(new RunAction());
}

// Este método é executado para CADA thread de trabalho.
void ActionInitialization::Build() const
{
    // Registra as ações que cada thread precisa ter sua própria cópia.
    SetUserAction(new PrimaryGeneratorAction());
    SetUserAction(new RunAction());
    SetUserAction(new EventAction());
}