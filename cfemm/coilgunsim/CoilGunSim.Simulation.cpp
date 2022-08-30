#include "CoilGunSim.h"

CoilGunSim::SimData CoilGunSim::Simulate(const char* fileName, const SimParameters& parameters)
{
    SimData data = {};
    
    constexpr int currents[] = { 10, 100, 1000 };
    constexpr int defaultCurrent = 5;
    
    m_api = {};
    m_api.femm_init(fileName);

    // TODO: Coil shelling option (with shell thickness)
    // TODO: Coil shape option (default, pointed [45 degrees], ball with hollow variants [default-hollow, ball-hollow etc.])

    CgsConfigure(parameters);
    CgsCreateBoundary(parameters);
    CgsCreateCoil(data, parameters);
    
    CgsCreateProjectile(data, parameters);
    
    // Calculate the maximal distance that the projectile can travel inside the boundary height
    const auto maxSteps = parameters.BoundaryHeight - parameters.ProjectileLength / 2 - 1; // -1 step (1 mm) as we have to make sure that projectile does not get out of bounds

    // Move the projectile to it's maximal position
    // This is needed, so we simulate the raw inductance correctly, as it might be a bit different, when
    // projectile is out of bounds/not yet crated.
    FemmExtensions::MoveGroup(m_api, 0, -maxSteps, GROUP_PROJECTILE);
    
    // Integral a inductance
    auto rawInductance = FemmExtensions::IntegrateInductance(m_api, "Coil", defaultCurrent, "temp.fem");

    // Move the projectile back to the center
    FemmExtensions::MoveGroup(m_api, 0, maxSteps, GROUP_PROJECTILE);
    
    // TODO: We might want to add inductance sim precision, to apply better denoising filters later
    // TODO: Inductance denoising (low-pass FIR)
    
    for(int stepIdx = 0; stepIdx < maxSteps; stepIdx++)
    {
        constexpr double inductanceThreshold = 0.25; // Around 0.11uH of difference is small enough, to just stop the inductance mapping
        
        auto inductance = FemmExtensions::IntegrateInductance(m_api, "Coil", defaultCurrent, "temp.fem");
        if (EnableLogging) printf("%dmm Inductance=%.1fuH (raw: %.1fuH)\n", stepIdx, inductance.Abs(), rawInductance.Abs());

        // Add inductance to the current vector, to feed that later into sim data steps.
        SimData::StepData step = {};
        step.Inductance = inductance.Abs();
        data.Steps.push_back(step);

        // Stop simulation when the inductance is close to the raw coil inductance
        if((rawInductance - inductance).Abs() <= inductanceThreshold)
            break;

        FemmExtensions::MoveGroup(m_api, 0, -1, GROUP_PROJECTILE);
    }
    
    data.NumSteps = static_cast<int>(data.Steps.size());
    
    // Reverse the steps
    std::reverse(data.Steps.begin(), data.Steps.end());
    
    // Copy currents into sim data
    for (const int current : currents)
        data.Currents.push_back(current);
    
    // Note: The projectile is now steps away from the coil, so we don't have to move it
    
    for (int i = 0; i < data.NumSteps; i++)
    {
        auto& step = data.Steps[i];
        step.Distance = static_cast<double>(data.NumSteps - (i + 1));
        
        for (const int current : currents)
        {
            auto force = FemmExtensions::IntegrateBlockForce(m_api, "Coil", current, GROUP_PROJECTILE, "temp.fem");
            
            if (EnableLogging) printf("%dmm %dA Force=%.1fN\n", data.NumSteps - (i + 1), current, force.Abs());

            step.Forces.push_back(force.Abs());
        }
        
        FemmExtensions::MoveGroup(m_api, 0, 1, GROUP_PROJECTILE);
    }
    
    m_api.femm_save(fileName);
    m_api.femm_close();

    return data;
}
