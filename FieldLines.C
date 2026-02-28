#include <TApplication.h>
#include <TCanvas.h>
#include <TH2.h>
#include <TStyle.h>

#include <vector>
#include <numeric>
#include <iostream>

#include "Garfield/ComponentParallelPlate.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ViewField.hh"

using namespace Garfield;

int main(int argc, char* argv[]) {

  TApplication app("app", &argc, argv);

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
  // Double-gap stack
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

  std::cout << "Total thickness = " << dTotal << " cm\n";

  // =========================
  // Applied voltage
  // =========================
  const double voltage = -14e3;  // -14 kV

  ComponentParallelPlate rpc;
  rpc.Setup(eps.size(), eps, thickness, voltage);

  // =========================
  // Gas (CMS RPC mixture)
  // =========================
  MediumMagboltz gas;
  if (!gas.LoadGasFile("cms_rpc_95.2_4.5_0.3_25-40kV.gas")) {
    std::cerr << "Erro ao carregar o arquivo de gás. Usando Ar/CO2.\n";
    gas.SetComposition("Ar", 80., "CO2", 20.);
  }
  gas.Initialise(true);

  // Atribui o meio (necessário para o cálculo do campo, mas o potencial independe)
  rpc.SetMedium(&gas);

  // =========================
  // Criação dos histogramas 2D
  // =========================
  const int nx = 300;   // resolução em x
  const int ny = 500;   // resolução em y (direção do empilhamento)

  TH2D *hPot = new TH2D("hPot", "Potential;x [cm];y [cm];V [V]",
                        nx, -0.1, 0.1, ny, 0., dTotal);
  TH2D *hField = new TH2D("hField", "Electric field magnitude;x [cm];y [cm];|E| [V/cm]",
                          nx, -0.1, 0.1, ny, 0., dTotal);

  std::cout << "\nAmostrando campo e potencial em uma grade " << nx << " x " << ny << "...\n";

  double ex, ey, ez, v;
  Medium* medium = nullptr;
  int status;

  for (int ix = 1; ix <= nx; ++ix) {
    double x = hPot->GetXaxis()->GetBinCenter(ix);
    for (int iy = 1; iy <= ny; ++iy) {
      double y = hPot->GetYaxis()->GetBinCenter(iy);
      rpc.ElectricField(x, y, 0., ex, ey, ez, v, medium, status);
      hPot->SetBinContent(ix, iy, v);
      double e = sqrt(ex*ex + ey*ey + ez*ez);
      hField->SetBinContent(ix, iy, e);
    }
  }

  // =========================
  // Plotagem
  // =========================
  gStyle->SetNumberContours(255);
  gStyle->SetPalette(kBird);

  TCanvas* cV = new TCanvas("cV", "Potential", 800, 600);
  cV->SetLeftMargin(0.12);
  cV->SetRightMargin(0.15);
  hPot->GetZaxis()->SetTitleOffset(1.4);
  hPot->Draw("colz");
  cV->Update();
  cV->SaveAs("potential.png");

  TCanvas* cE = new TCanvas("cE", "Electric field", 800, 600);
  cE->SetLeftMargin(0.12);
  cE->SetRightMargin(0.15);
  hField->GetZaxis()->SetTitleOffset(1.4);
  hField->Draw("colz");
  cE->Update();
  cE->SaveAs("field.png");

  std::cout << "Arquivos potential.png e field.png gerados.\n";

  app.Run(true);
  return 0;
}