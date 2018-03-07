#!/bin/bash
#
#SBATCH --job-name=anass
#SBATCH --output=log_anass.txt
#SBATCH --ntasks=10
#SBATCH --time=10:00
#SBATCH --mem-per-cpu=2000

WORKDIR=/cluster/kappa/90-days-archive/wongjiradlab/twongj01/ssnetana/gridscripts
CONTAINER=/cluster/kappa/90-days-archive/wongjiradlab/larbys/images/dllee_unified/singularity-dllee-intertool-20180216.img
JOBIDLIST=${WORKDIR}/rerunlist.txt
INPUTLISTDIR=${WORKDIR}/inputlists
OUTPUTDIR=/cluster/kappa/90-days-archive/wongjiradlab/twongj01/ssnetana/output

mkdir -p ${OUTPUTDIR}
module load singularity
srun singularity exec ${CONTAINER} bash -c "cd ${WORKDIR} && source run_job.sh ${WORKDIR} ${INPUTLISTDIR} ${JOBIDLIST} ${OUTPUTDIR}"

