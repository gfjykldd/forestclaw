#!/bin/bash

#SBATCH -J tsunami_004         # job name
#SBATCH -o tsunami_004.o%j     # output and error file name (%j expands to jobID)
#SBATCH --ntasks=4
#SBATCH -N 2                 # number of nodes requested
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

# cd /home/donnacalhoun/projects/ForestClaw/code/forestclaw/applications/clawpack/shallow/2d/tsunami_cuda

# mpirun tsunami --user:cuda=T --clawpack46:mthlim="1" --clawpack46:order="2 2" --nout=100

module load openmpi3/3.1.4
module load cuda/10.1

mpirun tsunami \
     --output=F \
     --user:cuda=T \
     --cudaclaw:order="2 2" \
     --cudaclaw:mthlim="4" \
     --clawpack46:order="2 2" \
     --clawpack46:mthlim="4"

#nvprof -o tsunami_prof.nvpp -e all srun --mpi=pmix_v2 tsunami 

