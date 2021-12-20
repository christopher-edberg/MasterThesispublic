#!/bin/bash 

#############################
# Job comment
#############################
#SBATCH -N 2
#SBATCH -J TApr8
#SBATCH -o logSCN6p8test_strongscaling5.out
#SBATCH -t 04:00:00
#############################
# OpenMPI Infiniband flags
#############################
OMPIFLAGS="--map-by ppr:8:node "
OMPIFLAGS+="--mca mpi_leave_pinned 1 "
OMPIFLAGS+="--mca btl_openib_allow_ib 1 "
OMPIFLAGS+="--mca btl openib,self,vader "

mpirun $OMPIFLAGS --bind-to socket ./tatp_nvm
