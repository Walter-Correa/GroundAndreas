#include "water.h"
#include "heightmap.h"

static const WaterVolume INLAND_WATER_VOLUMES[] =
{
    { "Sherman Reservoir",           -1408.0629f, -460.9319f,  2021.0587f,  2861.4893f,    0.0f,   40.5f },
    { "Madd Dog Mansion Outdoor Pool", 1269.5886f, 1288.8916f,  -810.4507f,  -800.5959f,   82.0f,   86.5f },
    { "Madd Dog Mansion Indoor Pool",  1271.4152f, 1289.0560f,  -779.6528f,  -769.5222f, 1078.0f, 1082.5f },
    { "Mulholland Mansion Pool",      1086.6010f, 1103.0225f,  -683.6998f,  -662.1832f,  107.0f,  112.0f },
    { "Richman Pools",                 169.6879f,  281.5444f, -1392.6630f, -1168.9749f,   70.0f,   77.0f },
    { "Fisher Lagoon",                -828.3018f, -443.5032f, -2107.1487f, -1855.6273f,   -5.0f,    5.5f },
    { "LS Airport Lake",              1198.9982f, 1275.7397f, -2407.0581f, -2341.6262f,    4.0f,    8.7f },
    { "Jefferson Church Pond",        2293.1467f, 2331.7388f, -1432.5869f, -1397.6476f,   17.0f,   22.0f },
    { "Glen Park Lake",               1915.3196f, 2016.6565f, -1236.2382f, -1167.9290f,   13.0f,   18.0f },
    { "San Fierro South Lake",       -2747.1692f, -2663.2468f, -510.1152f,  -411.4905f,   -2.0f,    2.6f },
    { "Pirate Ship Bay LV",           1838.8041f, 2035.4167f,  1463.2024f,  1703.0480f,    5.0f,    8.5f },
    { "The Visage Cascade LV",        2049.6646f, 2119.9021f,  1843.3029f,  1983.5464f,    5.0f,    9.5f },
    { "The Camel's Toe Fountain LV",  2082.3198f, 2194.3608f,  1203.2947f,  1357.6477f,    4.5f,    7.7f },
    { "Come-A-Lot Castle Moat LV",    2094.9255f, 2179.1790f,  1085.2852f,  1183.0610f,    4.5f,    7.5f },
    { "Rock Hotel Pool LV",           2545.3718f, 2593.6794f,  2347.7112f,  2403.8040f,   12.0f,   16.5f },
    { "Pilgrim Motel Pool LV",        2510.7275f, 2552.8618f,  1550.1548f,  1586.1469f,    6.0f,    8.8f }
};

float WaterSystem::GetWaveAmplitude(int weatherid)
{
    if (weatherid < 0)
    {
        return 0.0f;
    }
    if (weatherid == 4 || weatherid == 7 || weatherid == 8 ||
        weatherid == 12 || (weatherid >= 14 && weatherid <= 16))
    {
        return 1.0f;
    }
    if (weatherid == 19)
    {
        return 1.5f;
    }
    return 0.5f;
}

bool WaterSystem::GetWaterLevelAt(float x, float y, float& surfaceZ, float& bottomZ, int weatherid)
{
    // 1. Check known inland water volumes (reservoirs, lakes, fountains, swimming pools)
    for (const auto& vol : INLAND_WATER_VOLUMES)
    {
        if (x >= vol.minX && x <= vol.maxX && y >= vol.minY && y <= vol.maxY)
        {
            surfaceZ = vol.surfaceZ;
            bottomZ = vol.bottomZ;
            return true;
        }
    }

    // 2. Check heightmap for ocean, bay, or river bathymetry
    float rawGroundZ = 0.0f;
    if (HeightMap::Instance().GetZ(x, y, rawGroundZ, true, false))
    {
        if (rawGroundZ < 0.0f)
        {
            surfaceZ = GetWaveAmplitude(weatherid);
            bottomZ = rawGroundZ;
            return true;
        }
    }

    return false;
}

bool WaterSystem::IsPointInWater(float x, float y, float z, int weatherid)
{
    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(x, y, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }
    return (z <= surfaceZ && z >= (bottomZ - 1.0f));
}

bool WaterSystem::IsPointUnderWater(float x, float y, float z, int weatherid)
{
    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(x, y, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }
    return (z < surfaceZ && z >= (bottomZ - 1.0f));
}

bool WaterSystem::IsPlayerInWater(float px, float py, float pz, int weatherid)
{
    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(px, py, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }
    const float feetZ = pz - 0.90f;
    const float headZ = pz + 0.90f;
    return (feetZ <= surfaceZ && headZ >= (bottomZ - 1.0f));
}

bool WaterSystem::IsPlayerUnderWater(float px, float py, float pz, int weatherid)
{
    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(px, py, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }
    const float headZ = pz + 0.90f;
    return (headZ < surfaceZ && headZ >= (bottomZ - 1.0f));
}

bool WaterSystem::IsVehicleInWater(float vx, float vy, float vz, float sizeZ, int weatherid)
{
    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(vx, vy, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }
    const float wheelsZ = vz - (sizeZ * 0.5f);
    const float roofZ = vz + (sizeZ * 0.5f);
    return (wheelsZ <= surfaceZ && roofZ >= (bottomZ - 1.0f));
}

bool WaterSystem::IsVehicleUnderWater(float vx, float vy, float vz, float sizeZ, int weatherid)
{
    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(vx, vy, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }
    const float roofZ = vz + (sizeZ * 0.5f);
    return (roofZ < surfaceZ && roofZ >= (bottomZ - 1.0f));
}
