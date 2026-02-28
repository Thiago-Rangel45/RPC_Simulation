#include <TApplication.h>
#include <TCanvas.h>
#include <TBox.h>
#include <TLatex.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TH2F.h>

#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/ComponentParallelPlate.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/ViewDrift.hh"

using namespace Garfield;

int main(int argc, char *argv[]) {

  TApplication app("app", &argc, argv);
  
  // Configurações globais de estilo para forçar renderização de cor
  gStyle->SetCanvasColor(kWhite);
  gStyle->SetFrameFillColor(kWhite);
  gStyle->SetOptStat(0);

  // --- Geometria ---
  const double dMylar  = 0.035;
  const double dWindow = 0.12;
  const double dGas    = 0.025;
  const double dGlass  = 0.07;

  std::vector<double> eps = {3.1, 6.0, 1.0, 8.0, 8.0, 1.0, 6.0, 3.1};
  std::vector<double> thickness = {dMylar, dWindow, dGas, dGlass, dGlass, dGas, dWindow, dMylar};
  const double dTotal = std::accumulate(thickness.begin(), thickness.end(), 0.);

  ComponentParallelPlate rpc;
  rpc.Setup(eps.size(), eps, thickness, -9000.0);
  rpc.AddPlane("Readout");

  MediumMagboltz gas;
  gas.LoadGasFile("cms_rpc_95.2_4.5_0.3_25-40kV.gas");
  gas.Initialise(true);
  rpc.SetMedium(&gas);

  Sensor sensor(&rpc);
  sensor.AddElectrode(&rpc, "Readout");
  sensor.SetArea(-0.05, 0., -0.05, 0.05, dTotal, 0.05);

  AvalancheMicroscopic aval(&sensor);
  AvalancheMC drift(&sensor);
  drift.SetTimeSteps(0.01);

  aval.AddElectron(0., dMylar + dWindow + 0.5 * dGas, 0., 0., 0.1);
  aval.AddElectron(0., dTotal - (dMylar + dWindow + 0.5 * dGas), 0., 0., 0.1);

  // --- Canvas ---
  TCanvas canvas("c", "RPC Simulation", 800, 900);
  
  ViewDrift driftView;
  driftView.SetCanvas(&canvas);
  driftView.SetArea(-0.05, 0., 0.05, dTotal);
  aval.EnablePlotting(&driftView);
  drift.EnablePlotting(&driftView);

  TLatex text;
  double tmin = 0.0;
  double dt = 0.02;
  const int nFrames = 250;

  for (int i = 0; i < nFrames; ++i) {
    canvas.cd();
    canvas.Clear();

    // 1. Desenha o frame (eixos)
    TH2F frame("h", "Simulacao RPC;X [cm];Y [cm]", 10, -0.05, 0.05, 10, 0, dTotal);
    frame.SetStats(0);
    frame.Draw("AXIS");

    // 2. Função de desenho corrigida para FORÇAR preenchimento sólido
    auto drawLayer = [&](double y1, double y2, int color, const char* label) {
      TBox b;
      b.SetFillColor(color);
      b.SetFillStyle(1001); // 1001 é o código para preenchimento SÓLIDO
      b.SetLineColor(kBlack);
      b.SetLineWidth(1);
      // O segredo: DrawBox força a pintura dos pixels da área
      b.DrawBox(-0.05, y1, 0.05, y2);
      
      // Adiciona o texto
      text.SetTextColor(kBlack);
      text.SetTextAlign(32);
      text.SetTextSize(0.025);
      text.DrawLatex(0.048, 0.5 * (y1 + y2), label);
    };

    double currY = 0.;
    // Usando cores padrão do ROOT que são mais "seguras" para GIF
    drawLayer(currY, currY += dMylar,  kGreen-7,  "Mylar");
    drawLayer(currY, currY += dWindow, kYellow-7, "Window");
    drawLayer(currY, currY += dGas,    kCyan-7,   "Gas Gap 1");
    drawLayer(currY, currY += dGlass,  kRed-7,    "Glass");
    drawLayer(currY, currY += dGlass,  kRed-7,    "Glass");
    drawLayer(currY, currY += dGas,    kCyan-7,   "Gas Gap 2");
    drawLayer(currY, currY += dWindow, kYellow-7, "Window");
    drawLayer(currY, currY += dMylar,  kGreen-7,  "Mylar");

    // 3. Evolução da Física
    if (!aval.GetElectrons().empty()) {
      aval.SetTimeWindow(tmin, tmin + dt);
      aval.ResumeAvalanche();
    }
    if (!drift.GetIons().empty()) {
      drift.SetTimeWindow(tmin, tmin + dt);
      drift.ResumeAvalanche();
    }

    // 4. Plotar a avalanche (importante: o "true" mantém as cores do fundo)
    driftView.Plot2d(false, true); 

    // 5. Tempo
    char labelTime[50];
    sprintf(labelTime, "t = %.2f ns", tmin);
    text.SetTextAlign(22);
    text.SetTextSize(0.035);
    text.DrawLatexNDC(0.5, 0.95, labelTime);

    canvas.Update();
    gSystem->ProcessEvents();

    if (i == nFrames - 1) canvas.Print("rpc_drift.gif++");
    else canvas.Print("rpc_drift.gif+3");

    tmin += dt;
  }

  app.Run(true);
  return 0;
}