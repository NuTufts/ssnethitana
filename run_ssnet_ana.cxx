#include <iostream>
#include <string>

// larlitecv headers
// for source see: https://github.com/larbys/larlitecv
#include "Base/DataCoordinator.h"

// larlite
// for source see: https://github.com/LArLight/larlite
#include "DataFormat/mctrack.h"
#include "DataFormat/mcshower.h"

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
  std::string mcinfofile = argv[2];
  std::string outfile    = argv[3];
  
  // First, lets load the files
  larlitecv::DataCoordinator dataco; // allows us to read entry-aligned larlite and larcv files
  dataco.add_inputfile( ssnetfile, "larcv" );
  dataco.add_inputfile( mcinfofile, "larlite" );
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

  int numshower[4]; // planes u,v,y and then total
  int numtrack[4];
  int correcttrack[4];
  int correctshower[4];  
  int falsetrack[4];
  int falseshower[4];  
  int unlabeledtrack[4];
  int unlabeledshower[4];  
  int numabovethresh[4];

  TTree tssnet( "tssnet", "SSNet analyzaer" );
  tssnet.Branch( "numshower", numshower, "numshower[4]/I" );
  tssnet.Branch( "numtrack", numtrack, "numtrack[4]/I" );
  tssnet.Branch( "correctshower", correctshower, "correctshower[4]/I" );
  tssnet.Branch( "correcttrack",  correcttrack,   "correcttrack[4]/I" );
  tssnet.Branch( "falseshower", falseshower, "falseshower[4]/I" );
  tssnet.Branch( "falsetrack",  falsetrack,   "falsetrack[4]/I" );
  tssnet.Branch( "unlabeledshower", unlabeledshower, "unlabeledshower[4]/I" );
  tssnet.Branch( "unlabeledtrack",  unlabeledtrack,  "unlabeledtrack[4]/I" );
  tssnet.Branch( "numabovethresh", numabovethresh, "numabovethresh[4]/I" );  

  for (int ientry=0; ientry<nentries; ientry++) {

    // tell the data coordinator to go to the next entry
    dataco.goto_entry( ientry, "larcv" );

    std::cout << "Entry " << ientry << std::endl;

    // we load the object containers for this event

    // get the truth information (from the larlite file)
    larlite::event_mctrack* ev_tracks   = (larlite::event_mctrack*) dataco.get_larlite_data( larlite::data::kMCTrack,  "mcreco" );
    larlite::event_mcshower* ev_showers = (larlite::event_mcshower*)dataco.get_larlite_data( larlite::data::kMCShower, "mcreco" );

    // get the images (from the larcv file)

    // ADC image
    larcv::EventImage2D* ev_images      = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "modimg" ); // ADC image

    // scores from ssnet, for each plane
    larcv::EventImage2D* ev_ssnetout[3]; 
    for (int p=0; p<3; p++) {
      char treename[20];
      sprintf( treename, "uburn_plane%d", p );
      ev_ssnetout[p] = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, treename );
    }

    // track ID image
    larcv::EventImage2D* ev_trackid = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "instance" );

    // fill the variables
    // first, zero out variables
    for (int i=0; i<4; i++) {
      numshower[i] = 0;
      numtrack[i] = 0;
      numabovethresh[i] = 0;
      correctshower[i] = 0;
      correcttrack[i] = 0;      
      falseshower[i] = 0;
      falsetrack[i] = 0;      
      unlabeledshower[i] = 0;
      unlabeledtrack[i] = 0;      
    }

    // check number of ssnet images. sometimes we have none if the upstream algorithms
    // decide that there is nothing interesting in the image
    int nssnetimages = ev_ssnetout[0]->Image2DArray().size();

    if ( nssnetimages==0 ){
      // if nothing, fill and move on
      tssnet.Fill();
      continue;
    }

    // we loop through the tracks and showers and save the IDs there.
    // we'll use this to determine the true label
    std::set<int> showerids;
    std::set<int> trackids;
    for ( auto const& track : *ev_tracks ) {
      trackids.insert( track.TrackID() );
    }
    for ( auto const& shower : *ev_showers ) {
      showerids.insert( shower.TrackID() );
    }
    
    for (int p=0; p<3; p++) {
      // loop over the planes
      // get the adc image for the plane. the container stores all three planes. we grab plane p.
      const larcv::Image2D& adcimg = ev_images->Image2DArray().at(p);

      // get the adc image for the plane. the container stores all three planes. we grab plane p.
      const larcv::Image2D& idimg = ev_trackid->Image2DArray().at(p);
      
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
	  if ( adc<15 )
	    continue;

	  // get the score
	  float sh = showerimg.pixel(r,c);
	  float tr = trackimg.pixel(r,c);

	  // get the track id
	  int id = idimg.pixel(radc,cadc);
	  bool istrack = false;
	  bool isshower = false;
	  if ( trackids.find(id)!=trackids.end() ) {
	    istrack = true;
	  }
	  if ( showerids.find(id)!=showerids.end() ) {
	    isshower = true;
	  }

	  if ( !istrack && !isshower ) {
	    if (id!=-1)
	      isshower = true; // below threshold energy deposition
	    // else
	    //   std::cerr << "pixel is neither track nor shower at (" << r << "," << c << ") id=" << id << std::endl;	    
	    //throw std::runtime_error();
	  }

	  // do we have scores for this?
	  // to save time, we do not try to label the entire image
	  // we check if any track+shower score was produced
	  if ( sh+tr>0.2 ) {
	    numabovethresh[p]++;
	    if ( sh>tr ) {

	      numshower[p]++;
	      if ( isshower )
		correctshower[p]++;
	      else if ( istrack )
		falseshower[p]++;
	      else
		unlabeledshower[p]++;
	    }
	    else {
	      numtrack[p]++;
	      if ( istrack )
		correcttrack[p]++;
	      else if (isshower )
		falsetrack[p]++;
	      else
		unlabeledtrack[p]++;	      
	    }
	  }
	  
	}//end of col loop
      }//end of row loop

      // add the plane totals to the overal total
      numabovethresh[3] += numabovethresh[p];
      numshower[3] += numshower[p];
      numtrack[3] += numtrack[p];
      correctshower[3] += correctshower[p];
      correcttrack[3] += correcttrack[p];
      falseshower[3] += falseshower[p];
      falsetrack[3] += falsetrack[p];
      unlabeledshower[3] += unlabeledshower[p];
      unlabeledtrack[3] += unlabeledtrack[p];
    }//end of plane loop

    // fill the output tree
    tssnet.Fill();
    
  }//end of entry loop

  // write the file
  out->Write();
  
  return 0;
}
