#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "neat.h"
#include "network.h"
#include "organism.h"
#include "population.h"
#include <iostream>
//#include "genome.h"
//#include "species.h"

using namespace NEAT;

void launchNEAT(const char* neatParams, const char* startGenes, double (*fctExperiment)(Network&),
                unsigned int seed = 0);
void launchNEAT(const char* neatParams, const char* startGenes,
                void (*fctExperiment)(Population&, unsigned int), unsigned int seed = 0);

#endif
