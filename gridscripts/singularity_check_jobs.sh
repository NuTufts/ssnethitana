#!/bin/bash

CONTAINER=/cluster/kappa/90-days-archive/wongjiradlab/larbys/images/dllee_unified/singularity-dllee-intertool-20180216.img
WORKDIR=/cluster/kappa/90-days-archive/wongjiradlab/twongj01/ssnetana/gridscripts

module load singularity

singularity exec ${CONTAINER} bash -c "source /usr/local/bin/thisroot.sh && cd /usr/local/share/dllee_unified && source configure.sh && cd ${WORKDIR} && python check_jobs.py"

