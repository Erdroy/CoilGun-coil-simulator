#pragma once

#include "FemmExtensions.h"

class CoilGunSim
{
public:
    struct CoilGunSimData
    {
        // TODO: Inductance, force, resistance, num layers, coil values, projectile values, num steps, etc.
        double Inductance;
        double Force;
        double Resistance;
        
    };
    
private:
    const int CommonGroup = 0;
    const int CoilGroup = 1;
    const int ProjGroup = 2;
    
    const int BoundaryHeight = 150;
    
    const char* WireMaterial = "Wire";
    const char* ProjectileMaterial =  "Projectile";
            
    FemmAPI m_api;
    
private:
    void CgsConfigure()
    {
        // Setup
        m_api.smartmesh(true);
        
        m_api.mi_probdef(0, femm::LengthMillimeters, femm::AXISYMMETRIC, 1e-008, 1, 30);
        
        m_api.mi_getmaterial("Air");
        m_api.mi_getmaterial("1mm");
        m_api.mi_getmaterial(ProjectileMaterialType);

        m_api.mi_modifymaterial("1mm", 0, &WireMaterial);
        m_api.mi_modifymaterial(ProjectileMaterialType, 0, &ProjectileMaterial);
    }

    void CgsCreateCoil(const double length, const double innerDiameter, const int turns, double wireWidth, const double wireCompactFactor = 0.9)
    {
        const auto turnsPerLayer = length / wireWidth;
        const auto numLayers = turns / turnsPerLayer;
        const auto height = numLayers * wireWidth * wireCompactFactor;
        const auto outerDiameter = innerDiameter + height;

        printf("Creating coil (length=%.1f, inner_diameter=%.1f, outer_diameter=%.1f, turns=%d, wire_width=%.2f)...\n", length, innerDiameter, outerDiameter, turns, wireWidth);

        // Apply wire_width (13 = 'WireD')
        m_api.mi_modifymaterial("Wire", 13, &wireWidth);

        // Create circuit
        m_api.mi_addcircprop("Coil", 1, 1);

        const auto x0 = innerDiameter / 2;
        const auto x1 = outerDiameter / 2;
        const auto y0 = length / 2;
        const auto y1 = -length / 2;

        // Calculate the center
        const auto cx = (x0 + x1) / 2;
        const auto cy = (y0 + y1) / 2;
        
        // Add lines
        FemmExtensions::AddLine(m_api, x0, y0, x1, y0, CoilGroup); // Top
        FemmExtensions::AddLine(m_api, x1, y0, x1, y1, CoilGroup); // Right
        FemmExtensions::AddLine(m_api, x1, y1, x0, y1, CoilGroup); // Bottom
        FemmExtensions::AddLine(m_api, x0, y1, x0, y0, CoilGroup); // Left
        
        // Add a 'Wire' block with a 'Coil' circuit
        FemmExtensions::AddBlock(m_api, cx, cy, "Wire", "Coil", CoilGroup, turns);

        m_api.mi_clearselected();
    }

    void CgsCreateProjectile(const double width, const double length)
    {
        // TODO: Different coil shapes (default, angled, ball with hollow variants [default-hollow, ball-hollow etc.])

        const auto halfWidth = width / 2;
        const auto halfLength = length / 2;

        const auto y0 = -halfLength;
        const auto y1 = halfLength;

        // Calculate the center
        const auto cx = halfWidth / 2;
        const auto cy = (y0 + y1) / 2;

        // Add lines
        FemmExtensions::AddLine(m_api, 0, y0, halfWidth, y0, ProjGroup); // Top
        FemmExtensions::AddLine(m_api, halfWidth, y0, halfWidth, y1, ProjGroup); // Right
        FemmExtensions::AddLine(m_api, halfWidth, y1, 0, y1, ProjGroup); // Bottom
        FemmExtensions::AddLine(m_api, 0, y1, 0, y0, ProjGroup); // Left

        // Add a 'Projectile' block
        FemmExtensions::AddBlock(m_api, cx, cy, "Projectile", "", ProjGroup, 0);

        m_api.mi_clearselected();
    }

    /**
     * \brief Creates a boundary for the simulation.
     * \param height The height from the center of the document to the top and bottom arc starts.
     */
    void CgsCreateBoundary(const double height)
    {
        // Add air in a point that is in the free space (5% from the top of the boundary, as projectile goes on the bottom so there is a free space...)
        FemmExtensions::AddBlock(m_api, 0.1, height - height * 0.05, "Air", "", CommonGroup, 0);

        // Create boundary arc
        FemmExtensions::AddLine(m_api, 0, height, 0, -height);
        m_api.mi_addarc(0, -height, 0, height, 180, 16);

        printf("Boundary created with h=%.1f\n", height);

        m_api.mi_clearselected();
    }
    
public:
    const char* ProjectileMaterialType = "M-50";
    
    double CoilLength;
    double CoilInnerDiameter;
    int CoilWireTurns;
    double CoilWireWidth;
    
    double ProjectileDiameter;
    double ProjectileLength;
    
public:
    CoilGunSimData Simulate(const char* fileName)
    {
        CoilGunSimData data = {};
        
        constexpr int currents[] = { 10, 100, 1000 };
        constexpr int defaultCurrent = 5;
        
        m_api = {};
        m_api.femm_init(fileName);

        // TODO: Coil shelling option (with shell thickness)
        // TODO: Coil shape option (default, pointed [45 degrees], ball with hollow variants [default-hollow, ball-hollow etc.])

        CgsConfigure();
        CgsCreateBoundary(BoundaryHeight);
        CgsCreateCoil(CoilLength, CoilInnerDiameter, CoilWireTurns, CoilWireWidth);

        // Integral a inductance, where there is no projectile, yet, to get the raw inductance of the coil
        auto rawInductance = FemmExtensions::IntegrateInductance(m_api, "Coil", defaultCurrent, "temp.fem");
        
        CgsCreateProjectile(ProjectileDiameter, ProjectileLength);
        
        
        int steps = 0;
        // TODO: Calculate the maximal distance that the projectile can travel inside the boundary height
        while(true)
        {
            constexpr double inductanceThreshold = 0.11; // Around 0.1uH of difference is small enough, to just stop the inductance mapping
            
            auto inductance = FemmExtensions::IntegrateInductance(m_api, "Coil", defaultCurrent, "temp.fem");
            printf("Inductance=%.1fuH\n", inductance.Abs());
            FemmExtensions::MoveGroup(m_api, 0, -1, ProjGroup);

            // Stop simulation when the inductance is close to the raw coil inductance
            if((rawInductance - inductance).Abs() <= inductanceThreshold)
                break;
            
            steps++;

            // Make sure that projectile fits the boundary height
            if(steps >= maxSteps)
            {
                printf("Maximum number of steps reached!\n");
                break;
            }
        }
        
        steps += 1; // One extra step to reach the center of the coil
        
        // Note: The projectile is now steps away from the coil, so we don't have to move it
        
        for (int i = 0; i < steps; i++)
        {
            for (const int current : currents)
            {
                auto force = FemmExtensions::IntegrateBlockForce(m_api, "Coil", current, ProjGroup, "temp.fem");
                printf("%dmm %dA Force=%.1fN\n", steps - (i + 1), current, force.Abs());
            }
            
            FemmExtensions::MoveGroup(m_api, 0, 1, ProjGroup);
        }
        
        m_api.femm_save(fileName);
        m_api.femm_close();

        return data;
    }
};
