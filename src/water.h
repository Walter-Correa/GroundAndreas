#pragma once

#include <cstdint>

struct WaterVolume
{
    const char* name;
    float minX;
    float maxX;
    float minY;
    float maxY;
    float surfaceZ;
    float maxDepth;
    int flag;
};

class WaterSystem
{
public:
    static float GetWaveAmplitude(int weatherid);

    static bool GetWaterLevelAt(float x, float y, float& surfaceZ, float& bottomZ, int weatherid = -1);

    static bool IsPointInWater(float x, float y, float z, int weatherid = -1);
    static bool IsPointUnderWater(float x, float y, float z, int weatherid = -1);

    static bool IsPlayerInWater(float px, float py, float pz, int weatherid = -1);
    static bool IsPlayerUnderWater(float px, float py, float pz, int weatherid = -1);

    static bool IsVehicleInWater(float vx, float vy, float vz, float sizeZ, int weatherid = -1);
    static bool IsVehicleUnderWater(float vx, float vy, float vz, float sizeZ, int weatherid = -1);
};
