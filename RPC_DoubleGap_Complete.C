//======================================================================
// RPC_DoubleGap_Complete.C
// Simulação completa de uma RPC de duplo gap tipo CMS no Garfield++
// Inclui: Diagnóstico de campo, potencial, sinal, carga e animação.
//======================================================================

#include <TApplication.h>
#include <TCanvas.h>
#include <TH2.h>
#include <TStyle.h>
#include <TBox.h>
#include <TLatex.h>
#include <TSystem.h>

#include <iostream>
#include <numeric>
#include <vector>
#include <ctime>

#include "Garfield/ComponentParallelPlate.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/AvalancheGrid.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/ViewSignal.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/ViewDrift.hh"

using namespace Garfield;

#define LOG(x) std::cout << x << std::endl

int main(int argc, char *argv[]) {
    TApplication app("app", &argc, argv);

    // ===================================================================
    // 1. PARÂMETROS DA GEOMETRIA
    // ===================================================================
    const double epMylar  = 3.1;      // permissividade do Mylar
    const double epGlass  = 8.0;      // permissividade do vidro
    const double epWindow = 6.0;      // permissividade da janela (eletrodo)
    const double epGas    = 1.0;      // gás

    // Espessuras em cm
    const double dMylar  = 0.0035;    // 35 µm
    const double dWindow = 0.012;     // 120 µm
    const double dGas    = 0.14;      // 1.4 mm (gap configurado pelo usuário)
    const double dGlass  = 0.02;      // 0,2 mm (vidro entre gaps)

    // Empilhamento: Mylar, Window, Gap, Glass, Gap, Window, Mylar
    std::vector<double> eps = {
        epMylar, epWindow, epGas, epGlass,
        epGas, epWindow, epMylar
    };
    std::vector<double> thickness = {
        dMylar, dWindow, dGas, dGlass,
        dGas, dWindow, dMylar
    };
    const double dTotal = std::accumulate(thickness.begin(), thickness.end(), 0.);
    LOG("Espessura total = " << dTotal << " cm");

    // Tensão aplicada
    const double voltage = -14438;     // V

    // ===================================================================
    // 2. COMPONENTE DE CAMPO E DIAGNÓSTICO
    // ===================================================================
    ComponentParallelPlate rpc;
    rpc.Setup(eps.size(), eps, thickness, voltage);
    rpc.AddPlane("Readout");

    // Extração de valores para o Log de Diagnóstico
    double ex, ey, ez, v_start, v_end;
    Medium* med = nullptr;
    int status;

    // Medição no Gap 1 (Inferior)
    double y_gap1_start = dMylar + dWindow;
    double y_gap1_end   = dMylar + dWindow + dGas;
    rpc.ElectricField(0, y_gap1_start, 0, ex, ey, ez, v_start, med, status);
    rpc.ElectricField(0, y_gap1_end,   0, ex, ey, ez, v_end,   med, status);
    double fieldGap1 = std::sqrt(ex*ex + ey*ey + ez*ez);
    double deltaV_Gap1 = std::abs(v_start - v_end);

    // Medição no Gap 2 (Superior)
    double y_gap2_start = dMylar + dWindow + dGas + dGlass;
    double y_gap2_end   = dMylar + dWindow + dGas + dGlass + dGas;
    rpc.ElectricField(0, y_gap2_start, 0, ex, ey, ez, v_start, med, status);
    rpc.ElectricField(0, y_gap2_end,   0, ex, ey, ez, v_end,   med, status);
    double fieldGap2 = std::sqrt(ex*ex + ey*ey + ez*ez);
    double deltaV_Gap2 = std::abs(v_start - v_end);

    LOG("---------------------------------------------------------");
    LOG("RELATÓRIO DE CAMPO E POTENCIAL NOS GAPS:");
    LOG("Tensão Total Aplicada: " << voltage << " V");
    LOG("Gap 1 (Inferior):");
    LOG("  - Campo Elétrico: " << fieldGap1 << " V/cm (" << fieldGap1/1000.0 << " kV/cm)");
    LOG("  - Queda de Potencial (Delta V): " << deltaV_Gap1 << " V");
    LOG("Gap 2 (Superior):");
    LOG("  - Campo Elétrico: " << fieldGap2 << " V/cm (" << fieldGap2/1000.0 << " kV/cm)");
    LOG("  - Queda de Potencial (Delta V): " << deltaV_Gap2 << " V");
    LOG("---------------------------------------------------------");

    // ===================================================================
    // 3. MEIO GASOSO
    // ===================================================================
    MediumMagboltz gas;
    if (!gas.LoadGasFile("cms_rpc_95.2_4.5_0.3_25-40kV.gas")) {
        LOG("Arquivo de gás não encontrado. Usando Ar/CO2 (80/20).");
        gas.SetComposition("Ar", 80., "CO2", 20.);
    }
    gas.Initialise(true);
    rpc.SetMedium(&gas);

    // ===================================================================
    // 4. SENSOR
    // ===================================================================
    Sensor sensor(&rpc);
    sensor.AddElectrode(&rpc, "Readout");
    const int nTimeBins = 200;
    const double tMin = 0.;
    const double tMax = 60.;          // ns (Ajustado para gap de 1.4mm)
    const double tStep = (tMax - tMin) / nTimeBins;
    sensor.SetTimeWindow(tMin, tStep, nTimeBins);
    sensor.SetArea(-0.5, 0., -0.5, 0.5, dTotal, 0.5);

    // ===================================================================
    // 5. CÁLCULO E PLOTAGEM DO CAMPO ELÉTRICO E POTENCIAL
    // ===================================================================
    const int nx = 300;
    const int ny = 500;
    TH2D *hPot = new TH2D("hPot", "Potencial Elétrico;x [cm];y [cm];V [V]", nx, -0.5, 0.5, ny, 0., dTotal);
    TH2D *hField = new TH2D("hField", "Campo Elétrico;x [cm];y [cm];|E| [V/cm]", nx, -0.5, 0.5, ny, 0., dTotal);

    LOG("Amostrando campo e potencial na grade...");
    for (int ix = 1; ix <= nx; ++ix) {
        double x = hPot->GetXaxis()->GetBinCenter(ix);
        for (int iy = 1; iy <= ny; ++iy) {
            double y = hPot->GetYaxis()->GetBinCenter(iy);
            double ex_l, ey_l, ez_l, v_l;
            rpc.ElectricField(x, y, 0., ex_l, ey_l, ez_l, v_l, med, status);
            hPot->SetBinContent(ix, iy, v_l);
            hField->SetBinContent(ix, iy, std::sqrt(ex_l*ex_l + ey_l*ey_l + ez_l*ez_l));
        }
    }

    gStyle->SetNumberContours(255);
    gStyle->SetPalette(kBird);
    TCanvas* cV = new TCanvas("cV", "Potencial", 800, 600);
    hPot->Draw("colz");
    cV->SaveAs("potencial.png");

    TCanvas* cE = new TCanvas("cE", "Campo Eletrico", 800, 600);
    hField->Draw("colz");
    cE->SaveAs("campo_eletrico.png");

    // ===================================================================
    // 6. SIMULAÇÃO DA IONIZAÇÃO PRIMÁRIA E AVALANCHE
    // ===================================================================
    TrackHeed track(&sensor);
    track.SetParticle("mu-");
    track.SetMomentum(150.e9);
    track.CrossInactiveMedia(true);

    double yStart = dMylar + dWindow + 0.5 * dGas;
    track.NewTrack(0., yStart, 0., 0., 0., 1., 0.);

    AvalancheMicroscopic aval(&sensor);
    aval.SetTimeWindow(0., 0.1);

    AvalancheGrid avalgrid(&sensor);
    const double dY_grid = 1.e-4;
    const int nY_grid = int(dTotal / dY_grid);
    avalgrid.SetGrid(-0.05, 0.05, 5, 0., dTotal, nY_grid, -0.05, 0.05, 5);

    for (const auto& cluster : track.GetClusters()) {
        for (const auto& electron : cluster.electrons) {
            aval.AvalancheElectron(electron.x, electron.y, electron.z, electron.t, 0.1, 0., 0., 0.);
            avalgrid.AddElectrons(&aval);
        }
    }
    LOG("Iniciando avalanche em grade...");
    avalgrid.StartGridAvalanche();

    // ===================================================================
    // 7. SINAL INDUZIDO
    // ===================================================================
    TCanvas* cSignal = new TCanvas("cSignal", "Induced current", 600, 600);
    ViewSignal signalView(&sensor);
    signalView.SetCanvas(cSignal);
    signalView.PlotSignal("Readout");
    cSignal->SaveAs("signal_double_gap.pdf");

    sensor.IntegrateSignal("Readout");
    TCanvas* cCharge = new TCanvas("cCharge", "Induced charge", 600, 600);
    ViewSignal chargeView(&sensor);
    chargeView.SetCanvas(cCharge);
    chargeView.PlotSignal("Readout");
    cCharge->SaveAs("charge_double_gap.pdf");

    LOG("Carga total induzida = " << sensor.GetTotalInducedCharge("Readout") << " fC");

    // ===================================================================
    // 9. ANIMAÇÃO DA AVALANCHE
    // ===================================================================
    TCanvas canvasAnim("canvasAnim", "Animação da Avalanche", 800, 900);
    ViewDrift driftView;
    driftView.SetCanvas(&canvasAnim);
    driftView.SetArea(-0.2, 0., 0.2, dTotal);

    AvalancheMicroscopic avalAnim(&sensor);
    AvalancheMC driftAnim(&sensor);
    driftAnim.SetTimeSteps(0.01);

    avalAnim.AddElectron(0., dMylar + dWindow + 0.5 * dGas, 0., 0., 0.1);
    avalAnim.AddElectron(0., dTotal - (dMylar + dWindow + 0.5 * dGas), 0., 0., 0.1);

    avalAnim.EnablePlotting(&driftView);
    driftAnim.EnablePlotting(&driftView);

    TLatex text;
    double tAnim = 0.0;
    double dtAnim = 0.05;
    const int nFrames = 200;

    for (int i = 0; i < nFrames; ++i) {
        canvasAnim.cd();
        canvasAnim.Clear();
        TH2F frame("h", "Simulação RPC - Animação;x [cm];y [cm]", 10, -0.2, 0.2, 10, 0, dTotal);
        frame.SetStats(0);
        frame.Draw("AXIS");

        auto drawLayer = [&](double y1, double y2, int color, const char* label) {
            TBox b; b.SetFillColor(color); b.SetFillStyle(1001); b.SetLineColor(kBlack);
            b.DrawBox(-0.2, y1, 0.2, y2);
            text.SetTextColor(kBlack); text.SetTextAlign(32); text.SetTextSize(0.025);
            text.DrawLatex(0.18, 0.5*(y1+y2), label);
        };

        double ypos = 0.;
        drawLayer(ypos, ypos += dMylar,  kGreen-7, "Mylar");
        drawLayer(ypos, ypos += dWindow, kYellow-7, "Window");
        drawLayer(ypos, ypos += dGas,    kCyan-7, "Gas 1");
        drawLayer(ypos, ypos += dGlass,  kRed-7,   "Glass");
        drawLayer(ypos, ypos += dGas,    kCyan-7, "Gas 2");
        drawLayer(ypos, ypos += dWindow, kYellow-7, "Window");
        drawLayer(ypos, ypos += dMylar,  kGreen-7, "Mylar");

        if (!avalAnim.GetElectrons().empty()) {
            avalAnim.SetTimeWindow(tAnim, tAnim + dtAnim);
            avalAnim.ResumeAvalanche();
        }
        if (!driftAnim.GetIons().empty()) {
            driftAnim.SetTimeWindow(tAnim, tAnim + dtAnim);
            driftAnim.ResumeAvalanche();
        }

        driftView.Plot2d(false, true);
        char labelTime[50]; sprintf(labelTime, "t = %.2f ns", tAnim);
        text.SetTextAlign(22); text.SetTextSize(0.035);
        text.DrawLatexNDC(0.5, 0.95, labelTime);

        canvasAnim.Update();
        if (i == 0) canvasAnim.Print("animacao_rpc.gif++");
        else canvasAnim.Print("animacao_rpc.gif+");
        tAnim += dtAnim;
    }

    LOG("Animação salva como animacao_rpc.gif");
    LOG("End of program");
    app.Run(true);
    return 0;
}