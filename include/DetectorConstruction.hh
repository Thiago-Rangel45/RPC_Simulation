#ifndef DetectorConstruction_h
#define DetectorConstruction_h 

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;
class G4Region; // Adicionado
class GarfieldG4FastSimulationModel;
class GarfieldMessenger;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
    DetectorConstruction();
    virtual ~DetectorConstruction();
    virtual G4VPhysicalVolume *Construct() override;

private:
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();
    
    // Variáveis de membro para materiais e volumes lógicos
    G4LogicalVolume* logicPad = nullptr;
    G4Material* fPadMaterial = nullptr;
    G4Material* fBorderMaterial = nullptr;
    G4Material* fGasMaterial = nullptr; // Material do gás

    // Controle de sobreposição
    G4bool fCheckOverlaps = true; 
    
    // Ponteiros para a integração com Garfield++
    G4Region* fGasRegion = nullptr; // Região para o gás
    GarfieldG4FastSimulationModel* fGarfieldG4FastSimulationModel = nullptr;
    GarfieldMessenger* fGarfieldMessenger = nullptr;
};

#endif