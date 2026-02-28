#include <TApplication.h>
#include <TCanvas.h>
#include <TSystem.h>

#include <ctime>
#include <iostream>
#include <numeric>
#include <vector>

#include "Garfield/AvalancheGrid.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/ComponentParallelPlate.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/ViewSignal.hh"
#include "Garfield/ViewField.hh"

#define LOG(x) std::cout << x << std::endl

using namespace Garfield;

int main(int argc, char *argv[]) {
  TApplication app("app", &argc, argv);

  constexpr bool plotSignal = true;
  constexpr bool plotField  = true;

  // =========================
  // PDF filenames
  // =========================
  const std::string pdfSignal = "signal_double_gap.pdf";
  const std::string pdfCharge = "charge_double_gap.pdf";
  const std::string pdfField  = "weighting_field_double_gap.pdf";

  // =========================
  // Dielectric constants
  // =========================
  const double epMylar  = 3.1;
  const double epGlass  = 8.0;
  const double epWindow = 6.0;
  const double epGas    = 1.0;

  // =========================
  // Thicknesses [cm]
  // =========================
  const double dMylar  = 0.035;
  const double dGlass  = 0.07;
  const double dWindow = 0.12;
  const double dGas    = 0.025;

  // =========================
  // Layer stack (double gap)
  // =========================
  std::vector<double> eps = {
      epMylar, epWindow, epGas, epGlass,
      epGlass, epGas, epWindow, epMylar
  };

  std::vector<double> thickness = {
      dMylar, dWindow, dGas, dGlass,
      dGlass, dGas, dWindow, dMylar
  };

  const double dTotal =
      std::accumulate(thickness.begin(), thickness.end(), 0.);

  // =========================
  // Applied voltage
  // =========================
  const double voltage = -14e3;

  ComponentParallelPlate* rpc = new ComponentParallelPlate();
  rpc->Setup(eps.size(), eps, thickness, voltage);

  // =========================
  // Readout plane (central)
  // =========================
  const std::string label = "ReadoutPlane";
  rpc->AddPlane(label);

  // =========================
  // Gas
  // =========================
  MediumMagboltz gas;
  gas.LoadGasFile("cms_rpc_95.2_4.5_0.3_25-40kV.gas");
  gas.Initialise(true);

  rpc->SetMedium(&gas);

  // =========================
  // Sensor
  // =========================
  Sensor sensor(rpc);
  sensor.AddElectrode(rpc, label);

  const std::size_t nTimeBins = 200;
  sensor.SetTimeWindow(0., 4. / nTimeBins, nTimeBins);

  // =========================
  // Avalanches
  // =========================
  AvalancheMicroscopic aval(&sensor);
  aval.SetTimeWindow(0., 0.1);

  AvalancheGrid avalgrid(&sensor);

  const int nX = 5, nZ = 5;
  const double dY = 1.e-4;
  const int nY = int(dTotal / dY);

  avalgrid.SetGrid(-0.05, 0.05, nX,
                   0., dTotal, nY,
                   -0.05, 0.05, nZ);

  // =========================
  // Plots
  // =========================
  ViewSignal* signalView = nullptr;
  ViewSignal* chargeView = nullptr;
  TCanvas* cSignal = nullptr;
  TCanvas* cCharge = nullptr;

  if (plotSignal) {
    cSignal = new TCanvas("cSignal", "Induced current", 600, 600);
    signalView = new ViewSignal(&sensor);
    signalView->SetCanvas(cSignal);

    cCharge = new TCanvas("cCharge", "Induced charge", 600, 600);
    chargeView = new ViewSignal(&sensor);
    chargeView->SetCanvas(cCharge);
  }

  // =========================
  // Heed track — MUON 150 GeV
  // =========================
  TrackHeed track(&sensor);

  track.SetParticle("mu-");        // <-- múon negativo
  track.SetMomentum(1.5e11);       // <-- 150 GeV/c em eV/c
  track.CrossInactiveMedia(true);

  std::clock_t start = std::clock();

  // Start track in first gas gap
  const double y0 = dMylar + dWindow + 0.5 * dGas;
  track.NewTrack(0., y0, 0., 0., 0., 1., 0.);

  for (const auto& cluster : track.GetClusters()) {
    for (const auto& electron : cluster.electrons) {
      aval.AvalancheElectron(electron.x, electron.y,
                             electron.z, electron.t,
                             0.1, 0., 0., 0.);
      avalgrid.AddElectrons(&aval);
    }
  }

  LOG("Switching to grid based method");
  avalgrid.StartGridAvalanche();

  double duration =
      (std::clock() - start) / (double)CLOCKS_PER_SEC;

  LOG("Drift time: " << duration << " s");

  // =========================
  // Signal & charge
  // =========================
  if (plotSignal) {
    signalView->PlotSignal(label);
    cSignal->Update();
    cSignal->SaveAs(pdfSignal.c_str());

    sensor.ExportSignal(label, "Signal");

    sensor.IntegrateSignal(label);
    chargeView->PlotSignal(label);
    cCharge->Update();
    cCharge->SaveAs(pdfCharge.c_str());

    sensor.ExportSignal(label, "Charge");
  }

  LOG("Total induced charge = "
      << sensor.GetTotalInducedCharge(label) << " fC");

  // =========================
  // Weighting field
  // =========================
  if (plotField) {
    TCanvas* cField = new TCanvas("cField", "Weighting field", 600, 600);
    ViewField* fieldView = new ViewField(&sensor);
    fieldView->SetCanvas(cField);

    rpc->SetWeightingPotentialGrid(-0.5, 0.5, 1,
                                   0., dTotal, 200,
                                   -0.5, 0.5, 1,
                                   label);

    cField->SetLeftMargin(0.16);
    fieldView->PlotProfileWeightingField(label,
                                         0., 0., 0.,
                                         0., dTotal, 0.,
                                         "v", true);

    cField->Update();
    cField->SaveAs(pdfField.c_str());
  }

  LOG("End of program");
  app.Run(true);
  return 0;
}
