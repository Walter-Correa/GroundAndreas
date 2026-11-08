#include "water.h"
#include "heightmap.h"
#include <cmath>
#include <algorithm>

// Official GTA San Andreas Inland Water Bodies Catalog (extracted directly from data/water.dat)
// Contains all 44 inland water quads with authentic elevations and bounded maximum depths (T6).
static const WaterVolume INLAND_WATER_VOLUMES[] =
{
    { "Sherman Reservoir (Dam North)",               -1248.00f, -1088.00f,  2536.00f,  2824.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Dam North-East)",          -1088.00f, -1040.00f,  2544.00f,  2800.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (North-East Bay)",          -1040.00f,  -832.00f,  2544.00f,  2760.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (East Central)",            -1088.00f,  -832.00f,  2416.00f,  2544.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Central Basin)",           -1040.00f,  -864.00f,  2304.00f,  2416.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (South-East)",              -1024.00f,  -864.00f,  2144.00f,  2304.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (South Basin)",             -1072.00f, -1024.00f,  2152.00f,  2264.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (South-West Inlet)",        -1200.00f, -1072.00f,  2114.00f,  2242.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Southern Shore)",           -976.00f,  -848.00f,  2016.00f,  2144.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Gorge East)",               -864.00f,  -448.00f,  2144.00f,  2272.00f,   40.5999f, 60.0f, 1 },
    { "Sherman Reservoir (Gorge Far East)",           -700.00f,  -484.00f,  2272.00f,  2320.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (River Intake)",             -608.00f,  -528.00f,  2320.00f,  2352.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (South Channel)",            -848.00f,  -816.00f,  2044.00f,  2144.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Eastern Outlet)",           -816.00f,  -496.00f,  2060.00f,  2144.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Canal)",                    -604.00f,  -484.00f,  2036.00f,  2060.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (West Inlet)",              -1328.00f, -1200.00f,  2082.00f,  2210.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Western Reach)",           -1400.00f, -1328.00f,  2074.00f,  2150.00f,   40.6000f, 60.0f, 1 },
    { "Sherman Reservoir (Eastern Inlet)",            -550.00f,  -494.00f,  2004.00f,  2036.00f,   40.6000f, 60.0f, 1 },
    { "Glen Park Lake (Los Santos)",                  1928.00f,  2012.00f, -1222.00f, -1178.00f,   18.0000f, 10.0f, 3 },
    { "Jefferson Church Fountain (Los Santos)",       2248.00f,  2260.00f, -1182.00f, -1170.00f,   23.3374f,  3.0f, 3 },
    { "Jefferson Church Pond (Los Santos)",           2292.00f,  2328.00f, -1432.00f, -1400.00f,   22.1650f,  5.0f, 3 },
    { "Pirate Ship Bay East (Las Venturas)",          1888.00f,  2036.00f,  1468.00f,  1700.00f,    8.5984f,  8.0f, 3 },
    { "Pirate Ship Bay West (Las Venturas)",          1836.00f,  1888.00f,  1468.00f,  1568.00f,    8.5984f,  8.0f, 3 },
    { "The Mirage Pool (Las Venturas Strip)",         2090.00f,  2146.00f,  1670.00f,  1694.00f,    9.6117f,  4.0f, 3 },
    { "The Camel's Toe Fountain (Las Venturas)",      2110.00f,  2178.00f,  1234.00f,  1330.00f,    7.8327f,  3.0f, 3 },
    { "Come-A-Lot Castle Moat (Las Venturas)",        2108.00f,  2180.00f,  1084.00f,  1172.00f,    7.5628f,  5.0f, 3 },
    { "Pilgrim Motel Pool (Las Venturas)",            2506.00f,  2554.00f,  1546.00f,  1586.00f,    8.9671f,  4.0f, 3 },
    { "Madd Dogg Mansion Outdoor Pool (Mulholland)",  1270.00f,  1290.00f,  -812.00f,  -800.00f,   86.6730f,  5.0f, 3 },
    { "Mulholland Mansion Pool (Los Santos)",         1084.00f,  1104.00f,  -684.00f,  -660.00f,  112.0000f,  5.0f, 3 },
    { "Rodeo Mansion Pool (Los Santos)",               502.00f,   522.00f, -1114.00f, -1098.00f,   78.4231f,  5.0f, 3 },
    { "Richman Mansion Lower Pool Part 1 (Los Santos)", 214.00f,   246.00f, -1208.00f, -1180.00f,   74.0000f,  5.0f, 3 },
    { "Richman Mansion Lower Pool Part 2 (Los Santos)", 218.00f,   238.00f, -1180.00f, -1172.00f,   74.0000f,  5.0f, 3 },
    { "Richman Mansion Upper Pool (Los Santos)",       178.00f,   206.00f, -1244.00f, -1216.00f,   77.0534f,  5.0f, 3 },
    { "Pilson Intersection Pool (Las Venturas)",      1744.00f,  1792.00f,  2780.00f,  2868.00f,    8.4730f,  5.0f, 3 },
    { "SF South Lake (Mount Chiliad / Whetstone)",   -2778.00f, -2662.00f,  -522.00f,  -414.00f,    2.7926f, 15.0f, 1 },
    { "The Visage Cascade (Las Venturas)",            2058.00f,  2110.00f,  1868.00f,  1964.00f,    9.6292f,  6.0f, 3 },
    { "Vinewood Mansion Pool (Los Santos)",            890.00f,   902.00f, -1106.00f, -1098.00f,   22.4100f,  4.0f, 3 },
    { "LS Airport Lake (Los Santos)",                 1202.00f,  1278.00f, -2414.00f, -2334.00f,    8.8644f, 15.0f, 1 },
    { "Fallen Tree Lake West (Flint County)",         -848.00f,  -664.00f, -2082.00f, -1866.00f,    5.2700f, 20.0f, 0 },
    { "Fallen Tree Lake East (Flint County)",         -664.00f,  -464.00f, -1924.00f, -1864.00f,    5.2700f, 20.0f, 0 },
    { "Madd Dogg Mansion Indoor Pool (Gym)",          1270.00f,  1290.00f,  -780.00f,  -768.00f, 1082.7300f,  5.0f, 3 },
    { "Avispa Country Club Golf Ponds (San Fierro)", -2048.00f, -2004.00f,  -962.00f,  -758.00f,   30.4000f,  6.0f, 3 },
    { "Rock Hotel Pool (Las Venturas)",               2564.00f,  2604.00f,  2370.00f,  2398.00f,   16.4000f,  5.0f, 3 },
    { "Missionary Hill Pond (Foster Valley SF)",     -2522.00f, -2382.00f,  -310.00f,  -234.00f,   35.3820f, 15.0f, 1 }
};

float WaterSystem::GetWaveAmplitude(int weatherid)
{
    if (weatherid == 4 || weatherid == 7 || weatherid == 8 ||
        weatherid == 12 || (weatherid >= 14 && weatherid <= 16))
    {
        return 0.5f;
    }
    if (weatherid == 19)
    {
        return 0.8f; // Weather 19 (sandstorm)
    }
    return 0.15f;
}

bool WaterSystem::GetWaterLevelAt(float x, float y, float& surfaceZ, float& bottomZ, int weatherid)
{
    // Sanitize non-finite inputs (T7)
    if (!std::isfinite(x) || !std::isfinite(y))
    {
        return false;
    }

    // 1. Check official inland water quads from GTA San Andreas (data/water.dat)
    for (const auto& vol : INLAND_WATER_VOLUMES)
    {
        if (x >= vol.minX && x <= vol.maxX && y >= vol.minY && y <= vol.maxY)
        {
            float groundZ = 0.0f;
            if (HeightMap::Instance().GetZ(x, y, groundZ, true, false))
            {
                if (groundZ >= vol.surfaceZ)
                {
                    continue;
                }

                // If water surface is far above ground (e.g. interior pool at Z=1082 over outdoor terrain at Z=90):
                // treat as a floating interior volume with bounded depth so outdoor ground is not counted as submerged (T6).
                if (vol.surfaceZ - groundZ > vol.maxDepth + 10.0f)
                {
                    bottomZ = vol.surfaceZ - vol.maxDepth;
                }
                else
                {
                    bottomZ = (std::max)(groundZ, vol.surfaceZ - vol.maxDepth);
                }
            }
            else
            {
                bottomZ = vol.surfaceZ - vol.maxDepth;
            }

            surfaceZ = vol.surfaceZ;
            return true;
        }
    }

    // 2. Check heightmap ocean, canals, and coastal shores
    // Note (C7): Low terrain cells <= waveAmp (e.g. ultra-low docks or beaches) evaluate
    // to water during high swell weather.
    float rawGroundZ = 0.0f;
    if (HeightMap::Instance().GetZ(x, y, rawGroundZ, true, false))
    {
        const float waveAmp = GetWaveAmplitude(weatherid);
        if (rawGroundZ <= waveAmp)
        {
            surfaceZ = waveAmp;
            bottomZ = (rawGroundZ <= 0.05f) ? -100.0f : rawGroundZ;
            return true;
        }
    }

    // 3. Boundless ocean surrounding San Andreas beyond map borders
    if (x < -3000.0f || x > 3000.0f || y < -3000.0f || y > 3000.0f)
    {
        surfaceZ = GetWaveAmplitude(weatherid);
        bottomZ = -100.0f;
        return true;
    }

    return false;
}

bool WaterSystem::IsPointInWater(float x, float y, float z, int weatherid)
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
    {
        return false;
    }

    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(x, y, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }

    return (z <= surfaceZ && z >= bottomZ);
}

bool WaterSystem::IsPointUnderWater(float x, float y, float z, int weatherid)
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
    {
        return false;
    }

    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(x, y, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }

    return (z < (surfaceZ - 0.20f) && z >= bottomZ);
}

bool WaterSystem::IsPlayerInWater(float px, float py, float pz, int weatherid)
{
    if (!std::isfinite(px) || !std::isfinite(py) || !std::isfinite(pz))
    {
        return false;
    }

    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(px, py, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }

    // Player torso immersion: contact is detected if feet are below surface and head is above bottom
    return (pz <= (surfaceZ + 0.35f) && pz >= (bottomZ - 0.5f));
}

bool WaterSystem::IsPlayerUnderWater(float px, float py, float pz, int weatherid)
{
    if (!std::isfinite(px) || !std::isfinite(py) || !std::isfinite(pz))
    {
        return false;
    }

    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(px, py, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }

    // Head fully submerged: top of head ~ ped root + 0.9m
    return ((pz + 0.90f) < surfaceZ && pz >= (bottomZ - 0.5f));
}

bool WaterSystem::IsVehicleInWater(float vx, float vy, float vz, float sizeZ, int weatherid)
{
    if (!std::isfinite(vx) || !std::isfinite(vy) || !std::isfinite(vz) || !std::isfinite(sizeZ))
    {
        return false;
    }

    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(vx, vy, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }

    const float halfHeight = (sizeZ > 0.0f) ? (sizeZ * 0.5f) : 0.8f;
    const float vehicleBottom = vz - halfHeight;
    const float vehicleTop = vz + halfHeight;

    return (vehicleBottom <= surfaceZ && vehicleTop >= bottomZ);
}

bool WaterSystem::IsVehicleUnderWater(float vx, float vy, float vz, float sizeZ, int weatherid)
{
    if (!std::isfinite(vx) || !std::isfinite(vy) || !std::isfinite(vz) || !std::isfinite(sizeZ))
    {
        return false;
    }

    float surfaceZ = 0.0f;
    float bottomZ = 0.0f;
    if (!GetWaterLevelAt(vx, vy, surfaceZ, bottomZ, weatherid))
    {
        return false;
    }

    const float halfHeight = (sizeZ > 0.0f) ? (sizeZ * 0.5f) : 0.8f;
    const float vehicleTop = vz + halfHeight;
    const float vehicleBottom = vz - halfHeight;

    return (vehicleTop < surfaceZ && vehicleBottom >= bottomZ);
}
