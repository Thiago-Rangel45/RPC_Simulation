#include "DetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4Material.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4Region.hh"
#include "FastSimulationModel.hh"

DetectorConstruction::DetectorConstruction() {}

DetectorConstruction::~DetectorConstruction() {}

G4VPhysicalVolume* DetectorConstruction::Construct() {
    DefineMaterials();
    G4VPhysicalVolume* world = DefineVolumes();

    if (fGasRegion) {
        G4cout << ">>> Attaching Fast Simulation Model to region '" 
               << fGasRegion->GetName() << "'" << G4endl;
        new FastSimulationModel("GarfieldFastSim", fGasRegion);
    } else {
        G4cerr << "!!!! ERRO: Região do gás não foi criada. O modelo rápido do Garfield não será ativado." << G4endl;
    }
    
    return world;
}

void DetectorConstruction::DefineMaterials(){
    G4NistManager* nistManager = G4NistManager::Instance();

    // Materiais padrão
    nistManager->FindOrBuildMaterial("G4_Cu");
    nistManager->FindOrBuildMaterial("G4_Al");
    nistManager->FindOrBuildMaterial("G4_PLEXIGLASS");
    nistManager->FindOrBuildMaterial("G4_GRAPHITE");
    nistManager->FindOrBuildMaterial("G4_GLASS_PLATE");
    nistManager->FindOrBuildMaterial("G4_AIR");

    // Elementos
    G4Element* elC = nistManager->FindOrBuildElement("C");
    G4Element* elO = nistManager->FindOrBuildElement("O");
    G4Element* elAr = nistManager->FindOrBuildElement("Ar");

    // Dióxido de Carbono (CO2)
    G4Material* co2 = new G4Material("CO2", 1.977 * mg / cm3, 2, kStateGas);
    co2->AddElement(elC, 1);
    co2->AddElement(elO, 2);

    // Mistura de gás Argônio-CO2 (70/30)
    fGasMaterial = new G4Material("ArCO2_70_30", 1.8223 * mg / cm3, 2, kStateGas);
    fGasMaterial->AddElement(elAr, 70. * perCent);
    fGasMaterial->AddMaterial(co2, 30. * perCent);
    
    // Atribui materiais às variáveis de membro
    fPadMaterial = nistManager->FindOrBuildMaterial("G4_Cu");
    fBorderMaterial = nistManager->FindOrBuildMaterial("G4_Cu");

    G4cout << *(G4Material::GetMaterialTable()) << G4endl;
}

G4VPhysicalVolume* DetectorConstruction::DefineVolumes() {
  
    G4NistManager *nist  = G4NistManager::Instance(); // <<< A variável correta é 'nist'
    G4bool fCheckOverlaps = true;

    // Volume do Mundo
    G4Material* worldMat = nist->FindOrBuildMaterial("G4_AIR");
    G4double rWorld = 1.5 * m;
    G4Sphere* solidWorld = new G4Sphere("World", 0., rWorld, 0., 2*pi, 0., pi);      
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, worldMat, "logicWorld");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "physWorld", 0, false, 0, fCheckOverlaps);
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
    
    // =================================================================================
    // Definição das camadas do detector (EMPILHAMENTO AO LONGO DO EIXO Y)
    // =================================================================================
    G4double aluThickness = 2.5 * cm;
    G4double acrylicThickness = 1.0 * cm;
    G4double graphiteThickness = 1.0 * cm;
    G4double glassThickness = 0.2 * cm; 
    G4double gasThickness = 0.2 * cm;
    G4double padThickness = 0.5 * cm;

    // Dimensões transversais
    G4double dimX_Al = 128.5 * cm;
    G4double dimZ_Al = 165.0 * cm;

    // Definição dos sólidos com espessura na direção Y
    // CORREÇÃO: Trocando 'nistManager' por 'nist'
    G4Material* aluminium = nist->FindOrBuildMaterial("G4_Al");
    G4Box* solidAlu = new G4Box("AlLayerSolid", dimX_Al / 2, aluThickness / 2, dimZ_Al / 2);
    G4LogicalVolume* logicAlu = new G4LogicalVolume(solidAlu, aluminium, "AlLayerLV");
    logicAlu->SetVisAttributes(new G4VisAttributes(G4Colour(0.7, 0.7, 0.7))); 

    G4Material* acrylic = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
    G4Box* solidAcrylic = new G4Box("AcrylicBoxSolid", 125.0*cm/2, acrylicThickness/2, 155.0*cm/2);
    G4LogicalVolume* logicAcrylic = new G4LogicalVolume(solidAcrylic, acrylic, "AcrylicBoxLV");
    logicAcrylic->SetVisAttributes(new G4VisAttributes(G4Colour(0.2, 0.6, 0.9)));

    G4Material* graphite = nist->FindOrBuildMaterial("G4_GRAPHITE");
    G4Box* solidGraphite = new G4Box("GraphiteBoxSolid", 119.0*cm/2, graphiteThickness/2, 148.5*cm/2);
    G4LogicalVolume* logicGraphite = new G4LogicalVolume(solidGraphite, graphite, "GraphiteBoxLV");
    logicGraphite->SetVisAttributes(new G4VisAttributes(G4Colour(0.3, 0.3, 0.3))); 

    G4Material* glass = nist->FindOrBuildMaterial("G4_GLASS_PLATE");
    G4Box* solidGlass = new G4Box("GlassBoxSolid", 120.0*cm/2, glassThickness/2, 152.0*cm/2);
    G4LogicalVolume* logicGlass = new G4LogicalVolume(solidGlass, glass, "GlassBoxLV");
    logicGlass->SetVisAttributes(new G4VisAttributes(G4Colour(0.8, 1.0, 1.0, 0.3)));

    G4Box* solidGasVolume = new G4Box("GasVolumeSolid", dimX_Al / 2, gasThickness / 2, dimZ_Al / 2);
    G4LogicalVolume* logicGasVolume = new G4LogicalVolume(solidGasVolume, fGasMaterial, "GasVolumeLV");
    logicGasVolume->SetVisAttributes(new G4VisAttributes(G4Colour(0.5, 0.5, 1.0, 0.3)));

    // --- Início do posicionamento das camadas ao longo de Y ---
    G4double currentY = 0; 

    // O centro do detector ficará em Y=0. Primeiro calculamos a altura total.
    G4double totalThickness = 2*aluThickness + 2*acrylicThickness + 2*graphiteThickness + 2*glassThickness + gasThickness + padThickness;
    currentY = totalThickness / 2.0; // Começa do topo (Y positivo) e vai descendo

    // Camada de Alumínio Superior
    currentY -= aluThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicAlu, "AlLayerPV_Top", logicWorld, false, 1, fCheckOverlaps);
    currentY -= aluThickness / 2.0;

    // --- Grade de Pads (agora no plano XZ) ---
    G4double xPad = 14.0 * cm;
    G4double zPad_dim = 18.0 * cm; // A dimensão Y original do pad agora é Z
    G4double borderThickness = 1.0 * cm;
    
    currentY -= padThickness / 2.0;
    G4double padPos_Y = currentY;

    G4Box* solidPad = new G4Box("Pad", xPad/2, padThickness/2, zPad_dim/2);
    G4LogicalVolume* logicPad = new G4LogicalVolume(solidPad, fPadMaterial, "logicPad");
    logicPad->SetVisAttributes(new G4VisAttributes(G4Colour(1.0, 0.5, 0.0)));
    
    G4double offsetX = -3.5 * (xPad + borderThickness);
    G4double offsetZ = -3.5 * (zPad_dim + borderThickness);

    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            G4double posX = offsetX + j * (xPad + borderThickness);
            G4double posZ = offsetZ + i * (zPad_dim + borderThickness);
            new G4PVPlacement(0, G4ThreeVector(posX, padPos_Y, posZ), logicPad, "physPad", logicWorld, false, j * 8 + i, fCheckOverlaps);
        }
    }
    currentY -= padThickness / 2.0;
    // --- Fim da grade de pads ---
    
    // Camada de Acrílico Superior
    currentY -= acrylicThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicAcrylic, "AcrylicBoxPV_Top", logicWorld, false, 1, fCheckOverlaps);
    currentY -= acrylicThickness / 2.0;

    // Camada de Grafite Superior
    currentY -= graphiteThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicGraphite, "GraphiteBoxPV_Top", logicWorld, false, 1, fCheckOverlaps);
    currentY -= graphiteThickness / 2.0;

    // Camada de Vidro Superior
    currentY -= glassThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicGlass, "GlassBoxPV_Top", logicWorld, false, 1, fCheckOverlaps);
    currentY -= glassThickness / 2.0;

    // Volume de Gás (região central)
    currentY -= gasThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicGasVolume, "GasVolumePV", logicWorld, false, 0, fCheckOverlaps);
    fGasRegion = new G4Region("RegionGarfield");
    fGasRegion->AddRootLogicalVolume(logicGasVolume);
    currentY -= gasThickness / 2.0;

    // Camada de Vidro Inferior
    currentY -= glassThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicGlass, "GlassBoxPV_Bottom", logicWorld, false, 2, fCheckOverlaps);
    currentY -= glassThickness / 2.0;
    
    // Camada de Grafite Inferior
    currentY -= graphiteThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicGraphite, "GraphiteBoxPV_Bottom", logicWorld, false, 2, fCheckOverlaps);
    currentY -= graphiteThickness / 2.0;
    
    // Camada de Acrílico Inferior
    currentY -= acrylicThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicAcrylic, "AcrylicBoxPV_Bottom", logicWorld, false, 2, fCheckOverlaps);
    currentY -= acrylicThickness / 2.0;
    
    // Camada de Alumínio Inferior
    currentY -= aluThickness / 2.0;
    new G4PVPlacement(nullptr, G4ThreeVector(0, currentY, 0), logicAlu, "AlLayerPV_Bottom", logicWorld, false, 2, fCheckOverlaps);
    
    return physWorld;
}