#!/bin/bash

#SBATCH -J bump_002         # job name
#SBATCH -o bump_002.o%j     # output and error file name (%j expands to jobID)
#SBATCH --ntasks=2
#SBATCH -N 1                 # number of nodes requested
#SBATCH --tasks-per-node=2   # Each task gets exactly one GPU
#SBATCH -t 01:00:00          # run time (hh:mm:ss) - 12.0 hours in this example.


# --------------------------------------------------------
# Might not be needed ?  
# --------------------------------------------------------
# #SBATCH --cpus-per-task=4    # Used for multithreading
# #SBATCH --gres=gpu:2         # Doesn't seem to work on Redhawk
# #SBATCH -n 2                 # total number of cpus requested.
# #SBATCH --exclusive          # request exclusive usage of your nodes. 
# ---------------------------------------------------------


# Execute the program:

module load openmpi3/3.1.4
module load cuda/10.1

mpirun bump \
     --nout=200 --nstep=5 \
     --output= T\
     --user:cuda=T \
     --cudaclaw:order="2 2" \
     --clawpack46:order="2 2"

# nvprof -o tsunami_prof.nvpp -e all srun --mpi=pmix_v2 tsunami 

