#ifndef PhysicsList_h
#define PhysicsList_h

#include "G4VModularPhysicsList.hh"
#include "globals.hh"

// MUDANÇA: O nome da classe foi corrigido para corresponder ao .cc
class PhysicsList : public G4VModularPhysicsList {
public:
    // MUDANÇA: O construtor e destrutor foram corrigidos
    PhysicsList();
    virtual ~PhysicsList();
    
    // O resto dos métodos já estava correto
    virtual void SetCuts();
    virtual void ConstructParticle();
    virtual void ConstructProcess();

protected:
    void AddParameterisation();
};

#endif