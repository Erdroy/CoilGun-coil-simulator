#include <thread>
#include "CoilGunSim.h"

constexpr int num_threads = 1;

int main(int argc, char** argv)
{
    const clock_t now = clock();
    
    // Run 24 Sim instances in parallel
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; i++)
    {
        threads.emplace_back(std::thread([i]
        {
            // Add index to the file name
            char fileName[_MAX_FNAME];
            sprintf_s(fileName, "temp%d.fem", i);
            
            CoilGunSim sim = {};
            CoilGunSim::SimParameters parameters;
            parameters.CoilLength = 45.0;
            parameters.CoilInnerDiameter = 7.0;
            parameters.CoilWireTurns = 220;
            parameters.CoilWireWidth = 0.9;
            parameters.ProjectileDiameter = 4.5;
            parameters.ProjectileLength = 45.0;
            
            const auto simData = sim.Simulate(fileName, parameters);

            // TODO: Create file for this coil and save the data from the simulation

        }));
    }

    // Wait for all threads to finish
    for (auto& thread : threads)
        thread.join();

    const clock_t end = clock();
    printf("Time: %.2fs\n", static_cast<double>(end - now) / CLOCKS_PER_SEC);
}