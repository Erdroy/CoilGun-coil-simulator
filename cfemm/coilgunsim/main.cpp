#include "CoilGunSim.h"
#include "CoilGen.h"
#include "ThreadPool.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#define PRINT_TIME() printf("Time: %.2fs\n", static_cast<double>(clock() - g_now) / CLOCKS_PER_SEC)

constexpr uint32_t num_threads = 20u;

clock_t g_now;
uint32_t g_skippedCoils = 0u;
ThreadPool g_threadPool = {};

bool CoilIsDone(const CoilGunSim::SimParameters& parameters)
{
    const auto outputFileNameJSON = "./Data/" + parameters.GetPairName() + ".json";
    const auto outputFileNameCSV = "./Data/" + parameters.GetPairName() + ".csv";
    
    FILE* file = nullptr;
    fopen_s(&file, outputFileNameJSON.c_str(), "r");
     
    if (file == nullptr)
        return false;
    
    fclose(file);
    
    file = nullptr;
    fopen_s(&file, outputFileNameCSV.c_str(), "r");
    
    if (file == nullptr)
        return false;
    
    fclose(file);

    return true;
}

void WriteDataToFile(const CoilGunSim::SimParameters& parameters, const CoilGunSim::SimData& data)
{
    const auto numCurrents = static_cast<int>(data.Currents.size());

    // Write Json file
    const auto outputFileNameJSON = "./Data/" + parameters.GetPairName() + ".json";

    FILE* file = nullptr;
    fopen_s(&file, outputFileNameJSON.c_str(), "w");

    fprintf(file, "{\n");
    fprintf(file, "\t\"Name\": \"%s\",\n", parameters.GetPairName().c_str());

    // Write Currents
    fprintf(file, "\t\"Currents\": [\n");
    for (int i = 0; i < numCurrents; ++i)
    {
        fprintf(file, "\t\t%.2d", data.Currents[i]);

        if (i < numCurrents - 1)
        {
            fprintf(file, ",");
        }

        fprintf(file, "\n");
    }
    fprintf(file, "\t],\n");

    // Write Coil data
    fprintf(file, "\t\"CoilData\": {\n");
    fprintf(file, "\t\t\"Name\": \"%s\",\n", parameters.GetCoilName().c_str());
    fprintf(file, "\t\t\"Resistance\": %f,\n", data.Coil.Resistance);
    fprintf(file, "\t\t\"Length\": %f,\n", parameters.CoilLength);
    fprintf(file, "\t\t\"InnerDiameter\": %f,\n", parameters.GetCoilInnerDiameter());
    fprintf(file, "\t\t\"Height\": %f,\n", data.Coil.Height);
    fprintf(file, "\t\t\"WireDiameter\": %f,\n", parameters.CoilWireDiameter);
    fprintf(file, "\t\t\"WireTurns\": %d,\n", parameters.CoilWireTurns);
    fprintf(file, "\t\t\"WireLength\": %f,\n", data.Coil.WireLength);
    fprintf(file, "\t\t\"Layers\": %f,\n", parameters.GetCoilLayers());
    fprintf(file, "\t\t\"TurnsPerLayer\": %f,\n", parameters.GetCoilTurnsPerLayer());
    fprintf(file, "\t\t\"ShellWidth\": %f\n", parameters.CoilShellWidth);
    fprintf(file, "\t},\n");

    // Write Projectile data
    fprintf(file, "\t\"ProjectileData\": {\n");
    fprintf(file, "\t\t\"Name\": \"%s\",\n", parameters.GetProjectileName().c_str());
    fprintf(file, "\t\t\"Mass\": %f,\n", data.Projectile.Mass);
    fprintf(file, "\t\t\"Length\": %f,\n", parameters.ProjectileLength);
    fprintf(file, "\t\t\"Diameter\": %f,\n", parameters.ProjectileDiameter);
    fprintf(file, "\t\t\"MaterialType\": \"%s\",\n", parameters.ProjectileMaterialType);
    fprintf(file, "\t\t\"MaterialDensity\": %f,\n", parameters.ProjectileMaterialDensity);
    fprintf(file, "\t\t\"Shape\": %d,\n", parameters.ProjectileShape);
    fprintf(file, "\t\t\"HoleDiameter\": %f,\n", parameters.ProjectileHoleDiameter);
    fprintf(file, "\t\t\"HoleLength\": %f\n", parameters.ProjectileHoleLength);
    fprintf(file, "\t}\n");
    
    fprintf(file, "}");
    
    fflush(file);
    fclose(file);
    
    // Write CSV
    file = nullptr;
    const auto outputFileNameCSV = "./Data/" + parameters.GetPairName() + ".csv";
    fopen_s(&file, outputFileNameCSV.c_str(), "w");
    
    // Write step header
    fprintf(file, "%s", "Distance, Inductance, ");
    for (auto i = 0; i < numCurrents; ++i)
    {
        fprintf(file, "Force@%dA", data.Currents[i]);
        if (i < numCurrents - 1)
            fprintf(file, ", ");
    }
    fprintf(file, "\n");
    
    // Write-out all of the simulation steps in the reverse order
    for (auto&& step : data.Steps)
    {
        // Write step Distance, Inductance and forces array
        fprintf(file, "%f, %f, ", step.Distance, step.Inductance);
        const auto numForces = static_cast<int>(step.Forces.size());
        for (auto i = 0; i < numForces; ++i)
        {
            fprintf(file, "%f", step.Forces[i]);
            if (i < numForces - 1)
                fprintf(file, ", ");
        }
        fprintf(file, "\n");
    }

    fflush(file);
    fclose(file);
}

void SimulateSingle()
{
    CoilGunSim sim = {};
    sim.EnableLogging = true;
    CoilGunSim::SimParameters parameters;
    parameters.ProjectileMaterialType = "M-50";
    parameters.CoilLength = 50.0;
    parameters.CoilWireTurns = 220;
    parameters.CoilWireDiameter = 0.9;
    parameters.ProjectileDiameter = 4.5;
    parameters.ProjectileLength = 35.0;
    parameters.CoilShellWidth = 0.0;
    parameters.BoreWallWidth = 1.0;
    const auto data = sim.Simulate("temp0.fem", parameters);
    // Write the simulation data to a file, inside "Data" folder
    WriteDataToFile(parameters, data);
}

void ThreadWorker(const CoilGunSim::SimParameters* coil, const int coilId, const int numCoils, const uint32_t threadId)
{
    // Add index to the file name
    char fileName[_MAX_FNAME];
    sprintf_s(fileName, "temp%d.fem", threadId);
    
    CoilGunSim sim = {};
    sim.EnableLogging = false;
    const auto parameters = *coil;
    
    printf("Simulating coil %d/%d (skipped %d) - %s\n",
           coilId,
           numCoils,
           g_skippedCoils,
           parameters.GetPairName().c_str()
    );
    
    const auto data = sim.Simulate(fileName, parameters);

    // Write the simulation data to a file, inside "Data" folder
    WriteDataToFile(parameters, data);
    PRINT_TIME();
}

void SimulateVariants(const int numCoils, const std::vector<CoilGunSim::SimParameters>& coils)
{
    constexpr int startCoil = 0; // Change this, if simulation failed and you want to continue from a specific coil
    
    g_threadPool.Start(num_threads);
    
    for(int coilId = startCoil; coilId < numCoils; coilId++)
    {
        // Skip the coil if it's already simulated
        if (CoilIsDone(coils[coilId]))
        {
            printf("Skip coil %d/%d\n", coilId, numCoils);
            g_skippedCoils++;
            continue;
        }

        // Wait if there are too many jobs queued (memory optimization)
        while (g_threadPool.GetNumJobs() > num_threads * 2)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        g_threadPool.QueueJob([=] (const uint32_t threadId) {
            ThreadWorker(&coils[coilId], coilId, numCoils, threadId);
        });
    }
    
    while (g_threadPool.IsBusy()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char** argv)
{
    g_now = clock();

    int numCoils = 0;
    const auto coils = GetCoilVariants(&numCoils);

#ifdef _WIN32
    // Create Data directory if it doesn't exist (using Win32 API)
    CreateDirectory("Data", nullptr);
#endif
    
    printf("Generated %d coil variants, took: ", numCoils);
    PRINT_TIME();

    //SimulateSingle();
    SimulateVariants(numCoils, coils);
    
    printf("Simulated all coils. Simulation time took: ");
    PRINT_TIME();

    system("pause");
}
