#include <thread>
#include "CoilGunSim.h"
#include "CoilGen.h"

constexpr int num_threads = 20;
clock_t now;

#define PRINT_TIME() printf("Time: %.2fs\n", static_cast<double>(clock() - now) / CLOCKS_PER_SEC)

void SimulateVariants(const int numCoils, const std::vector<CoilGunSim::SimParameters>& coils)
{
    for(int coilId = 0; coilId < numCoils; coilId += num_threads)
    {
        auto imax = coilId + num_threads;
        imax = std::min(imax, numCoils);
        const auto threadCount = imax - coilId;
        
        printf("Starting to simulate coils. Progress: %d/%d (+%d)\n", coilId, numCoils, threadCount);
        printf("Simulating");
        
        // Run 24 Sim instances in parallel
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        for (int threadId = 0; threadId < threadCount; threadId++)
        {
            threads.emplace_back(std::thread([=]
            {
                // Add index to the file name
                char fileName[_MAX_FNAME];
                sprintf_s(fileName, "temp%d.fem", threadId);
            
                CoilGunSim sim = {};
                sim.EnableLogging = false;
                const auto parameters = coils[coilId];
                const auto simData = sim.Simulate(fileName, parameters);

                // TODO: Write-out the simulation data

                printf(".");
            }));
        }

        // Wait for all threads to finish
        for (auto& thread : threads)
            thread.join();
        
        printf(" DONE\n");
        printf("Generated %d coil variants, time: ", numCoils);
        PRINT_TIME();
    }
}

int main(int argc, char** argv)
{
    now = clock();

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
    printf("Time: %.2fs\n", static_cast<double>(clock() - now) / CLOCKS_PER_SEC);
}
