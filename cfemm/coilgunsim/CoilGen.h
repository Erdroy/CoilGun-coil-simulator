#pragma once

#ifndef COILGEN_H
#define COILGEN_H

#include <xutility>

#include "CoilGunSim.h"

// All values are in mm

constexpr double BORE_WALL_THICKNESS = 1.0;

// Ranges
constexpr double COIL_LENGTH_STEP = 5.0;
constexpr double COIL_LENGTH_RANGE[] = {15, 70};

constexpr int COIL_TURN_STEP = 10; // turns
constexpr int COIL_TURN_RANGE[] = {50, 300}; // turns

constexpr double PROJECTILE_LENGTH_STEP = 5;
constexpr double PROJECTILE_LENGTH_RANGE[] = { 20, 75 };

// Values
constexpr double COIL_WIRE_SIZES[] = { 0.5, 0.9, 1.2 };
constexpr double PROJECTILE_DIAMETERS[] = { 4.5, 5.5, 8.0, 10.0 };

// Not used:
constexpr CoilGunSim::ProjectileShape PROJECTILE_TYPES[] = {
    CoilGunSim::ProjectileShape::Cylinder,
    CoilGunSim::ProjectileShape::Ball,
    CoilGunSim::ProjectileShape::Angled45
};

inline std::vector<CoilGunSim::SimParameters> GetCoilVariants(int* numCoils)
{
    constexpr auto numWires = static_cast<int>(std::size(COIL_WIRE_SIZES));
    const auto numCoilLengths = static_cast<int>(ceil((COIL_LENGTH_RANGE[1] - COIL_LENGTH_RANGE[0]) / COIL_LENGTH_STEP));
    const auto numCoilTurns = static_cast<int>(ceil((COIL_TURN_RANGE[1] - COIL_TURN_RANGE[0]) / COIL_TURN_STEP));
    constexpr auto numProjectileDiameters = static_cast<int>(std::size(PROJECTILE_DIAMETERS));
    const auto numProjectileLengths = static_cast<int>(ceil((PROJECTILE_LENGTH_RANGE[1] - PROJECTILE_LENGTH_RANGE[0]) / PROJECTILE_LENGTH_STEP));

    *numCoils = numWires * numCoilLengths * numCoilTurns * numProjectileDiameters * numProjectileLengths;

    std::vector<CoilGunSim::SimParameters> params = {};
    
    for (const double wireSize : COIL_WIRE_SIZES)
    {
        for (int coilLengthIdx = 0; coilLengthIdx < numCoilLengths; coilLengthIdx ++) 
        {
            for (int coilTurnsIdx = 0; coilTurnsIdx < numCoilTurns; coilTurnsIdx ++) 
            {
                for (const double projectileDiameter : PROJECTILE_DIAMETERS)
                {
                    for (int projectileLengthIdx = 0; projectileLengthIdx < numProjectileLengths; projectileLengthIdx ++)
                    {
                        const auto coilLength = COIL_LENGTH_RANGE[0] + coilLengthIdx * COIL_LENGTH_STEP;
                        const auto coilTurns = COIL_TURN_RANGE[0] + coilTurnsIdx * COIL_TURN_STEP;
                        const auto projectileLength = PROJECTILE_LENGTH_RANGE[0] + projectileLengthIdx * PROJECTILE_LENGTH_STEP;

                        CoilGunSim::SimParameters parameters;
                        parameters.CoilLength = coilLength;
                        parameters.CoilWireTurns = coilTurns;
                        parameters.CoilWireDiameter = wireSize;
                        parameters.ProjectileDiameter = projectileDiameter;
                        parameters.ProjectileLength = projectileLength;

                        // Not varied:
                        parameters.BoreWallWidth = BORE_WALL_THICKNESS;
                        // TODO: Hollowed projectiles, projectile shapes, projectile materials and coil shelling
                        
                        params.push_back(parameters);
                    }
                }
           }
        }
    }
    
    return params;
}

#endif // COILGEN_H
