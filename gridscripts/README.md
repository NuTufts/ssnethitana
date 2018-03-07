## Steps to get this setup on the Tufts Cluster

### Get the code
* clone this repository in the group's storage area. preferrably in your user area.

### Modify the scripts to work with your copy
* run `make_inputlists.py`: parses the input file lists in `filelists/` and makes individual input lists in `inputlists`. Each worker node will use these to know file to process. The script also produces `joblist.txt`. This keeps tracks of the individual jobs to be run.
* change `PATH` and `LD_LIBRARY_PATH` in `run_job.sh` to where the program lives
* change `WORKDIR` and `OUTPUTDIR` in `submit.sh` to where this readme lives and where the output files should go, respectively. (You probably do not need to change the container location.)
* change `WORKDIR` in `singularity_check_jobs.sh` to point where this README lives
* change `OUTDIR` in `check_jobs.py` to where the output folder is (same as `OUTPUTDIR` in `submit.sh`)

I have a feeling I am missing something. If you find it, please let me know. Basically, if you see a folder in my area, change it.

(also if you want to write a python script that generates these files, please feel free.)

### Compile the program

* go into the directory above
* go into `start_container.sh` and choose the lines for the Tufts cluster
* run it to start the container
* type bash in the following prompt
* you'll probably have to find your way back to the repository. It's probably something like `/cluster/kappa/90-days-archive/wongjiradlab/[username]/ssnetana`.
* run `setup_env.sh`
* type `make`

It should compile, hopefully.



