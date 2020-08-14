#!/usr/bin/python3

import argparse

p = argparse.ArgumentParser(description="generate empty node genomes for evostick/neat (fully connected, hidden com nodes)")
p.add_argument('-i', '--inputs', help='number of input nodes', type=int)
p.add_argument('-b', '--bias', help='number of bias input nodes', type=int)
p.add_argument('-o', '--outputs', help='number of ouput nodes', type=int)
p.add_argument('-f', '--file', help='output file')
p.add_argument('-hi', '--hidden', help='hidden nodes', type=int)

def gen(args):
    genome = "genomestart 1\ntrait 1 0.1 0 0 0 0 0 0 0\n"
    for i in range(1,args.inputs+1):
        genome += "node {} 0 1 1\n".format(i)
    for i in range(args.inputs+1, args.inputs+args.bias+1):
        genome += "node {} 0 1 3\n".format(i)
    for i in range(args.inputs+args.bias+1, args.inputs+args.outputs+args.bias+1):
        genome += "node {} 0 0 2\n".format(i)
    for i in range(args.inputs+args.outputs+args.bias+1,args.inputs+args.outputs+args.bias+args.hidden+1):
        genome += "node {} 0 0 0\n".format(i)
    for i in range(args.inputs+args.outputs+args.bias+args.hidden+1,args.inputs+args.outputs+args.bias+args.hidden+1+1): #we had one extra com in
        genome += "node {} 0 0 1\n".format(i)
    
    id = 1
    for node in range(1,args.inputs+args.bias+1):
        for output in range(args.inputs+args.bias+1, args.inputs+args.outputs+args.bias+1-1): #we remove the com output
            genome += "gene 1 {} {} 0.0 0 {} 0 1\n".format(node, output, id)
            id += 1
    for node in range(1,args.inputs+args.bias+1):
        for output in range(args.inputs+args.outputs+args.bias+1,args.inputs+args.outputs+args.bias+args.hidden+1): #connect to hidden nodes
            genome += "gene 1 {} {} 0.0 0 {} 0 1\n".format(node, output, id)
            id += 1
    for node in range(args.inputs+args.outputs+args.bias+1,args.inputs+args.outputs+args.bias+args.hidden+1): #connect hidden to output
        for output in range(args.inputs+args.bias+1, args.inputs+args.outputs+args.bias+1): #we add the com output
            genome += "gene 1 {} {} 0.0 0 {} 0 1\n".format(node, output, id)
            id += 1
    for node in range(args.inputs+args.outputs+args.bias+1,args.inputs+args.outputs+args.bias+args.hidden+1):
        genome += "gene 1 {} {} 0.0 1 {} 0 1\n".format(node, node, id)
        id += 1
    
    for node in range(args.inputs+args.bias+1+2, args.inputs+args.outputs+args.bias+1): #only com output
        for output in range(args.inputs+args.outputs+args.bias+args.hidden+1,args.inputs+args.outputs+args.bias+args.hidden+1+1): #we had one extra com in
            genome += "gene 1 {} {} 0.0 1 {} 0 1\n".format(node, output, id)
            id += 1

    for node in range(args.inputs+args.outputs+args.bias+args.hidden+1,args.inputs+args.outputs+args.bias+args.hidden+1+1): #we had one extra com in
        for output in range(args.inputs+args.bias+1, args.inputs+args.outputs+args.bias+1): #we add the com output
            genome += "gene 1 {} {} 0.0 0 {} 0 1\n".format(node, output, id)
            id += 1


    genome += "genomeend 1\n"
    print(genome)   

if __name__ == "__main__":
    args = p.parse_args()
    gen(args)
