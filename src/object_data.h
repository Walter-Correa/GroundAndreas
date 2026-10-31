#pragma once

#include <unordered_map>
#include <cmath>

struct ObjectDimensions
{
    float sizeX;
    float sizeY;
    float sizeZ;
};

inline bool GetCommonObjectModelDimensions(int modelId, float& sizeX, float& sizeY, float& sizeZ)
{
    static const std::unordered_map<int, ObjectDimensions> s_objectDimensions = {
        // Desert plant & industrial models from real SA-MP mappings
        { 3682,  { 25.0f, 25.0f, 30.0f } }, // Industrial tower / tank
        { 3683,  { 35.0f, 25.0f, 15.0f } }, // Desert warehouse / complex
        { 3288,  { 20.0f, 20.0f, 8.0f } },  // Plant structure
        { 3289,  { 20.0f, 20.0f, 8.0f } },  // Plant structure
        { 3290,  { 20.0f, 20.0f, 8.0f } },  // Plant structure
        { 3291,  { 20.0f, 20.0f, 8.0f } },  // Plant structure
        { 3424,  { 30.0f, 30.0f, 10.0f } }, // Plant building
        { 3259,  { 25.0f, 25.0f, 8.0f } },  // Plant warehouse
        { 3636,  { 30.0f, 25.0f, 15.0f } }, // Complex structure
        { 3256,  { 20.0f, 20.0f, 8.0f } },  // Base building
        { 3258,  { 20.0f, 20.0f, 8.0f } },  // Base building
        { 3675,  { 15.0f, 15.0f, 8.0f } },  // Complex pipe / structure
        { 3673,  { 20.0f, 20.0f, 30.0f } }, // Tower structure
        { 3257,  { 20.0f, 20.0f, 8.0f } },  // Base building
        { 3674,  { 20.0f, 20.0f, 35.0f } }, // Tall chimney / stack
        { 3255,  { 20.0f, 20.0f, 8.0f } },  // Base building
        { 16086, { 10.0f, 2.0f,  4.0f } },  // Perimeter fence / gate
        { 16087, { 10.0f, 2.0f,  4.0f } },  // Perimeter fence / gate
        { 16088, { 10.0f, 2.0f,  4.0f } },  // Perimeter fence / gate
        { 16089, { 10.0f, 2.0f,  4.0f } },  // Perimeter fence / gate
        { 16090, { 10.0f, 2.0f,  4.0f } },  // Perimeter fence / gate
        { 16091, { 10.0f, 2.0f,  4.0f } },  // Perimeter fence / gate
        { 16092, { 10.0f, 2.0f,  4.0f } },  // Perimeter fence / gate

        // SA-MP 0.3d & 0.3.7 Concrete Floor Slabs & Platforms
        { 19353, { 10.0f, 10.0f, 0.5f } },  // Concrete floor 10x10m
        { 19354, { 5.0f,  5.0f,  0.5f } },  // Concrete floor 5x5m
        { 19355, { 20.0f, 20.0f, 0.5f } },  // Concrete floor 20x20m
        { 19356, { 20.0f, 10.0f, 0.5f } },  // Concrete floor 20x10m
        { 19357, { 30.0f, 30.0f, 0.5f } },  // Concrete floor 30x30m
        { 19358, { 30.0f, 15.0f, 0.5f } },  // Concrete floor 30x15m
        { 19359, { 50.0f, 50.0f, 0.5f } },  // Concrete floor 50x50m
        { 19379, { 60.0f, 60.0f, 0.5f } },  // Mega tarmac / helipad slab (60x60m)
        { 19366, { 50.0f, 50.0f, 0.5f } },  // Large airport hangar floor
        { 19898, { 40.0f, 40.0f, 0.5f } },  // Parking lot tarmac slab

        // SA-MP 0.3.7 Modular Roads & Highways
        { 19475, { 10.0f, 20.0f, 0.4f } },  // Modular 2-lane road straight
        { 19476, { 20.0f, 20.0f, 0.4f } },  // Modular 4-lane road straight
        { 19477, { 20.0f, 20.0f, 0.4f } },  // Modular road intersection
        { 19478, { 10.0f, 20.0f, 0.4f } },  // Modular dirt road straight
        { 19479, { 20.0f, 20.0f, 0.4f } },  // Modular dirt road curve
        { 19480, { 20.0f, 20.0f, 0.4f } },  // Modular dirt road intersection
        { 19481, { 24.0f, 40.0f, 1.0f } },  // Highway elevated bridge straight
        { 19482, { 24.0f, 40.0f, 4.0f } },  // Highway elevated bridge incline ramp
        { 19483, { 30.0f, 30.0f, 1.0f } },  // Highway bridge curve

        // SA-MP 0.3.7 Stunt Tracks & Ramps
        { 16335, { 6.0f,  12.0f, 3.0f } },  // Stunt ramp medium
        { 16336, { 4.0f,  8.0f,  2.0f } },  // Stunt ramp small
        { 16337, { 10.0f, 20.0f, 5.0f } },  // Stunt ramp large
        { 16338, { 12.0f, 24.0f, 6.0f } },  // Stunt ramp steep
        { 1634,  { 3.0f,  5.0f,  1.2f } },  // Wood ramp small
        { 1635,  { 4.0f,  7.0f,  1.8f } },  // Wood ramp medium
        { 19945, { 15.0f, 30.0f, 1.0f } },  // 0.3.7 Stunt track straight
        { 19946, { 30.0f, 30.0f, 1.0f } },  // 0.3.7 Stunt track curve
        { 19947, { 15.0f, 30.0f, 8.0f } },  // 0.3.7 Stunt track incline
        { 19948, { 20.0f, 40.0f, 6.0f } },  // 0.3.7 Stunt half-pipe
        { 19949, { 25.0f, 50.0f, 15.0f } }, // 0.3.7 Stunt loop track

        // Airfield, docks & prison mapping objects
        { 3095,  { 15.0f, 15.0f, 0.4f } },  // Helipad pad circle
        { 18450, { 30.0f, 60.0f, 0.5f } },  // Runway slab
        { 18451, { 30.0f, 40.0f, 0.5f } },  // Runway end section
        { 19377, { 12.0f, 30.0f, 0.8f } },  // Pier / dock large
        { 19378, { 6.0f,  15.0f, 0.8f } },  // Pier / dock small
        { 18833, { 30.0f, 30.0f, 0.5f } },  // Prison courtyard ground
        { 18834, { 20.0f, 20.0f, 0.5f } },  // Prison floor slab
        { 18835, { 15.0f, 3.0f,  1.0f } },  // Prison wall foundation

        // GTA SA Original bridges & highway segments
        { 8393,  { 25.0f, 50.0f, 1.5f } },  // SF bridge girder
        { 8394,  { 20.0f, 40.0f, 0.8f } }   // SF bridge road segment
    };

    auto it = s_objectDimensions.find(modelId);
    if (it != s_objectDimensions.end())
    {
        sizeX = it->second.sizeX;
        sizeY = it->second.sizeY;
        sizeZ = it->second.sizeZ;
        return true;
    }

    // Smart heuristic for uncatalogued SA-MP 0.3.7 modular objects
    if (modelId >= 19353 && modelId <= 19483)
    {
        sizeX = 20.0f;
        sizeY = 20.0f;
        sizeZ = 0.5f;
        return true;
    }
    if (modelId >= 19945 && modelId <= 19999)
    {
        sizeX = 15.0f;
        sizeY = 30.0f;
        sizeZ = 1.0f;
        return true;
    }

    // Default fallback dimensions
    sizeX = 12.0f;
    sizeY = 12.0f;
    sizeZ = 1.0f;
    return false;
}

inline float GetModelPhysicalRadius(int modelId)
{
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    GetCommonObjectModelDimensions(modelId, sx, sy, sz);
    float hx = sx * 0.5f;
    float hy = sy * 0.5f;
    return std::sqrt(hx * hx + hy * hy);
}
