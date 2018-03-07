#include <iostream>
#include <string>

// larlitecv headers
// for source see: https://github.com/larbys/larlitecv
#include "Base/DataCoordinator.h"

// larlite
// for source see: https://github.com/LArLight/larlite
#include "DataFormat/hit.h"
#include "LArUtil/LArProperties.h"

// larcv
// for source see: https://github.com/larbys/LArCV
#include "DataFormat/EventImage2D.h"

// ROOT
#include "TTree.h"
#include "TFile.h"

int main(int nargs, char** argv ) {

  std::cout << "SSNet Ana" << std::endl;

  // We open a larcv-format root file
  // From it we loop over the events and
  //   retrieve the image container for the ssnet output images for the event
  //   from the event container, we get the images for the event
  //   for each image, we analyze the contents, checking the output
  //
  //   to check the judgements, we have to determine the true answer for each pixel
  //   so in additition to the images, we have to retrieve the truth MC information
  //     about the event. We have the following truth information
  //     (1) mctrack and mcshower: objects which carry information
  //           about what particle traveled through the detector. each particle carries
  //           a unique ID number
  //     (2) instance image: tells us the ID number of the particle that deposited the
  //           energy into a given pixel in the image. we use it to correlate
  //           info in the images to the truth information about the particles

  // parse the arguments
  std::string ssnetfile  = argv[1];
  std::string hitfile    = argv[2];
  std::string outfile    = argv[3];
  
  // First, lets load the files
  larlitecv::DataCoordinator dataco; // allows us to read entry-aligned larlite and larcv files
  dataco.add_inputfile( ssnetfile, "larcv" );
  dataco.add_inputfile( hitfile, "larlite" );
  dataco.initialize();
  int nentries = dataco.get_nentries("larcv");
  std::cout << "LArCV/larlite files loaded" << std::endl;
  std::cout << "Numebr of entries: " << nentries << std::endl;

  // make the output file and define the output tree
  TFile* out = nullptr;
  try {
    out = new TFile( outfile.c_str(), "new" ); // new, but cannot overwrite
  }
  catch (...) {
    std::cout << "Problem making output file." << std::endl;
    return 0;
  }


  TTree tssnet( "thitaana", "Hit analyzaer" );

  // variables go here

  for (int ientry=0; ientry<nentries; ientry++) {

    // tell the data coordinator to go to the next entry
    dataco.goto_entry( ientry, "larcv" );

    std::cout << "Entry " << ientry << std::endl;

    // we load the object containers for this event

    // INPUT DATA
    // ----------
    // ADC image
    larcv::EventImage2D* ev_images      = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "modimg" ); // ADC image

    // scores from ssnet, for each plane
    larcv::EventImage2D* ev_ssnetout[3]; 
    for (int p=0; p<3; p++) {
      char treename[20];
      sprintf( treename, "uburn_plane%d", p );
      ev_ssnetout[p] = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, treename );
    }

    // list of hits
    larlite::event_hit* ev_hits = (larlite::event_hit*)dataco.get_larlite_data( larlite::data::kHit, "gaushit" );
    const std::vector<larlite::hit>& hit_v = *ev_hits;
    int numhits = hit_v.size();


    // first, zero out variables

    // check number of ssnet images. sometimes we have none if the upstream algorithms
    // decide that there is nothing interesting in the image
    int nssnetimages = ev_ssnetout[0]->Image2DArray().size();

    if ( nssnetimages==0 ){
      // if nothing, fill and move on
      tssnet.Fill();
      continue;
    }

    // drift velocity * us/tick:
    // conversion factor for translating between time after trigger and tick position in image
    const float image_tick_at_trigger = 3200; 
    const float usec_per_tick = 0.5; // microseconds
    const float cm_per_tick = larutil::LArProperties::GetME()->DriftVelocity()*0.5;
    
    // example loop over hits
    for ( auto const& hit : hit_v ) {
      // hit is type larlite::hit
      // to see header go to https://github.com/larlight/larlite/blob/trunk/core/DataFormat/hit.h
      int plane = hit.View();
      const larlite::geo::WireID& wid = hit.WireID();
      int wire  = (int)wid.Wire;
      float peak = hit.PeakTime(); // us after trigger
      int peak_tick = image_tick_at_trigger + peak/usec_per_tick;
      float integral = hit.Integral();
    }
    
    // loop over images
    for (int p=0; p<3; p++) {
      // loop over the planes
      // get the adc image for the plane. the container stores all three planes. we grab plane p.
      const larcv::Image2D& adcimg = ev_images->Image2DArray().at(p);

      // we get the judgement for each image
      // the ev_ssnetout container contains 2 images, the shower labels, and track labels.
      const larcv::Image2D& showerimg = ev_ssnetout[p]->Image2DArray().at(0);
      const larcv::Image2D& trackimg  = ev_ssnetout[p]->Image2DArray().at(1);      

      // ImageMeta class stores parameters about the image
      // we need one for both the adc and ssnet because they are not the same size images
      // to save time, we only label a cropped image
      const larcv::ImageMeta& adcmeta   = adcimg.meta();
      const larcv::ImageMeta& ssnetmeta = showerimg.meta();

      // loop over rows and columns of the ssnetmeta, as it is a subset
      for (size_t r=0; r<ssnetmeta.rows(); r++) {

	// we have to translate to the bigger image as well
	// we get the absolute coordinate value (tick,wire)
	float tick = ssnetmeta.pos_y( r );

	// then go and get the row,col in the full adc image
	int radc = adcmeta.row(tick);
	
	for (size_t c=0; c<ssnetmeta.cols(); c++) {
	  // same thing for the x-axis (wires)
	  float wire = ssnetmeta.pos_x( c );
	  int cadc   = adcmeta.col(wire);	  
	
	  // get the adc value
	  float adc = adcimg.pixel(radc,cadc);

	  // if below threshold, skip, not interesting
	  if ( adc<5.0 )
	    continue;

	  // get the score
	  float sh = showerimg.pixel(r,c);
	  float tr = trackimg.pixel(r,c);

	  
	}//end of col loop
      }//end of row loop

    }//end of plane loop

    // fill the output tree
    tssnet.Fill();
    
  }//end of entry loop

  // write the file
  out->Write();
  
  return 0;
}
