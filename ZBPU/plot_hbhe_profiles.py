#!/usr/bin/env python3
"""
Script to plot 2D profiles of average HCAL energy as a function of eta and phi for each depth.
Creates profiles from the 3D histograms (phi, eta, Energy) for each depth from PF candidates.
Plots separate profiles for HBHE and HF regions.
"""

import ROOT
import sys
import os

# Set ROOT to batch mode (no display)
ROOT.gROOT.SetBatch(True)

def plot_depth_profiles_for_region(f, region, output_dir):
    """
    Create 2D profile plots of average energy vs eta and phi for each depth for a specific region.
    
    Args:
        f: ROOT file object containing the histograms
        region: Region name ("HBHE" or "HF")
        output_dir: Directory to save output plots
    """
    
    # Set up canvas
    ROOT.gStyle.SetOptStat(0)
    ROOT.gStyle.SetPalette(ROOT.kBird)
    c = ROOT.TCanvas("c", f"HCAL Energy Profiles - {region}", 1200, 1000)
    c.SetRightMargin(0.15)
    
    # Loop over each depth (1-7)
    profiles = []
    for depth in range(1, 8):
        print(f"Processing {region} depth {depth}...")
        
        # Get the 3D histogram for this depth and region
        hist3d_name = f"myAnalyzer/hcal3D_depth{depth}_{region}"
        hist3d = f.Get(hist3d_name)
        
        if not hist3d:
            print(f"Warning: Cannot find histogram '{hist3d_name}', skipping...")
            continue
        
        print(f"  Found 3D histogram with {hist3d.GetEntries()} entries")
        
        # Create a TProfile2D from the TH3D by projecting energy (Z) onto phi-eta (X-Y)
        profile_name = f"profile_depth{depth}_{region}"
        profile = hist3d.Project3DProfile("yx")  # Project Z onto Y-X plane
        profile.SetName(profile_name)
        profile.SetTitle(f"HCAL Average E_{{T}} Profile - Depth {depth} ({region});#phi;#eta;Average E_{{T}} (GeV)")
        
        # Draw and save the profile
        c.Clear()
        profile.Draw("COLZ")
        
        # Update canvas
        c.Update()
        
        # Save as PNG and PDF
        output_png = os.path.join(output_dir, f"hcal_energy_profile_depth{depth}_{region}.png")
        output_pdf = os.path.join(output_dir, f"hcal_energy_profile_depth{depth}_{region}.pdf")
        c.SaveAs(output_png)
        c.SaveAs(output_pdf)
        
        print(f"  Saved: {output_png}")
        print(f"  Saved: {output_pdf}")
        
        # Keep the profile in memory
        profiles.append(profile.Clone())
    
    # Create a summary canvas with all depths for this region
    if profiles:
        print(f"Creating summary canvas for {region}...")
        c_summary = ROOT.TCanvas(f"c_summary_{region}", f"HCAL Energy Profiles - All Depths ({region})", 2400, 2000)
        c_summary.Divide(3, 3)
        
        for i, depth in enumerate(range(1, 8)):
            if i < len(profiles):
                c_summary.cd(i + 1)
                ROOT.gPad.SetRightMargin(0.15)
                profiles[i].Draw("COLZ")
        
        summary_png = os.path.join(output_dir, f"hcal_energy_profile_all_depths_{region}.png")
        summary_pdf = os.path.join(output_dir, f"hcal_energy_profile_all_depths_{region}.pdf")
        c_summary.SaveAs(summary_png)
        c_summary.SaveAs(summary_pdf)
        
        print(f"Saved summary: {summary_png}")
        print(f"Saved summary: {summary_pdf}")
    
    return profiles


def plot_pfcand_per_vertex_vs_eta(f, output_dir):
    """
    Create 1D plots of PF candidates per vertex vs eta, averaged over phi.
    
    Args:
        f: ROOT file object containing the histograms
        output_dir: Directory to save output plots
    """
    print("\n" + "="*60)
    print("Plotting PF Candidates per Vertex vs Eta (averaged over phi)")
    print("="*60)
    
    # Set up canvas
    ROOT.gStyle.SetOptStat(0)
    c = ROOT.TCanvas("c_pfcand", "PF Candidates per Vertex vs Eta", 1200, 800)
    c.SetLeftMargin(0.12)
    c.SetRightMargin(0.05)
    c.SetGrid()
    
    # Get the 2D histograms for HBHE and HF
    hist_hbhe_name = "myAnalyzer/pfCandPerVertex_etaPhi_HBHE"
    hist_hf_name = "myAnalyzer/pfCandPerVertex_etaPhi_HF"
    
    hist_hbhe = f.Get(hist_hbhe_name)
    hist_hf = f.Get(hist_hf_name)
    
    if not hist_hbhe or not hist_hf:
        print(f"Warning: Cannot find PF candidate histograms")
        return
    
    # Project onto eta axis (Y axis), which averages over phi (X axis)
    prof_hbhe = hist_hbhe.ProjectionY(f"{hist_hbhe_name}_eta_profile")
    prof_hf = hist_hf.ProjectionY(f"{hist_hf_name}_eta_profile")
    
    # Normalize by number of phi bins to get average
    n_phi_bins_hbhe = hist_hbhe.GetNbinsX()
    n_phi_bins_hf = hist_hf.GetNbinsX()
    prof_hbhe.Scale(1.0 / n_phi_bins_hbhe)
    prof_hf.Scale(1.0 / n_phi_bins_hf)
    
    # Style the histograms
    prof_hbhe.SetLineColor(ROOT.kBlue + 1)
    prof_hbhe.SetLineWidth(2)
    prof_hbhe.SetMarkerColor(ROOT.kBlue + 1)
    prof_hbhe.SetTitle("PF Candidates per Vertex per Event vs #eta (averaged over #phi);#eta;N_{PF} / (N_{PV} #times Event)")
    prof_hbhe.SetMarkerStyle(20)
    prof_hbhe.SetMarkerSize(0.8)
    
    prof_hf.SetLineColor(ROOT.kRed + 1)
    prof_hf.SetLineWidth(2)
    prof_hf.SetMarkerColor(ROOT.kRed + 1)
    prof_hf.SetMarkerStyle(21)
    prof_hf.SetMarkerSize(0.8)
    
    # Find the maximum value between both histograms to set y-axis range
    max_hbhe = prof_hbhe.GetMaximum()
    max_hf = prof_hf.GetMaximum()
    max_val = max(max_hbhe, max_hf)
    
    # Set y-axis range with 10% margin at top
    prof_hbhe.SetMaximum(max_val * 1.1)
    prof_hbhe.SetMinimum(0)
    
    # Draw both profiles
    prof_hbhe.Draw("HIST")
    prof_hf.Draw("HIST SAME")
    
    # Add legend
    legend = ROOT.TLegend(0.15, 0.75, 0.40, 0.88)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    legend.AddEntry(prof_hbhe, "HBHE Region", "l")
    legend.AddEntry(prof_hf, "HF Region", "l")
    legend.Draw()
    
    c.Update()
    
    # Save plots
    output_png = os.path.join(output_dir, "pfCandPerVertex_vs_eta.png")
    output_pdf = os.path.join(output_dir, "pfCandPerVertex_vs_eta.pdf")
    c.SaveAs(output_png)
    c.SaveAs(output_pdf)
    
    print(f"Saved: {output_png}")
    print(f"Saved: {output_pdf}")
    
    return prof_hbhe, prof_hf


def plot_depth_profiles(input_file, output_dir="plots"):
    """
    Create 2D profile plots of average energy vs eta and phi for each depth.
    Plots both HBHE and HF regions separately.
    
    Args:
        input_file: Path to ROOT file containing the hcal3D_depth histograms
        output_dir: Directory to save output plots
    """
    
    # Create output directory if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Open the input file
    f = ROOT.TFile.Open(input_file, "READ")
    if not f or f.IsZombie():
        print(f"Error: Cannot open file {input_file}")
        sys.exit(1)
    
    # Plot PF candidates per vertex vs eta (averaged over phi)
    plot_pfcand_per_vertex_vs_eta(f, output_dir)
    
    # Plot profiles for HBHE region
    print("\n" + "="*60)
    print("Processing HBHE Region")
    print("="*60)
    profiles_hbhe = plot_depth_profiles_for_region(f, "HBHE", output_dir)
    
    # Plot profiles for HF region
    print("\n" + "="*60)
    print("Processing HF Region")
    print("="*60)
    profiles_hf = plot_depth_profiles_for_region(f, "HF", output_dir)
    
    # Close the file
    f.Close()
    
    print("\n" + "="*60)
    print("Done! All plots saved to:", output_dir)
    print(f"HBHE profiles: {len(profiles_hbhe)}")
    print(f"HF profiles: {len(profiles_hf)}")
    print("="*60)


if __name__ == "__main__":
    # Default input file
    input_file = "output_histograms.root"
    
    # Check for command line argument
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found")
        print(f"Usage: python {sys.argv[0]} [input_file.root]")
        sys.exit(1)
    
    print(f"Reading from: {input_file}")
    plot_depth_profiles(input_file)
