#pragma once

#include <femmconstants.h>

#include "FemmExtensions.h"

#pragma region FEMM DATA
#define GROUP_COMMON 0
#define GROUP_COIL 1
#define GROUP_PROJECTILE 2

#define MATERIAL_WIRE "Wire"
#define MATERIAL_PROJECTILE "Projectile"

#define COPPER_WIRE_RESISTANCE 1.68e-8
#pragma endregion

class CoilGunSim
{
public:
    enum class ProjectileShape
    {
        Cylinder = 0,
        Ball,
        Angled45
    };
    
public:
    struct SimParameters
    {
    public:
        int BoundaryLayers = 3;
        double BoundaryHeight = 150.0;

        double WireCompactFactor = 1.1;
        double BoreWallWidth = 1.0;
        
    public:
        double CoilLength = 35.0;
        int CoilWireTurns = 130;
        double CoilWireDiameter = 0.9;
        
    public:
        double ProjectileDiameter = 4.5;
        double ProjectileLength = 45;
        
    public:
        double CoilShellWidth = 0.0;
    
    public:
        /**
         * \brief The diameter of the hole.
         */
        double ProjectileHoleDiameter = 0.0;
            
        /**
         * \brief The length of the hole, starting from the back of the coil.
         */
        double ProjectileHoleLength = 0.0;
        
    public:
        /**
         * \brief The shape of the projectile.
         */
        ProjectileShape ProjectileShape = ProjectileShape::Cylinder;
        
        /**
         * \brief The material of the projectile. Use FEMM to lookup different materials.
         *  The suggested materials are: 'M-50' (steel), '416 Stainless Steel' and 'Mu Metal' (the best one).
         */
        const char* ProjectileMaterialType = "M-50";
        
        /**
         * \brief The material density of the projectile (grams per centimeter cubed)
         */
        double ProjectileMaterialDensity = 7.85;

    public:
        /**
         * \brief Returns coil name in format '[WireD]_C[Length]x[Turns]T-P[Diameter]x[Length]'.
         * \return The coil name string.
         */
        std::string GetPairName() const
        {
            // In the coil name, we have to include all parameters that directly 
            // [WireD]_C[Length]x[Turns]T-P[Diameter]x[Length]
            // example: 0.9_C45.0x220T-P4.5x35.json
            
            // TODO: Add hollow, shape and shielding when done

            return GetCoilName() + "-" + GetProjectileName();
        }
        
        std::string GetProjectileName() const
        {
            // In the coil name, we have to include all parameters that directly 
            // P[Diameter]x[Length]
            // example: P4.5x35
            
            char buffer[256] = {};
            sprintf_s(buffer, "P%.1fx%.0f",
                ProjectileDiameter,
                ProjectileLength
            );

            // TODO: Add hollow, shape and shielding when done

            return buffer;
        }
        
        std::string GetCoilName() const
        {
            // In the coil name, we have to include all parameters that directly 
            // [WireD]_C[Length]x[Turns]T
            // example: 0.9_C45.0x220T
            
            char buffer[256] = {};
            sprintf_s(buffer, "%.1f_C%.1fx%dT",
                CoilWireDiameter,
                CoilLength,
                CoilWireTurns
            );

            // TODO: Add hollow, shape and shielding when done

            return buffer;
        }

        double GetCoilInnerDiameter() const
        {
            return ProjectileDiameter + BoreWallWidth * 2.0;
        }

        double GetCoilTurnsPerLayer() const
        {
            return CoilLength / CoilWireDiameter;
        }
        
        double GetCoilLayers() const
        {
            const auto numLayers = CoilWireTurns / GetCoilTurnsPerLayer();
    
            return numLayers;
        }

        double GetCoilHeight() const
        {
            const double wireDiameter = CoilWireDiameter;
            const auto height = GetCoilLayers() * wireDiameter * WireCompactFactor;
    
            return height;
        }

        double GetCoilWireLength() const
        {
            const double coilRadius = GetCoilInnerDiameter() / 2;
            const double wireDiameter = CoilWireDiameter;
            const auto numLayers = GetCoilLayers();
            const auto turnsPerLayer = GetCoilTurnsPerLayer();
    
            // Calculate the length of the wire
            auto wireLength = 0.0;
            for(int layer = 0; layer < static_cast<int>(floor(numLayers)); layer++)
            {
                const auto layerRadius = coilRadius + layer * wireDiameter * WireCompactFactor;
                wireLength += turnsPerLayer * PI * 2 * layerRadius;
            }
            const auto layerRadius = coilRadius + numLayers * wireDiameter * WireCompactFactor;
            wireLength += turnsPerLayer * PI * 2 * layerRadius * (numLayers - floor(numLayers));
            wireLength /= 1000.0;

            return wireLength;
        }

        double GetCoilWireResistance() const
        {
            const auto wireRadius = CoilWireDiameter / 2;
            return COPPER_WIRE_RESISTANCE * GetCoilWireLength() / (PI * (wireRadius / 1000.0) * (wireRadius / 1000.0));
        }
    };
    
    struct SimData
    {
    public:
        struct StepData
        {
            double Distance;
            
            double Inductance;
            std::vector<double> Forces;
        };
        
    public:
        /**
         * \brief Coil data.
         */
        struct
        {
            /**
             * \brief The calculated height of the current coil.
             */
            double Height;
            
            /**
             * \brief Resistance of the coil.
             */
            double Resistance;
            
            /**
             * \brief The total number of wire layers in the coil.
             *  This is a double, because we do not get a full layers sometimes, as
             *  there are only a few turns on the last layer.
             */
            double Layers;
            
            /**
             * \brief The total length of wire in the coil (meters).
             */
            double WireLength;
        } Coil{};
        
        /**
         * \brief Projectile data.
         */
        struct
        {
            /**
             * \brief The mass of the projectile (grams).
             */
            double Mass;
        } Projectile{};
        
    public:
        std::vector<int> Currents = {};
        std::vector<StepData> Steps = {};
        int NumSteps = 0;
    };

public:
    bool EnableLogging = true;
    
private:
    FemmAPI m_api;

private:
    void CgsConfigure(const SimParameters& parameters);
    void CgsCreateBoundary(const SimParameters& parameters);
    void CgsCreateCoil(SimData& data, const SimParameters& parameters);
    void CgsCreateProjectile(SimData& data, const SimParameters& parameters);
    
public:
    
    /**
     * \brief Simulates the coil using currently set values.
     * \param fileName The temporary FEMM file name.
     * \param parameters The parameters of the simulation. Includes coil and projectile configuration.
     * \return The simulated coil data. Make sure to pass it to Cleanup method, once finished processing the data.
     */
    SimData Simulate(const char* fileName, const SimParameters& parameters);
};
