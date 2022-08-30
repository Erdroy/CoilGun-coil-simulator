#include <thread>
#include "CoilGunSim.h"
#include "CoilGen.h"

constexpr int num_threads = 1;

clock_t Now;

#define PRINT_TIME() printf("Time: %.2fs\n", static_cast<double>(clock() - Now) / CLOCKS_PER_SEC)

void ThreadWorker(const CoilGunSim::SimParameters* coil, const int coilId, const int threadId)
{
    // Add index to the file name
    char fileName[_MAX_FNAME];
    sprintf_s(fileName, "temp%d.fem", threadId);
            
    CoilGunSim sim = {};
    sim.EnableLogging = false;
    const auto parameters = *coil;

    printf("Simulating coil%d - %s\n",
        coilId,
        parameters.GetCoilName().c_str()
    );
                
    const auto simData = sim.Simulate(fileName, parameters);

    const auto descFile = parameters.GetCoilName() + ".json";
    const auto dataFile = parameters.GetCoilName() + ".csv";

    // TODO: Write-out the simulation data

    printf(".");
}

void SimulateVariants(const int numCoils, const std::vector<CoilGunSim::SimParameters>& coils)
{
    for(int coilId = 0; coilId < numCoils; coilId += num_threads)
    {
        auto maxCoils = coilId + num_threads;
        maxCoils = std::min(maxCoils, numCoils);
        const auto batchSize = maxCoils - coilId;
        
        printf("Starting to simulate more coils. Progress: %d/%d (+%d)\n", coilId, numCoils, batchSize);

        // TODO: ThreadPool-based worker
        
        // Run 24 Sim instances in parallel
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        for (int threadId = 0; threadId < batchSize; threadId++)
        {
            threads.emplace_back(std::thread([=]
            {
                ThreadWorker(&coils[coilId + threadId], threadId + coilId, threadId);
            }));
        }

        // Wait for all threads to finish
        for (auto& thread : threads)
            thread.join();
        
        printf("[DONE] Generated %d coil variants. ", batchSize);
        PRINT_TIME();
    }
}

int main(int argc, char** argv)
{
    Now = clock();

    int numCoils = 0;
    const auto coils = GetCoilVariants(&numCoils);

    printf("Generated %d coil variants, took: ", numCoils);
    PRINT_TIME();

    // CoilGunSim sim = {};
    // sim.EnableLogging = true;
    // CoilGunSim::SimParameters parameters;
    // parameters.CoilLength = 45.0;
    // parameters.CoilInnerDiameter = 7.0;
    // parameters.CoilWireTurns = 220;
    // parameters.CoilWireWidth = 0.9;
    // parameters.ProjectileDiameter = 4.5;
    // parameters.ProjectileLength = 45.0;
    // const auto simData = sim.Simulate("temp0.fem", parameters);

    SimulateVariants(numCoils, coils);
    
    printf("Simulated all coils. Simulation time took: ");
    PRINT_TIME();
}
