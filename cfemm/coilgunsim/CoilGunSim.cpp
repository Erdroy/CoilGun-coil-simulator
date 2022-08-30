﻿#include "CoilGunSim.h"

#include <femmconstants.h>

#define COPPER_WIRE_RESISTANCE 1.68e-8

void CoilGunSim::CalculateCoilHeightAndLayers(SimData& data, const SimParameters& parameters) const
{
    const double wireDiameter = parameters.CoilWireWidth;
    
    const auto turnsPerLayer = parameters.CoilLength / parameters.CoilWireWidth;
    const auto numLayers = parameters.CoilWireTurns / turnsPerLayer;
    const auto height = numLayers * wireDiameter * parameters.WireCompactFactor;
    
    data.Coil.Height = height;
    data.Coil.Layers = ceil(numLayers);
}

void CoilGunSim::CalculateCoilWireLength(SimData& data, const SimParameters& parameters) const
{
    const double coilRadius = parameters.CoilInnerDiameter / 2;
    const double wireDiameter = parameters.CoilWireWidth;
    const auto turnsPerLayer = parameters.CoilLength / parameters.CoilWireWidth;
    const auto numLayers = parameters.CoilWireTurns / turnsPerLayer;
    
    // Calculate the length of the wire
    auto wireLength = 0.0;
    for(int layer = 0; layer <  static_cast<int>(floor(numLayers)); layer++)
    {
        const auto layerRadius = coilRadius + layer * wireDiameter * parameters.WireCompactFactor;
        wireLength += turnsPerLayer * PI * 2 * layerRadius;
    }
    const auto layerRadius = coilRadius + numLayers * wireDiameter * parameters.WireCompactFactor;
    wireLength += turnsPerLayer * PI * 2 * layerRadius * (numLayers - floor(numLayers));
    wireLength /= 1000.0;

    data.Coil.WireLength = wireLength;
}

void CoilGunSim::CalculateCoilResistance(SimData& data, const SimParameters& parameters) const
{
    const auto wireRadius = parameters.CoilWireWidth / 2;
    
    data.Coil.Resistance = COPPER_WIRE_RESISTANCE * data.Coil.WireLength / (PI * (wireRadius / 1000.0) * (wireRadius / 1000.0));
}

void CoilGunSim::CgsConfigure(const SimParameters& parameters)
{
    // Setup
    m_api.smartmesh(true);
        
    m_api.mi_probdef(0, femm::LengthMillimeters, femm::AXISYMMETRIC, 1e-008, 1, 30);
        
    m_api.mi_getmaterial("Air");
    m_api.mi_getmaterial("1mm");
    m_api.mi_getmaterial("1mm");
    m_api.mi_getmaterial(parameters.ProjectileMaterialType);

    m_api.mi_modifymaterial("1mm", 0, MATERIAL_WIRE);
    m_api.mi_modifymaterial(parameters.ProjectileMaterialType, 0, MATERIAL_PROJECTILE);
}

void CoilGunSim::CgsCreateBoundary(const SimParameters& parameters)
{
    // Create multi-layer boundary, as this will decrease the inductance integration noise 
    m_api.mi_makeABC(parameters.BoundaryLayers, parameters.BoundaryHeight, 0);
        
    // Add air in a point that is in the free space (5% from the top of the inner boundary layer, as projectile goes on the bottom so there is always a free space...)
    FemmExtensions::AddBlock(m_api, 0.1, parameters.BoundaryHeight - parameters.BoundaryHeight * 0.05, "Air", "", GROUP_COMMON, 0);
         
    if (EnableLogging) printf("Boundary created with r=%.1f\n", parameters.BoundaryHeight);
    m_api.mi_clearselected();
}

void CoilGunSim::CgsCreateCoil(SimData& data, const SimParameters& parameters)
{
    double wireDiameter = parameters.CoilWireWidth;

    CalculateCoilHeightAndLayers(data, parameters);
    CalculateCoilWireLength(data, parameters);
    CalculateCoilResistance(data, parameters);
    
    const auto outerDiameter = parameters.CoilInnerDiameter + data.Coil.Height;
    
    if (EnableLogging) printf("Creating coil (length=%.1f, inner_diameter=%.1f, outer_diameter=%.1f, turns=%d, wire_width=%.2f)...\n",
        parameters.CoilLength,
        parameters.CoilInnerDiameter,
        outerDiameter,
        parameters.CoilWireTurns,
        wireDiameter);

    // Apply wire_width (13 = 'WireD')
    m_api.mi_modifymaterial("Wire", 13, &wireDiameter);

    // Create circuit
    m_api.mi_addcircprop("Coil", 1, 1);

    const auto x0 = parameters.CoilInnerDiameter / 2;
    const auto x1 = outerDiameter / 2;
    const auto y0 = parameters.CoilLength / 2;
    const auto y1 = -parameters.CoilLength / 2;

    // Calculate the center
    const auto cx = (x0 + x1) / 2;
    const auto cy = (y0 + y1) / 2;
        
    // Add lines
    FemmExtensions::AddLine(m_api, x0, y0, x1, y0, GROUP_COIL); // Top
    FemmExtensions::AddLine(m_api, x1, y0, x1, y1, GROUP_COIL); // Right
    FemmExtensions::AddLine(m_api, x1, y1, x0, y1, GROUP_COIL); // Bottom
    FemmExtensions::AddLine(m_api, x0, y1, x0, y0, GROUP_COIL); // Left
        
    // Add a 'Wire' block with a 'Coil' circuit
    FemmExtensions::AddBlock(m_api, cx, cy, "Wire", "Coil", GROUP_COIL, parameters.CoilWireTurns);

    m_api.mi_clearselected();
}

void CoilGunSim::CgsCreateProjectile(SimData& data, const SimParameters& parameters)
{
    // TODO: Different coil shapes (default, angled, ball with hollow variants [default-hollow, ball-hollow etc.])

    const auto radius = parameters.ProjectileDiameter / 2;
    const auto area = PI * (radius * radius);
    const auto volume = (area * parameters.ProjectileLength) / 1000; // cm^3
    data.Projectile.Mass = volume * parameters.ProjectileMaterialDensity; // mass in grams
    
    const auto halfWidth = parameters.ProjectileDiameter / 2;
    const auto halfLength = parameters.ProjectileLength / 2;

    const auto y0 = -halfLength;
    const auto y1 = halfLength;

    // Calculate the center
    const auto cx = halfWidth / 2;
    const auto cy = (y0 + y1) / 2;

    // Add lines
    FemmExtensions::AddLine(m_api, 0, y0, halfWidth, y0, GROUP_PROJECTILE); // Top
    FemmExtensions::AddLine(m_api, halfWidth, y0, halfWidth, y1, GROUP_PROJECTILE); // Right
    FemmExtensions::AddLine(m_api, halfWidth, y1, 0, y1, GROUP_PROJECTILE); // Bottom
    FemmExtensions::AddLine(m_api, 0, y1, 0, y0, GROUP_PROJECTILE); // Left

    // Add a 'Projectile' block
    FemmExtensions::AddBlock(m_api, cx, cy, "Projectile", "", GROUP_PROJECTILE, 0);

    m_api.mi_clearselected();
}
