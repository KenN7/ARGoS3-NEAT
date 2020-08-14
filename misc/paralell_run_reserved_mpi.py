#!/usr/bin/python3

#from __future__ import print_function
import argparse
from subprocess import *
import os
import re
import time
###############################################################################
# This script launches several runs in parallel in a SGE Cluster, and
# each run is parallelized using MPI.  Execute without parameters to see usage.
###############################################################################

########################### CONFIGURATION POINTS ##############################

NEAT_DIR = "/home/khasselmann/neat-argos3"
try:
    NEAT_DIR = os.environ['U_NEATDIR']
except:
    pass
print("taking {} as neat_dir".format(NEAT_DIR))

###############################################################################

p = argparse.ArgumentParser(description='runs a run multiple times in paralell using mpi, python paralell_run_mpi.py')
p.add_argument('-d', '--dir', help='the execution directory (where the folders for each run will be stored)', required=True)
p.add_argument('-j', '--jobname', help='the name of the job', required=True)
p.add_argument('-n', '--nbcores', help='number of parallel processes', type=int, required=True)
p.add_argument('-e', '--exp', help='the experiment (.xml or .argos)', required=True)
p.add_argument('-p', '--params', help='the file for the parameters of the evolution', required=True)
p.add_argument('-s', '--startgenes', help='the file for the starting genome', required=True)
p.add_argument('-r', '--runs', help='number of runs to do (number of sequential jobs) (default=1)', type=int, default=1)
#p.add_argument('-m', '--machine', help='the machine to run to (default TBD in script)', default=MACHINE)
#p.add_argument('-q', '--queue', help='the queue to run to (default TBD in script)', default=QUEUE)

class cd:
    """Context manager for changing the current working directory"""
    def __init__(self, newPath):
        self.newPath = os.path.expanduser(newPath)

    def __enter__(self):
        self.savedPath = os.getcwd()
        os.chdir(self.newPath)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.savedPath)


def run_neat(args,run):
    data = {"execdir": (os.path.abspath(args.dir)+ ("/%s-%i" % (args.jobname, run))),
            "nbcores": args.nbcores,
            "neatdir": NEAT_DIR,
            "experiment": os.path.abspath(args.exp),
            "params": os.path.abspath(args.params),
            "startgenes": os.path.abspath(args.startgenes),
    }
    print( "{neatdir}/bin/NEAT-evolution {experiment} {params} {startgenes} {nbcores} {neatdir}/bin/scheduler".format(**data) )

    with cd( "{execdir}".format(**data) ):
        print('Job running')
        check_call("{neatdir}/bin/NEAT-evolution {experiment} {params} {startgenes} {nbcores} {neatdir}/bin/scheduler".format(**data), shell=True)


if __name__ == "__main__":
    args = p.parse_args()

    for run in range(args.runs):
        os.makedirs(os.path.abspath(args.dir) + ("/%s-%i" % (args.jobname, run)))
        os.makedirs(os.path.abspath(args.dir) + ("/%s-%i" % (args.jobname, run)) + "/gen")
        run_neat(args,run)
        time.sleep(0.1)
