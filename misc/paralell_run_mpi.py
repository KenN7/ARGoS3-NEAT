#!/usr/bin/python2

from __future__ import print_function
import argparse
from subprocess import Popen
import os
import re
import time
###############################################################################
# This script launches several runs in parallel in a SGE Cluster, and
# each run is parallelized using MPI.  Execute without parameters to see usage.
###############################################################################

########################### CONFIGURATION POINTS ##############################

NEAT_DIR = "/home/khasselmann/neat-argos3"

QUEUE='short' #or 'long'
#MACHINE='opteron2216' #rack 1
MACHINE='xeon5410' #rack 2
#MACHINE='opteron6128' #rack 3
#MACHINE='opteron6272' #rack 4
#MACHINE='xeon2680' #rack 5


###############################################################################

p = argparse.ArgumentParser(description='runs a run multiple times in paralell using mpi, python paralell_run_mpi.py')
p.add_argument('-d', '--dir', help='the execdir', required=True)
p.add_argument('-n', '--nbjobs', help='number of parallel jobs', type=int, required=True)
p.add_argument('-e', '--exp', help='the experiment xml', required=True)
p.add_argument('-p', '--params', help='the file for the parameters of the evolution', required=True)
p.add_argument('-s', '--startgenes', help='the file for the starting genome', required=True)
p.add_argument('-m', '--machine', help='the machine to run to', default=MACHINE)
p.add_argument('-q', '--queue', help='the queue to run to', default=QUEUE)


def run_neat(args):
    data = {"jobname": "neatevo-%i" % os.getpid(),
            "machine": args.machine,
            "queue": args.queue,
            "execdir": os.path.abspath(args.dir),
            "nbjob": args.nbjobs,
            "neatdir": NEAT_DIR,
            "experiment": os.path.abspath(args.exp),
            "params": os.path.abspath(args.params),
            "startgenes": os.path.abspath(args.startgenes),
    }
    script = """#!/bin/bash
#$ -N %(jobname)s
#$ -l %(machine)s
#$ -l %(queue)s
#$ -m base
#      b     Mail is sent at the beginning of the job.
#      e     Mail is sent at the end of the job.
#      a     Mail is sent when the job is aborted or rescheduled.
#      s     Mail is sent when the job is suspended.
#$ -o %(execdir)s/argos3-neat.stdout
#$ -e %(execdir)s/argos3-neat.stderr
#$ -cwd
#$ -pe mpi %(nbjob)s
#$ -binding linear:256

USERNAME=`whoami`
NEATDIR=%(neatdir)s
COMMAND="$NEATDIR/bin/train %(experiment)s %(params)s %(startgenes)s %(nbjob)s $NEATDIR/bin/scheduler"

mkdir -p %(execdir)s/gen
cd %(execdir)s
echo "$COMMAND"
eval $COMMAND
$RET=$?
echo $RET
exit $RET"""
     #pro = subprocess.call("qsub "+(script % data), shell=True)
    p = Popen("qsub -V PATH", shell=True, bufsize=bufsize, stdin=PIPE, stdout=PIPE, close_fds=True)
    (child_stdout, child_stdin) = (p.stdout, p.stdin)
    child_stdin.write(script % data)
    child_stdin.close()
    print('Job sended')
    print(output.read())
    time.sleep(0.1)
    #print("qsub "+(script % data))


if __name__ == "__main__":
    args = p.parse_args()
    run_neat(args)
