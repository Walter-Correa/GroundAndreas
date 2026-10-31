#define FILTERSCRIPT

#include <a_samp>
#include "GroundAndreas.inc"

/*
 *  Convenience Helper Stocks
 */

stock Float:GetGroundHeight(Float:x, Float:y, bool:interpolate = true, bool:water_surface = true)
{
    new Float:z = 0.0;
    GetGroundZPos(x, y, z, interpolate, water_surface);
    return z;
}

stock bool:HasLineOfSight(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2)
{
    new Float:hx, Float:hy, Float:hz;
    return !IsWorldBlockingLine(x1, y1, z1, x2, y2, z2, hx, hy, hz);
}

stock bool:IsPositionUnderground(Float:x, Float:y, Float:z)
{
    return IsPointUnderGround(x, y, z);
}

public OnFilterScriptInit()
{
    print("\n========================================================");
    print("  GroundAndreas - Test & Demonstration Suite");
    print("========================================================\n");

    // Check if heightmap is loaded
    if (!GroundAndreas_IsLoaded())
    {
        print("[GroundAndreas Test] Attempting manual init...");
        if (!GroundAndreas_Init("scriptfiles/SAfull.hmap"))
        {
            print("[GroundAndreas Test] ERROR: Failed to load scriptfiles/SAfull.hmap!");
            return 1;
        }
    }
    print("[GroundAndreas Test] Heightmap loaded and ready!\n");

    // 1. Elevation Query Test at iconic landmarks
    new Float:z_discrete, Float:z_interpolated, Float:z_water, Float:z_default;
    
    // Mount Chiliad summit (~ -2300.0, -1630.0)
    GetGroundZPos(-2300.0, -1630.0, z_discrete, false, false);
    GetGroundZPos(-2300.0, -1630.0, z_interpolated, true, false);
    printf("[1. Elevation] Mount Chiliad: Discrete=%.2fm | Interpolated=%.2fm", z_discrete, z_interpolated);

    // Grove Street cul-de-sac (2495.0, -1670.0)
    GetGroundZPos(2495.0, -1670.0, z_discrete, false, false);
    GetGroundZPos(2495.0, -1670.0, z_interpolated, true, false);
    printf("[1. Elevation] Grove Street: Discrete=%.2fm | Interpolated=%.2fm", z_discrete, z_interpolated);

    // Los Santos Beach Ocean Pier (360.0, -2090.0)
    GetGroundZPos(360.0, -2090.0, z_discrete, false, false); // water_surface = false (seabed)
    GetGroundZPos(360.0, -2090.0, z_water, false, true);    // water_surface = true (sea level)
    GetGroundZPos(360.0, -2090.0, z_default);               // default water_surface is true
    printf("[1. Elevation] Santa Maria Sea: Seabed=%.2fm | WaterSurface=%.2fm | Default=%.2fm\n", z_discrete, z_water, z_default);

    // 2. Surface Normal & Slope Calculation
    new Float:nx, Float:ny, Float:nz, Float:slope;
    GetGroundNormal(-2300.0, -1630.0, nx, ny, nz, slope);
    printf("[2. Normal & Slope] Mount Chiliad Slope: %.2f degrees | Normal=(%.3f, %.3f, %.3f)", slope, nx, ny, nz);

    GetGroundNormal(2495.0, -1670.0, nx, ny, nz, slope);
    printf("[2. Normal & Slope] Grove Street Slope: %.2f degrees | Normal=(%.3f, %.3f, %.3f)\n", slope, nx, ny, nz);

    // 3. Dynamic Object Rotation on Slopes (Pitch & Roll)
    new Float:rx, Float:ry;
    GetGroundObjectRotation(-2300.0, -1630.0, 0.0, rx, ry); // Facing North
    printf("[3. Object Rotation] Object on slope facing North: Pitch(RX)=%.2f deg | Roll(RY)=%.2f deg\n", rx, ry);

    // 4. Line of Sight / World Blocking Raycast
    new Float:hx, Float:hy, Float:hz;
    new bool:blocked = IsWorldBlockingLine(1500.0, -1600.0, 500.0, 2000.0, -1600.0, 500.0, hx, hy, hz);
    new bool:los = HasLineOfSight(1500.0, -1600.0, 500.0, 2000.0, -1600.0, 500.0);
    printf("[4. Raycast] Sky Line (Z=500): Blocked=%s | HasLineOfSight=%s", blocked ? ("YES") : ("NO"), los ? ("YES") : ("NO"));

    blocked = IsWorldBlockingLine(-2350.0, -1630.0, 300.0, -2250.0, -1630.0, 300.0, hx, hy, hz);
    los = HasLineOfSight(-2350.0, -1630.0, 300.0, -2250.0, -1630.0, 300.0);
    printf("[4. Raycast] Chiliad Tunnel Trajectory: Blocked=%s (Hit at: %.1f, %.1f, %.1f) | HasLineOfSight=%s\n", 
        blocked ? ("YES") : ("NO"), hx, hy, hz, los ? ("YES") : ("NO"));

    // 5. Underground Detection System
    new bool:is_point_ug = IsPointUnderGround(2495.0, -1670.0, 10.0); // Ground is ~24m, Z=10 is underground
    new Float:point_dist = GetPointGroundDistance(2495.0, -1670.0, 10.0);
    printf("[5. Underground] Grove Street at Z=10.0m: UnderGround=%s | Distance=%.2fm", is_point_ug ? ("YES") : ("NO"), point_dist);

    is_point_ug = IsPointUnderGround(2495.0, -1670.0, 30.0); // Z=30 is above ground
    point_dist = GetPointGroundDistance(2495.0, -1670.0, 30.0);
    printf("[5. Underground] Grove Street at Z=30.0m: UnderGround=%s | Distance=%.2fm\n", is_point_ug ? ("YES") : ("NO"), point_dist);

    // 6. Water Subsystem (Ocean, Inland Lakes, Pools, Weather Dynamics)
    new Float:water_level;
    // Sherman Reservoir (Desert Dam)
    new bool:has_water = GetWaterLevel(-1000.0, 2400.0, water_level);
    new bool:in_water = IsPointInWater(-1000.0, 2400.0, 25.0);
    new bool:under_water = IsPointUnderWater(-1000.0, 2400.0, 25.0);
    printf("[6. Water] Sherman Dam: HasWater=%s (Level=%.1fm) | Z=25 InWater=%s, UnderWater=%s", 
        has_water ? ("YES") : ("NO"), water_level, in_water ? ("YES") : ("NO"), under_water ? ("YES") : ("NO"));

    // Glen Park Lake
    has_water = GetWaterLevel(1960.0, -1200.0, water_level);
    in_water = IsPointInWater(1960.0, -1200.0, 17.5);
    under_water = IsPointUnderWater(1960.0, -1200.0, 17.5);
    printf("[6. Water] Glen Park: HasWater=%s (Level=%.1fm) | Z=17.5 InWater=%s, UnderWater=%s", 
        has_water ? ("YES") : ("NO"), water_level, in_water ? ("YES") : ("NO"), under_water ? ("YES") : ("NO"));

    // Ocean with weather surge (Weather 19: Sandstorm / Storm tide)
    has_water = GetWaterLevel(360.0, -2090.0, water_level, 19);
    printf("[6. Water] Ocean (Weather 19): HasWater=%s | Storm Tide Level=%.1fm\n", 
        has_water ? ("YES") : ("NO"), water_level);

    // 7. Batch Processing Performance Benchmark (10,000 queries)
    new const TOTAL_BENCHMARK = 10000;
    new Float:batch_x[TOTAL_BENCHMARK];
    new Float:batch_y[TOTAL_BENCHMARK];
    new Float:batch_z[TOTAL_BENCHMARK];

    for (new i = 0; i < TOTAL_BENCHMARK; ++i)
    {
        batch_x[i] = -2000.0 + (float(i % 100) * 40.0);
        batch_y[i] = -2000.0 + (float(i / 100) * 40.0);
    }

    new start_tick = GetTickCount();
    new processed = GetGroundZPosBatch(batch_x, batch_y, batch_z, TOTAL_BENCHMARK, true);
    new elapsed = GetTickCount() - start_tick;

    printf("[7. Benchmark] Processed %d elevation queries in %d ms (~%.2f ns/query)\n", 
        processed, elapsed, (float(elapsed) * 1000000.0) / float(TOTAL_BENCHMARK));

    print("========================================================");
    print("  GroundAndreas tests completed successfully!");
    print("========================================================\n");
    return 1;
}

public OnFilterScriptExit()
{
    print("[GroundAndreas Test] Unloading filterscript.");
    return 1;
}
