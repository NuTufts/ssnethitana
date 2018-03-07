# ssnetana

Usage: ./run_ssnet_hitana [larcv image file] [larlite reco2d (hit) file] [output rootfile]

This is meant to be run (and developed) while in the DL LEE analysis container.

To start the container, first run

    ./start_container.sh


Then setup the shell environment for the DL LEE programs

    ./setup_env.sh


Then you can make the program

    make

You can run (on Meitner) a test example:

   ./test.sh


## Example files

If you want example files to develop with, look in `gridscripts/filelists/extbnb/mcc8v6_extbnb ... `

It will have files per (run,subrun) pair.  Just download the files for a given subrun (should be an ssnetout and larlite-reco2d file).  
You can pass this into the program.
