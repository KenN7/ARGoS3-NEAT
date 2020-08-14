// MPI
#ifdef MPI_FOUND
#include <mpi.h>
#endif
// Standard C++ Library
#include <iostream>
#include <math.h>
#include <sstream>
#include <string>
#include <vector>

// NEAT
#include "../NEAT/experiments.h"

// ARGOS
#include <argos3/core/simulator/loop_functions.h>
#include <argos3/core/simulator/simulator.h>
#include <argos3/core/simulator/visualization/visualization.h>
#include <argos3/core/utility/plugins/dynamic_loading.h>
#include <argos3/plugins/robots/e-puck/simulator/epuck_entity.h>

// Controller
#include "../NEATController.h"

// Loop function
#include <argos3/demiurge/loop-functions/CoreLoopFunctions.h>

// GLOBAL VARIABLE
#ifdef MPI_FOUND
MPI::Intercomm g_com;
#endif
auto g_nbProcess = 0;
/**
 * Function that launches the experiment and evaluates this last one.
 * @note: To launch your own program: you can use, for example, system("") which
 * is a combination of fork(), exec() and waitpid().
 */
void launchARGoSAndEvaluate(NEAT::Population& pop, unsigned int num_runs_per_gen) {

    // Check
    if (num_runs_per_gen == 0)
        return;

    std::cout << "numgen: " << NEAT::num_gens << std::endl;
    std::cout << "numruns: " << NEAT::num_runs << std::endl;

    // The CSimulator class of ARGoS is a singleton.
    static auto& cSimulator = argos::CSimulator::GetInstance();
    // Gets a reference to the loop functions (the evaluation is done by the loop fct)
    static auto& cLoopFunctions = dynamic_cast<CoreLoopFunctions&>(cSimulator.GetLoopFunctions());

    // Produces the different random seeds for the experiment
    auto* pRNG = argos::CRandom::CreateRNG("neat");
    std::vector<UInt32> vecRandomSeed;
    for (unsigned int i = 0; i < num_runs_per_gen; i++) {
        vecRandomSeed.push_back(pRNG->Uniform(argos::CRange<UInt32>(0, UINT32_MAX)));
    }

    int i = 0;
    for (auto org : pop.organisms) {
        std::cout << "\nOrganism[" << i << "]: " << std::endl;

        // Sets the neural network
        auto cEntities = cSimulator.GetSpace().GetEntitiesByType("controller");
        for (auto ent : cEntities) {
            auto* pcEntity = any_cast<CControllableEntity*>(ent.second);
            try {
                auto& cController = dynamic_cast<CEPuckNEATController&>(pcEntity->GetController());
                cController.SetNetwork(*(org)->net);
            } catch (std::exception& ex) {
                LOGERR << "Error while setting network: " << ex.what() << std::endl;
            }
        }

        // Launches the experiment with the specified random seed, and get the score
        auto dPerformance = 0.0;
        std::cout << "--> number runs per gen = " << num_runs_per_gen << std::endl;

        for (auto seed : vecRandomSeed) {
            std::cout << "Random seed: " << seed << std::endl;
            cSimulator.SetRandomSeed(seed);
            cSimulator.Reset();
            cSimulator.Execute();
            dPerformance += cLoopFunctions.GetObjectiveFunction();
        }
        // Computes the average performance
        if (num_runs_per_gen > 0) {
            dPerformance /= num_runs_per_gen;
        }

        // The evaluation of this network is stored in the quality score
        org->fitness = dPerformance;

        std::cout << "--> Average fitness = " << org->fitness << std::endl;
    }
}

/**
 * Function that launches in parallel (with MPI) the experiment for each
 * organism in the population and evaluates each one.
 */
#ifdef MPI_FOUND
void launchARGoSInParallelAndEvaluate(NEAT::Population& pop, unsigned int num_runs_per_gen) {
    // Check
    if (num_runs_per_gen == 0)
        return;

    // Produces the different random seeds for the experiment, initialized with the clock
    auto* pRNG = argos::CRandom::CreateRNG("neat");
    std::vector<UInt32> vecRandomSeed;
    for (unsigned int i = 0; i < num_runs_per_gen; i++) {
        vecRandomSeed.push_back(pRNG->Uniform(argos::CRange<UInt32>(0, UINT32_MAX)));
    }

    // Serialization: Genome -> string
    std::vector<std::string> vecStr;
    for (auto org : pop.organisms) {
        std::stringstream s;
        NEAT::Genome* g = org->gnome;
        g->print_to_file(s);
        auto str = s.str();
        auto pos = str.find("\n") + 1;
        str = str.substr(pos, str.find("genomeend") - pos);
        vecStr.push_back(str);
    }

    // MPI: Sends the random seed and the genome & Receives the fitness.
    unsigned int nId = 0;
    unsigned int nId1 = 0;
    auto nSize = vecStr.size();
    auto nGroup = ceil((double)nSize / g_nbProcess);
    MPI::Status status;
    auto nSource = 0;
    auto cont = true;
    auto dFitness = 0.0;
    std::vector<double> vecMedBehaviorCharac;

    // The following for-loop takes into account the case where the #Processes ≠ #Organisms.
    for (auto i = 0; i < nGroup; i++) {
        // Sends the random seed and the genome (string) to all the child processes.
        for (auto j = 0; (j < g_nbProcess) && (nId < nSize); j++, nId++) {
            g_com.Send(&cont, 1, MPI::BOOL, j, 1);
            g_com.Send(&vecRandomSeed[0], vecRandomSeed.size(), MPI::UNSIGNED, j, 1);
            g_com.Send(vecStr[nId].c_str(), vecStr[nId].length(), MPI::CHAR, j, 1);
        }
        // Receives the result of each process and store it in the fitness of each organism
        for (auto j = 0; (j < g_nbProcess) && (nId1 < nSize); j++, nId1++) {
            g_com.Recv(&dFitness, 1, MPI::DOUBLE, MPI::ANY_SOURCE, MPI::ANY_TAG, status);
            nSource = status.Get_source();
            (pop.organisms[nSource + (nId1 - j)])->fitness = dFitness;
        }
    }
}
#endif

/**
 * Main program.
 */
int main(int argc, char* argv[]) {

    // Checks the arguments
    if (argc < 4) {
        std::cerr << "Arg1: A configuration file (.argos or .xml) is required for ARGoS."
                  << std::endl;
        std::cerr << "Arg2: A NEAT parameters file (.ne file) is required to run "
                     "the experiments."
                  << std::endl;
        std::cerr << "Arg3: A starter genome file is required for the creation of "
                     "the initial population."
                  << std::endl;
        std::cerr << "Arg4 (optional): The number (natural number) of processes "
                     "you want to launch. If none is specified, or if it's 0 or 1, "
                     "there will be only one process."
                  << std::endl;
        std::cerr << "Arg5 (non-optional if more than 1process!): the name of the "
                     "binary file to launch in parallel."
                  << std::endl;
        std::cerr << "Arg6 (optional): The seed (unsigned integer) used to "
                     "initialize NEAT. If none is specified, then NEAT will "
                     "initialize with the current time"
                  << std::endl;
        return -1;
    }

    // Checks the number of processes to launch
    if (argc > 5) {
        try {
            g_nbProcess = atoi(argv[4]); // should check if it's negative
        } catch (const std::invalid_argument& err) {
            std::cerr << "Invalid argument: " << err.what() << std::endl;
            std::cerr << "There will be only one process!" << std::endl;
        }
    }

    // Checks the seed (if it exists)
    auto seed = 0;
    if (argc >= 7) {
        try {
            seed = atoi(argv[6]);
        } catch (const std::invalid_argument& err) {
            std::cerr << "Invalid argument: " << err.what() << std::endl;
            std::cerr << "The seed will be left at 0!" << std::endl;
        }
    }

    // Initializes the random number generator
    time_t t;
    if (seed == 0) {
        srand((unsigned)time(&t));
    } else {
        srand(seed);
    }
    auto g_unRandomSeed = rand();

    argos::CRandom::CreateCategory("neat", g_unRandomSeed);

    // Launches the program
    if (g_nbProcess > 1) { // in parallel
#ifdef MPI_FOUND
        std::cout << "PARALLEL RUN" << std::endl;
        // Initializes the MPI execution environment.
        MPI::Init();

        // Spawns a number of identical binaries.
        g_com = MPI::COMM_WORLD.Spawn(argv[5], (const char**)argv, g_nbProcess, MPI::Info(), 0);

        // Launches NEAT with the specified experiment
        launchNEAT(argv[2], argv[3], launchARGoSInParallelAndEvaluate, seed);

        // Sends a signal to terminate the children.
        std::cout << "Parent: Terminate children" << std::endl;
        auto cont = false;
        for (int j = 0; (j < g_nbProcess); j++) {
            g_com.Send(&cont, 1, MPI::BOOL, j, 1);
        }

        // Terminates MPI execution environment.
        MPI_Finalize();
#else
        std::cout << "CANNOT RUN IN PARALELL" << std::endl;
        std::cout << "Please compile with MPI support or set number of process to 1." << std::endl;
#endif
    } else { // sequential

        std::cout << "SEQUENTIAL RUN" << std::endl;

        // Initialization of ARGoS
        auto& cSimulator = argos::CSimulator::GetInstance();
        argos::CDynamicLoading::LoadAllLibraries();
        cSimulator.SetExperimentFileName(argv[1]);
        cSimulator.LoadExperiment();

        // Launches NEAT with the specified experiment
        launchNEAT(argv[2], argv[3], launchARGoSAndEvaluate, seed);
        std::cout << "/*   End of evolution...   */" << std::endl;
        // Disposes of ARGoS stuff
        cSimulator.Destroy();
    }

    if (CRandom::ExistsCategory("neat"))
        CRandom::RemoveCategory("neat");

    return 0;
}
