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
    new Float:clearance, bool:underground;
    if (GetGroundClearance(x, y, z, clearance, underground))
    {
        return underground;
    }
    return false;
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

    // Stock helper test
    new Float:helper_z = GetGroundHeight(2495.0, -1670.0);
    printf("[1. Elevation Helper] GetGroundHeight: %.2fm\n", helper_z);

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
    // Test A: High altitude line (clear)
    new Float:hx, Float:hy, Float:hz;
    new bool:blocked = IsWorldBlockingLine(1500.0, -1600.0, 500.0, 2000.0, -1600.0, 500.0, hx, hy, hz);
    new bool:los = HasLineOfSight(1500.0, -1600.0, 500.0, 2000.0, -1600.0, 500.0);
    printf("[4. Raycast] Sky Line (Z=500): Blocked=%s | HasLineOfSight=%s", blocked ? ("YES") : ("NO"), los ? ("YES") : ("NO"));

    // Test B: Line piercing through a hill
    blocked = IsWorldBlockingLine(-2350.0, -1630.0, 300.0, -2250.0, -1630.0, 300.0, hx, hy, hz);
    los = HasLineOfSight(-2350.0, -1630.0, 300.0, -2250.0, -1630.0, 300.0);
    printf("[4. Raycast] Chiliad Tunnel Trajectory: Blocked=%s (Hit at: %.1f, %.1f, %.1f) | HasLineOfSight=%s\n", 
        blocked ? ("YES") : ("NO"), hx, hy, hz, los ? ("YES") : ("NO"));

    // 5. Water Depth & Underwater Check
    new Float:water_depth;
    new bool:underwater = IsGroundUnderwater(360.0, -2090.0, water_depth);
    printf("[5. Water] Santa Maria Ocean: Underwater=%s | Column Depth=%.2fm\n", underwater ? ("YES") : ("NO"), water_depth);

    // 6. Clearance and Underground Check
    new Float:clearance, bool:is_ug;
    GetGroundClearance(2495.0, -1670.0, 25.0, clearance, is_ug);
    new bool:stock_ug = IsPositionUnderground(2495.0, -1670.0, 25.0);
    printf("[6. Clearance] Grove Street at Z=25.0: Clearance=%.2fm | Underground=%s | StockCheck=%s\n", 
        clearance, is_ug ? ("YES") : ("NO"), stock_ug ? ("YES") : ("NO"));

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
