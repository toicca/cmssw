// -*- C++ -*-
//
// Package:    ZBPU/ZBPUAnalyzer
// Class:      ZBPUAnalyzer
//
/**\class ZBPUAnalyzer ZBPUAnalyzer.cc ZBPU/ZBPUAnalyzer/plugins/ZBPUAnalyzer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Nico Timothy Toikka
//         Created:  Mon, 09 Mar 2026 14:06:42 GMT
//
//

// system include files
#include <memory>
#include <iostream>
#include <cmath>

// ROOT include files
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TString.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Common/interface/TriggerNames.h"
//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<>
// This will improve performance in multithreaded jobs.

class ZBPUAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit ZBPUAnalyzer(const edm::ParameterSet&);
  ~ZBPUAnalyzer() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void endJob() override;

  // ----------member data ---------------------------
  edm::EDGetTokenT<reco::PFCandidateCollection> pfCandidatesToken_;  //used to select what PF candidates to read
  edm::EDGetTokenT<reco::VertexCollection> verticesToken_;  //used to select what vertices to read
  edm::EDGetTokenT<double> rhoToken_;  //used to select what rho value to read
  edm::EDGetTokenT<edm::TriggerResults> triggerResultsToken_;  //used to access MET filter decisions
  std::vector<std::string> metFilterNames_;  //names of MET filters to check

  // Event counter
  long long nEvents_;

  // Histograms
  TH1D* h_nVertices_;  //histogram for number of vertices
  TH1D* h_rho_;        //histogram for rho
  TH1D* h_nPFCandidates_;  //histogram for number of PF Candidates
  TH1D* h_metFilters_;  //histogram for MET filter results
  
  // All PF candidates histogram
  TH2D* h_pfCandPerVertex_etaPhi_All_;  //2D histogram: PF candidates per vertex vs eta and phi (All)
  
  // HBHE region histograms
  TH2D* h_pfCandPerVertex_etaPhi_HBHE_;  //2D histogram: PF candidates per vertex vs eta and phi (HBHE)
  TH3D* h_hcal3D_depth_HBHE[8];  //3D histograms for depths 1-7: phi, eta, energy (HBHE, index 0 unused)
  
  // HF region histograms
  TH2D* h_pfCandPerVertex_etaPhi_HF_;  //2D histogram: PF candidates per vertex vs eta and phi (HF)
  TH3D* h_hcal3D_depth_HF[8];  //3D histograms for depths 1-7: phi, eta, energy (HF, index 0 unused)
#ifdef THIS_IS_AN_EVENTSETUP_EXAMPLE
  edm::ESGetToken<SetupData, SetupRecord> setupToken_;
#endif
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
ZBPUAnalyzer::ZBPUAnalyzer(const edm::ParameterSet& iConfig)
    : pfCandidatesToken_(consumes<reco::PFCandidateCollection>(iConfig.getParameter<edm::InputTag>("pfCandidates"))),
      verticesToken_(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("vertices"))),
      rhoToken_(consumes<double>(iConfig.getParameter<edm::InputTag>("rho"))),
      triggerResultsToken_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("triggerResults"))),
      metFilterNames_(iConfig.getParameter<std::vector<std::string>>("metFilterNames")),
      nEvents_(0) {
  usesResource("TFileService");
#ifdef THIS_IS_AN_EVENTSETUP_EXAMPLE
  setupDataToken_ = esConsumes<SetupData, SetupRecord>();
#endif
  //now do what ever initialization is needed
}

ZBPUAnalyzer::~ZBPUAnalyzer() {
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)
  //
  // please remove this method altogether if it would be left empty
}

//
// member functions
//

// ------------ method called for each event  ------------
void ZBPUAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using namespace edm;

  ++nEvents_;

  // Check MET filters
  auto triggerResults = iEvent.get(triggerResultsToken_);
  const edm::TriggerNames& triggerNames = iEvent.triggerNames(triggerResults);
  
  bool passAllFilters = true;
  std::cout << "\n=== MET Filter Results ===" << std::endl;
  
  for (size_t i = 0; i < metFilterNames_.size(); ++i) {
    const std::string& filterName = metFilterNames_[i];
    unsigned int filterIndex = triggerNames.triggerIndex(filterName);
    
    bool filterPassed = false;
    if (filterIndex < triggerResults.size()) {
      filterPassed = triggerResults.accept(filterIndex);
      std::cout << filterName << ": " << (filterPassed ? "PASS" : "FAIL") << std::endl;
      
      // Fill histogram: bin i+1 for filter i
      h_metFilters_->Fill(i, filterPassed ? 1.0 : 0.0);
    } else {
      std::cout << filterName << ": NOT FOUND" << std::endl;
    }
    
    if (!filterPassed) passAllFilters = false;
  }
  
  std::cout << "Overall: " << (passAllFilters ? "PASS ALL" : "FAIL SOME") << std::endl;
  h_metFilters_->Fill(metFilterNames_.size(), passAllFilters ? 1.0 : 0.0);

  auto vertices = iEvent.get(verticesToken_);
  std::cout << "Number of primary vertices: " << vertices.size() << std::endl;
  
  // Fill histogram with number of vertices
  h_nVertices_->Fill(vertices.size());
  
  auto pfCandidates = iEvent.get(pfCandidatesToken_);
  std::cout << "Number of PF Candidates: " << pfCandidates.size() << std::endl;
  h_nPFCandidates_->Fill(pfCandidates.size());
  
  // Calculate weight for per-vertex normalization
  float weight = (vertices.size() > 0) ? 1.0 / vertices.size() : 0.0;

  for (const auto& cand : pfCandidates) {
    // Skip candidates with puppiWeight = 0
    // if (cand.puppiWeight() == 0.0) {
      // continue;
    // }
    
    // Get HCAL depth energy fractions
    const auto& depthFractions = cand.hcalDepthEnergyFractions();
    
    // Get total HCAL energy, normalized by cosh(eta) to obtain transverse energy
    float eta = cand.eta();
    float phi = cand.phi();
    float hcalEnergy = cand.hcalEnergy() / std::cosh(eta);
    
    // Fill histogram for all PF candidates
    h_pfCandPerVertex_etaPhi_All_->Fill(phi, eta, weight);
    
    // Determine if this is HF or HBHE based on particleId
    // h_HF = 6, egamma_HF = 7
    int particleId = cand.particleId();
    bool isHF = (particleId == 6 || particleId == 7);
    
    if (isHF) {
      // Fill HF histograms
      h_pfCandPerVertex_etaPhi_HF_->Fill(phi, eta, weight);
      
      for (int depth = 1; depth <= 7; depth++) {
        float depthEnergy = hcalEnergy * depthFractions[depth - 1];
        if (depthEnergy > 0) {
          h_hcal3D_depth_HF[depth]->Fill(phi, eta, depthEnergy);
        }
      }
    } else {
      // Fill HBHE histograms
      h_pfCandPerVertex_etaPhi_HBHE_->Fill(phi, eta, weight);
      
      for (int depth = 1; depth <= 7; depth++) {
        float depthEnergy = hcalEnergy * depthFractions[depth - 1];
        if (depthEnergy > 0) {
          h_hcal3D_depth_HBHE[depth]->Fill(phi, eta, depthEnergy);
        }
      }
    }
  }

  double rho = iEvent.get(rhoToken_);
  if (rho != 0) {
    std::cout << "Rho (fixedGridRhoFastjetAll): " << rho << std::endl;
  }
  
  // Fill histogram with rho value
  h_rho_->Fill(rho);

#ifdef THIS_IS_AN_EVENTSETUP_EXAMPLE
  // if the SetupData is always needed
  auto setup = iSetup.getData(setupToken_);
  // if need the ESHandle to check if the SetupData was there or not
  auto pSetup = iSetup.getHandle(setupToken_);
#endif
}

// ------------ method called once each job just before starting event loop  ------------
void ZBPUAnalyzer::beginJob() {
  // Book histograms using TFileService
  edm::Service<TFileService> fs;
  
  // Example histograms
  h_nVertices_ = fs->make<TH1D>("nVertices", "Number of Primary Vertices;N_{PV};Events", 100, 0, 100);
  h_rho_ = fs->make<TH1D>("rho", "Rho (fixedGridRhoFastjetAll);#rho (GeV);Events", 100, 0, 100);
  h_nPFCandidates_ = fs->make<TH1D>("nPFCandidates", "Number of PF Candidates;N_{PF};Events", 5000, 0, 5000);
  
  // MET filters histogram
  int nFilters = metFilterNames_.size() + 1; // +1 for "all filters" bin
  h_metFilters_ = fs->make<TH1D>("metFilters", "MET Filter Results;Filter;Pass Rate", 
                                  nFilters, 0, nFilters);
  // Set bin labels
  for (size_t i = 0; i < metFilterNames_.size(); ++i) {
    h_metFilters_->GetXaxis()->SetBinLabel(i + 1, metFilterNames_[i].c_str());
  }
  h_metFilters_->GetXaxis()->SetBinLabel(nFilters, "All Filters");
  
  // Create 2D histograms for PF candidates per vertex vs eta and phi
  // All PF candidates
  h_pfCandPerVertex_etaPhi_All_ = fs->make<TH2D>("pfCandPerVertex_etaPhi_All", 
                                                  "PF Candidates per Vertex per Event (All);#phi;#eta;N_{PF}/N_{PV}",
                                                  72, -M_PI, M_PI,   // phi bins
                                                  50, -5.0, 5.0);     // eta bins
  
  // HBHE region
  h_pfCandPerVertex_etaPhi_HBHE_ = fs->make<TH2D>("pfCandPerVertex_etaPhi_HBHE", 
                                                   "PF Candidates per Vertex per Event (HBHE);#phi;#eta;N_{PF}/N_{PV}",
                                                   72, -M_PI, M_PI,   // phi bins
                                                   50, -5.0, 5.0);     // eta bins
  
  // HF region
  h_pfCandPerVertex_etaPhi_HF_ = fs->make<TH2D>("pfCandPerVertex_etaPhi_HF", 
                                                 "PF Candidates per Vertex per Event (HF);#phi;#eta;N_{PF}/N_{PV}",
                                                 72, -M_PI, M_PI,   // phi bins
                                                 50, -5.0, 5.0);     // eta bins
  
  // Create TH3D for each depth (1-7): phi, eta, energy - HBHE region
  for (int depth = 1; depth <= 7; depth++) {
    TString name = Form("hcal3D_depth%d_HBHE", depth);
    TString title = Form("HCAL 3D Histogram Depth %d (HBHE);#phi;#eta;Energy / cosh(#eta)", depth);
    h_hcal3D_depth_HBHE[depth] = fs->make<TH3D>(name.Data(), title.Data(), 
                                                  72, -M_PI, M_PI,   // phi bins from -π to π
                                                  50, -5.0, 5.0,     // eta bins from -5 to 5
                                                  256, 0, 25.6);     // Energy bins
  }
  
  // Create TH3D for each depth (1-7): phi, eta, energy - HF region
  for (int depth = 1; depth <= 7; depth++) {
    TString name = Form("hcal3D_depth%d_HF", depth);
    TString title = Form("HCAL 3D Histogram Depth %d (HF);#phi;#eta;Energy / cosh(#eta)", depth);
    h_hcal3D_depth_HF[depth] = fs->make<TH3D>(name.Data(), title.Data(), 
                                                72, -M_PI, M_PI,   // phi bins from -π to π
                                                50, -5.0, 5.0,     // eta bins from -5 to 5
                                                256, 0, 25.6);     // Energy bins
  }
}

// ------------ method called once each job just after ending the event loop  ------------
void ZBPUAnalyzer::endJob() {
  if (nEvents_ > 0) {
    double scale = 1.0 / nEvents_;
    h_pfCandPerVertex_etaPhi_All_->Scale(scale);
    h_pfCandPerVertex_etaPhi_HBHE_->Scale(scale);
    h_pfCandPerVertex_etaPhi_HF_->Scale(scale);
  }
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void ZBPUAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);

}

//define this as a plug-in
DEFINE_FWK_MODULE(ZBPUAnalyzer);
