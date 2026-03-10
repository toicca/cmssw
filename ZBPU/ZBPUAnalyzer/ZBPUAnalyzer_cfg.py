import FWCore.ParameterSet.Config as cms
import FWCore.PythonUtilities.LumiList as LumiList
import argparse
import os

parser = argparse.ArgumentParser(description='ZBPUAnalyzer configuration')
parser.add_argument('--lumiJson', type=str, default='', help='Path to lumi JSON file')
parser.add_argument('--inputFiles', nargs='+', help='Input ROOT files')
parser.add_argument('--maxEvents', type=int, default=-1, help='Maximum number of events to process')
parser.add_argument('--outputFile', type=str, default='output_histograms.root', help='Output ROOT file name')
options = parser.parse_args()


process = cms.Process("ZBPUAnalyzer")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.load("HLTrigger.HLTfilters.hltHighLevel_cfi")

process.hltHighLevel.HLTPaths = cms.vstring('*ZeroBias*')

# Load MET filters
process.load('RecoMET.METFilters.BadPFMuonFilter_cfi')
process.load('RecoMET.METFilters.BadChargedCandidateFilter_cfi')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(options.maxEvents) )

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        options.inputFiles
        # 'root://xrootd-cms.infn.it///store/data/Run2025G/ZeroBias/AOD/PromptReco-v1/000/398/600/00000/f023f442-c23f-4eb0-b286-418cf889c919.root'
        )
)

# Apply lumi JSON filter if provided
if options.lumiJson and os.path.exists(options.lumiJson):
    print(f"Applying luminosity JSON filter from: {options.lumiJson}")
    lumiList = LumiList.LumiList(filename=options.lumiJson).getCMSSWString().split(',')
    process.source.lumisToProcess = cms.untracked.VLuminosityBlockRange()
    process.source.lumisToProcess.extend(lumiList)
elif options.lumiJson:
    print(f"WARNING: Lumi JSON file not found: {options.lumiJson}")
    print("Proceeding without luminosity filtering")
else:
    print("No luminosity JSON file provided - processing all events")

process.myAnalyzer = cms.EDAnalyzer("ZBPUAnalyzer",
    pfCandidates = cms.InputTag("particleFlow"),
    vertices = cms.InputTag("offlinePrimaryVertices"),
    rho = cms.InputTag("fixedGridRhoFastjetAll"),
    triggerResults = cms.InputTag("TriggerResults", "", "RECO"),
    metFilterNames = cms.vstring(
        'Flag_HBHENoiseFilter',
        'Flag_HBHENoiseIsoFilter',
        'Flag_EcalDeadCellTriggerPrimitiveFilter',
        'Flag_goodVertices',
        'Flag_eeBadScFilter',
        'Flag_globalSuperTightHalo2016Filter',
        'Flag_BadPFMuonFilter',
        'Flag_BadChargedCandidateFilter'
    )
)

# TFileService for histogram output
process.TFileService = cms.Service("TFileService",
    fileName = cms.string(options.outputFile)
)

# Define the path with HLT filter and analyzer
process.p = cms.Path(
    process.hltHighLevel + 
    process.BadPFMuonFilter + 
    process.BadChargedCandidateFilter + 
    process.myAnalyzer
)