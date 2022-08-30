#pragma once

#include "FemmExtensions.h"

#pragma region FEMM DATA
#define GROUP_COMMON 0
#define GROUP_COIL 1
#define GROUP_PROJECTILE 2

#define MATERIAL_WIRE "Wire"
#define MATERIAL_PROJECTILE "Projectile"
#pragma endregion

class CoilGunSim
{
public:
    enum class CoilShape
    {
        Cylinder = 0,
        Ball,
        Angled45
    };
    
public:
    struct SimParameters
    {
        int BoundaryLayers = 3;
        double BoundaryHeight = 150.0;

        double WireCompactFactor = 1.1;
        
        double CoilLength = 35.0;
        double CoilInnerDiameter = 7.0;
        int CoilWireTurns = 130;
        double CoilWireWidth = 0.9;
        double CoilShellWidth = 0.0;
    
        double ProjectileDiameter = 4.5;
        double ProjectileLength = 45;
        
        /**
         * \brief The diameter of the hole.
         */
        double ProjectileHoleDiameter = 0.0;
            
        /**
         * \brief The length of the hole, starting from the back of the coil.
         */
        double ProjectileHoleLength = 0.0;
        
        /**
         * \brief The material of the projectile. Use FEMM to lookup different materials.
         *  The suggested materials are: 'M-50' (steel), '416 Stainless Steel' and 'Mu Metal' (the best one).
         */
        const char* ProjectileMaterialType = "M-50";
        
        /**
         * \brief The material density of the projectile (grams per centimeter cubed)
         */
        double ProjectileMaterialDensity = 7.85;
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
    void CalculateCoilHeightAndLayers(SimData& data, const SimParameters& parameters) const;
    void CalculateCoilWireLength(SimData& data, const SimParameters& parameters) const;
    void CalculateCoilResistance(SimData& data, const SimParameters& parameters) const;
    
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
