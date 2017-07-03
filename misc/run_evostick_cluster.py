#!/usr/bin/python2

from __future__ import print_function
import sys, getopt
from subprocess import *
import os
import re
import time

###############################################################################
# This script launches ...
###############################################################################

########################### CONFIGURATION POINTS ##############################

EVOSTICK_BIN_DIR = "/home/vsubramanian/ARGoS3-NEAT/bin"
def run_evostick_simu(name, genome, mission):
    for count in range(6):
                output_file = open("/home/vsubramanian/PRC/Results/Simulation/test6/" + name + "-"+str(count)+".txt", 'w')
                seeds = open("/home/vsubramanian/PRC/seeds.txt", 'r')
                for seed in seeds.readlines():
                    seed = int(seed)
                    output_file.write(str(seed) + '\t')

                    genome1 = "evostick2/evo-"+str(count)+"/gen/gen_last_1_champ"
                    full_cmd = EVOSTICK_BIN_DIR + '/evostick_launch' + ' -s ' + str(seed) + ' -c ' + mission + ' -g ' + genome1
                    p = Popen(full_cmd.split(" "), stdin=PIPE, stdout=PIPE, stderr=PIPE)
                    output, err = p.communicate("input data that is passed to subprocess' stdin")
                    print(output)
                    print(err)
                    vec_output = output.split("\n")
                    score = vec_output[len(vec_output)-2].split(" ")[1]
                    output_file.write(str(score) + '\t')
                    output_file.write('\n')
                output_file.close()

# def run_evostick_pr(name, genome, mission):
#         output_file = open("/home/aligot/PRClusterExec/Results/PseudoReality/" + name + ".txt", 'w')
#         seeds = open("/home/aligot/PRClusterExec/seeds.txt", 'r')
#         for seed in seeds.readlines():
#             seed = int(seed)
#             output_file.write(str(seed) + '\t')
#             full_cmd = EVOSTICK_BIN_DIR + '/evostick_launch_pr' + ' -s ' + str(seed) + ' -c ' + mission + ' -g ' + genome
#             p = Popen(full_cmd.split(" "), stdin=PIPE, stdout=PIPE, stderr=PIPE)
#             output, err = p.communicate(b"input data that is passed to subprocess' stdin")
#             print(output)
#             print(err)
#             vec_output = output.split("\n")
#             score = vec_output[len(vec_output)-2].split(" ")[1]
#             output_file.write(str(score) + '\t')
#             output_file.write('\n')
#         output_file.close()


if __name__ == "__main__":
    mission_file = ""
    controller_file = ""
    controller_index = 0
    simulation_flag = True
    name = ""
    genome = ""
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hm:g:s:n:", ["mission=", "controllers=", "index=", "simulation=", "name="])
    except getopt.GetoptError:
        print("run_evostick_cluster.py -m <mission> -g <genome> -s <simulation (T or F)> -n <name of exp>")
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-h":
            print("run_evostick_cluster.py -m <mission> -g <genome> -s <simulation (T or F)> -n <name of exp>")
            sys.exit()
        elif opt in ("-m", "--mission"):
            mission_file = arg
        elif opt in ("-g", "--genome"):
            genome = arg
        elif opt in ("-s", "--simulation"):
            simulation_flag = arg
        elif opt in ("-n", "--name"):
            name = arg

    # if (simulation_flag):
    run_evostick_simu(name, genome, mission_file)
    # else:
        # run_evostick_pr(name, genome, mission_file)
