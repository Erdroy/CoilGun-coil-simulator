#pragma once

#ifndef COILGEN_H
#define COILGEN_H

#include <json.hpp>
#include <xutility>

#include "CoilGunSim.h"

struct PermutationConfig
{
public:
    double BoreWallThickness = 1.0;
    
    double CoilLengthStep = 5.0;
    std::vector<double> CoilLengthRange = {15, 70};

    int CoilTurnStep = 10;
    std::vector<int> CoilTurnRange = {50, 300};
    std::vector<double> CoilWireSizes = { 0.5, 0.9, 1.2 };

    double ProjectileLengthStep = 5;
    std::vector<double> ProjectileLengthRange = { 20, 75 };
    std::vector<double> ProjectileDiameters = { 4.5, 5.5, 8.0, 10.0 };

    void Read(nlohmann::json::const_reference json)
    {
        BoreWallThickness = json["BoreWallThickness"].get<double>();
        CoilLengthStep = json["CoilLengthStep"].get<double>();
        CoilLengthRange = json["CoilLengthRange"].get<std::vector<double>>();
        CoilTurnStep = json["CoilTurnStep"].get<int>();
        CoilTurnRange = json["CoilTurnRange"].get<std::vector<int>>();
        CoilWireSizes = json["CoilWireSizes"].get<std::vector<double>>();
        ProjectileLengthStep = json["ProjectileLengthStep"].get<double>();
        ProjectileLengthRange = json["ProjectileLengthRange"].get<std::vector<double>>();
        ProjectileDiameters = json["ProjectileDiameters"].get<std::vector<double>>();
    }
};

inline std::vector<CoilGunSim::SimParameters> GetCoilVariants(int* numCoils, const PermutationConfig& config)
{
    const auto numWires = static_cast<int>(config.CoilWireSizes.size());
    const auto numCoilLengths = static_cast<int>(ceil((config.CoilLengthRange[1] - config.CoilLengthRange[0]) / config.CoilLengthStep));
    const auto numCoilTurns = static_cast<int>(ceil((config.CoilTurnRange[1] - config.CoilTurnRange[0]) / config.CoilTurnStep));
    const auto numProjectileDiameters = static_cast<int>(config.ProjectileDiameters.size());
    const auto numProjectileLengths = static_cast<int>(ceil((config.ProjectileLengthRange[1] - config.ProjectileLengthRange[0]) / config.ProjectileLengthStep));

    *numCoils = numWires * numCoilLengths * numCoilTurns * numProjectileDiameters * numProjectileLengths;

    std::vector<CoilGunSim::SimParameters> params = {};
    
    for (const double wireSize : config.CoilWireSizes)
    {
        for (int coilLengthIdx = 0; coilLengthIdx < numCoilLengths; coilLengthIdx ++) 
        {
            for (int coilTurnsIdx = 0; coilTurnsIdx < numCoilTurns; coilTurnsIdx ++) 
            {
                for (const double projectileDiameter : config.ProjectileDiameters)
                {
                    for (int projectileLengthIdx = 0; projectileLengthIdx < numProjectileLengths; projectileLengthIdx ++)
                    {
                        const auto coilLength = config.CoilLengthRange[0] + coilLengthIdx * config.CoilLengthStep;
                        const auto coilTurns = config.CoilTurnRange[0] + coilTurnsIdx * config.CoilTurnStep;
                        const auto projectileLength = config.ProjectileLengthRange[0] + projectileLengthIdx * config.ProjectileLengthStep;

                        CoilGunSim::SimParameters parameters;
                        parameters.CoilLength = coilLength;
                        parameters.CoilWireTurns = coilTurns;
                        parameters.CoilWireDiameter = wireSize;
                        parameters.ProjectileDiameter = projectileDiameter;
                        parameters.ProjectileLength = projectileLength;

                        // Not varied:
                        parameters.BoreWallWidth = config.BoreWallThickness;
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
