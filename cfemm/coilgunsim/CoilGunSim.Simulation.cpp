#include "CoilGunSim.h"

// TODO: Coil shape option
// TODO: Projectile shape option (default, pointed [45 degrees], ball with hollow variants [default-hollow, ball-hollow etc.])

CoilGunSim::SimData CoilGunSim::Simulate(const char* fileName, const SimParameters& parameters)
{
    SimData data = {};
    // Calculate the maximal distance that the projectile can travel inside the boundary height
    data.NumSteps = 100;
    
    constexpr int currents[] = { 10, 100, 1000 };
    constexpr int numCurrents = std::size(currents);
    constexpr int defaultCurrent = 5;
    
    m_api = {};
    m_api.femm_init(fileName);

    CgsConfigure(parameters);
    CgsCreateBoundary(parameters);
    CgsCreateCoil(data, parameters);
    
    CgsCreateProjectile(data, parameters);

    // Move the projectile to it's maximal position
    // This is needed, so we simulate the raw inductance correctly, as it might be a bit different, when
    // projectile is out of bounds/not yet crated.
    FemmExtensions::MoveGroup(m_api, 0, -data.NumSteps, GROUP_PROJECTILE);
    
    // Integral a inductance
    auto rawInductance = FemmExtensions::IntegrateInductance(m_api, "Coil", defaultCurrent, "temp.fem");

    // Move the projectile back to the center
    // Note: The boundary is at 150mm from the center, so we have max 100mm projectile length limit at 100 steps (100mm + 100mm / 2 < 150mm)
    FemmExtensions::MoveGroup(m_api, 0, data.NumSteps, GROUP_PROJECTILE);
    
    // Fill all the steps, with 0N forces and raw inductance
    for(int stepIdx = 0; stepIdx < data.NumSteps; stepIdx++)
    {
        SimData::StepData step = {};
        step.Distance = static_cast<double>(stepIdx);
        step.Inductance = rawInductance.Abs();
        step.Forces = {};
        for (int currentIdx = 0; currentIdx < numCurrents; currentIdx++)
            step.Forces.push_back(0);
        data.Steps.push_back(step);
    }
    
    // Copy currents into sim data
    for (const int current : currents)
        data.Currents.push_back(current);
    
    // Simulate inductance
    for(int stepIdx = 0; stepIdx < data.NumSteps; stepIdx++)
    {
        constexpr double inductanceThreshold = 0.25; // Around 0.11uH of difference is small enough, to just stop the inductance mapping
        
        auto inductance = FemmExtensions::IntegrateInductance(m_api, "Coil", defaultCurrent, "temp.fem");
        if (EnableLogging) printf("%dmm Inductance=%.1fuH (raw: %.1fuH)\n", stepIdx, inductance.Abs(), rawInductance.Abs());

        // Add inductance to the current vector, to feed that later into sim data steps.
        auto& step = data.Steps[stepIdx];
        step.Inductance = inductance.Abs();

        // Stop simulation when the inductance is close to the raw coil inductance
        if((rawInductance - inductance).Abs() <= inductanceThreshold)
        {
            // Reset the projectile group position to 0
            FemmExtensions::MoveGroup(m_api, 0, stepIdx, GROUP_PROJECTILE);
            break;
        }

        FemmExtensions::MoveGroup(m_api, 0, -1, GROUP_PROJECTILE);
    }
    
    // Simulate forces on all the currents
    for (int i = 0; i < data.NumSteps; i++)
    {
        bool reachedForceThreshold = false;
        auto& step = data.Steps[i];

        for (int currentIdx = 0; currentIdx < numCurrents; currentIdx++)
        {
            constexpr double forceThreshold = 0.1; // Around 0.1N of difference is small enough, to just stop the force mapping
            
            const auto current = currents[currentIdx];
            auto force = FemmExtensions::IntegrateBlockForce(m_api, "Coil", current, GROUP_PROJECTILE, "temp.fem");

            // Set the force if the current step and current
            step.Forces[currentIdx] = force.Abs();
            
            if (EnableLogging) printf("%dmm %dA Force=%.1fN\n", i, current, force.Abs());

            // Stop the force sim when projectile is out of the coil and the force falls down to 0N
            if(currentIdx == numCurrents - 1 && step.Distance > parameters.CoilLength && force.Abs() < forceThreshold)
            {
                if (EnableLogging) printf("%dmm reached minimum force, stopping.\n", i);
                reachedForceThreshold = true;
                break;
            }
        }

        // Stop the simulation, as the simulation reached 0N at max current
        if(reachedForceThreshold) break;
        
        FemmExtensions::MoveGroup(m_api, 0, 1, GROUP_PROJECTILE);
    }
    
    m_api.femm_save(fileName);
    m_api.femm_close();

    return data;
}
