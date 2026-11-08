/*
 * ============================================================================
 * GroundAndreas 2.5D Heightmap Architecture & Technical Notes
 * ============================================================================
 * - Representation:
 *   The terrain is modeled as a 6000x6000 grid of 16-bit unsigned elevation values
 *   (0.01m resolution, range 0.0m to 655.35m).
 * - 2.5D Limitation:
 *   Each (X, Y) grid cell records only the single highest solid collision surface.
 *   Subterranean locations, multi-level structures, tunnels (e.g. LS/SF train tunnels),
 *   under-bridge spaces, and indoor interiors will return the roof/deck elevation as ground Z.
 * - Interior Awareness:
 *   Player and vehicle state natives accept `bool:ignore_interiors = true` by default.
 *   When the target entity is located in an interior (> 0), these natives gracefully
 *   bypass heightmap checks to prevent false underground flags. Note: Virtual worlds (VW)
 *   used outdoors in roleplay servers do not trigger ignore_interiors.
 * - Negative Elevations:
 *   Because values are unsigned uint16, underwater ocean floor elevations (< 0.0m)
 *   are not representable. Water queries utilize the dedicated 44-volume 3D water catalog
 *   with weather-based dynamic wave amplitudes (0.15m calm, 0.50m rain/storm/fog/overcast (4, 7, 8, 12, 14-16), 0.80m weather 19 (sandstorm)).
 * ============================================================================
 */

#if defined _GroundAndreas_included
    #endinput
#endif
#define _GroundAndreas_included

#if !defined _samp_included && !defined _INC_open_mp
    #error "Include a_samp or open.mp before GroundAndreas.inc"
#endif

#define GROUNDANDREAS_VERSION_MAJOR           1
#define GROUNDANDREAS_VERSION_MINOR           1
#define GROUNDANDREAS_VERSION_PATCH           0
#define GROUNDANDREAS_VERSION_STRING          "1.1.0"

#if defined GROUNDANDREAS_PREFIXED_NATIVES
// =========================================================================
// Anti-Collision Prefixed Mode (GA_...)
// Declares ONLY prefixed native symbols to prevent third-party collisions.
// =========================================================================
native GA_Init(const file_path[] = "scriptfiles/SAfull.hmap");
native GA_Unload();
native bool:GA_IsLoaded();

native GA_GetGroundZPos(Float:x, Float:y, &Float:z, bool:interpolate = true, bool:water_surface = true);
native GA_GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true, bool:water_surface = true);
native bool:GA_GetGroundZPosArea(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:avg_z, &Float:max_z = 0.0, &Float:min_z = 0.0, bool:water_surface = true);
native bool:GA_GetGroundZPosRadius(Float:x, Float:y, Float:radius, &Float:avg_z, &Float:max_z = 0.0, &Float:min_z = 0.0, bool:water_surface = true);

#define GA_GetGroundZ(%0,%1,%2)             GA_GetGroundZPos(%0,%1,%2)
#define GA_GetGroundZBatch(%0,%1,%2,%3)     GA_GetGroundZPosBatch(%0,%1,%2,%3)

native bool:GA_IsWorldBlockingLine(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2, &Float:hit_x = 0.0, &Float:hit_y = 0.0, &Float:hit_z = 0.0);
native GA_GetGroundNormal(Float:x, Float:y, &Float:nx, &Float:ny, &Float:nz, &Float:slope_degrees);
native GA_GetGroundObjectRotation(Float:x, Float:y, Float:facing_angle, &Float:rx, &Float:ry);
native GA_GetGroundObjectPose(Float:x, Float:y, Float:facing_angle, Float:z_offset, &Float:out_z, &Float:out_rx, &Float:out_ry, bool:interpolate = true);
native GA_GetGroundTwoPointRotation(Float:x1, Float:y1, Float:x2, Float:y2, Float:z_offset, &Float:center_z, &Float:rx, &Float:ry, &Float:rz, bool:interpolate = true);
native GA_GetGroundLinePoints(Float:start_x, Float:start_y, Float:end_x, Float:end_y, Float:spacing, Float:z_offset, Float:out_x[], Float:out_y[], Float:out_z[], Float:out_rx[], Float:out_ry[], Float:out_rz[], max_points = sizeof(out_x));
native bool:GA_IsGroundUnderwater(Float:x, Float:y, &Float:water_depth = 0.0);
native GA_GetGroundClearance(Float:x, Float:y, Float:z, &Float:clearance, &bool:is_underground = false);

native bool:GA_IsPointUnderGround(Float:x, Float:y, Float:z, Float:tolerance = 0.2);
native bool:GA_IsPlayerUnderGround(playerid, bool:ignore_interiors = true);
native bool:GA_IsVehicleUnderGround(vehicleid, bool:ignore_interiors = true);
native bool:GA_IsPlayerOnGround(playerid, Float:max_clearance = 0.35, bool:check_slope = true, bool:allow_water = false, bool:ignore_interiors = true);
native bool:GA_IsVehicleOnGround(vehicleid, Float:max_clearance = 0.35, bool:check_upsidedown = true, bool:allow_water = false, bool:ignore_interiors = true);
native Float:GA_GetPointGroundDistance(Float:x, Float:y, Float:z);
native Float:GA_GetPlayerGroundDistance(playerid, bool:ignore_interiors = true);
native Float:GA_GetVehicleGroundDistance(vehicleid, bool:ignore_interiors = true);
native bool:GA_SetVehicleInterior(vehicleid, interiorid);
native bool:GA_SetVehInterior(vehicleid, interiorid);

native bool:GA_IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:GA_IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:GA_IsPlayerInWater(playerid, weatherid = -1);
native bool:GA_IsPlayerUnderWater(playerid, weatherid = -1);
native bool:GA_IsVehicleInWater(vehicleid, weatherid = -1);
native bool:GA_IsVehicleUnderWater(vehicleid, weatherid = -1);
native bool:GA_GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);

native GA_RemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
native GA_ApplyObject(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, bool:force_replace = false, Float:custom_sizex = 0.0, Float:custom_sizey = 0.0, Float:custom_sizez = 0.0);
native bool:GA_SavePatches(const filename[]);
native bool:GA_LoadPatches(const filename[]);

native bool:GA_IsGroundSafeForSpawn(Float:x, Float:y, Float:max_slope = 15.0);
native bool:GA_GetSafeGroundSpawn(Float:search_x, Float:search_y, &Float:safe_x, &Float:safe_y, &Float:safe_z, Float:max_slope = 15.0, Float:search_radius = 50.0);
native bool:GA_GetGroundImpactTime(Float:x, Float:y, Float:z, Float:vz, &Float:seconds_to_impact, &Float:impact_z, Float:gravity = 9.8);
native bool:GA_IsPointInRadarShadow(Float:radar_x, Float:radar_y, Float:radar_z, Float:target_x, Float:target_y, Float:target_z);
native bool:GA_GetAreaElevationStats(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:min_z, &Float:max_z, &Float:avg_z, &Float:max_slope_deg);

#else
// =========================================================================
// 1. Engine Lifecycle & Initialization
// =========================================================================
native GroundAndreas_Init(const file_path[] = "scriptfiles/SAfull.hmap");
native GroundAndreas_Unload();
native bool:GroundAndreas_IsLoaded();

// =========================================================================
// 2. Core Elevation Queries
// =========================================================================
native GetGroundZPos(Float:x, Float:y, &Float:z, bool:interpolate = true, bool:water_surface = true);
native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true, bool:water_surface = true);
native bool:GetGroundZPosArea(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:avg_z, &Float:max_z = 0.0, &Float:min_z = 0.0, bool:water_surface = true);
native bool:GetGroundZPosRadius(Float:x, Float:y, Float:radius, &Float:avg_z, &Float:max_z = 0.0, &Float:min_z = 0.0, bool:water_surface = true);

// Ergonomic & Legacy Aliases
#define GetGroundZ(%0,%1,%2)            GetGroundZPos(%0,%1,%2)
#define GetGroundZBatch(%0,%1,%2,%3)    GetGroundZPosBatch(%0,%1,%2,%3)

// =========================================================================
// 3. Raycasting, Normals & Surface Alignment
// =========================================================================
native bool:IsWorldBlockingLine(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2, &Float:hit_x = 0.0, &Float:hit_y = 0.0, &Float:hit_z = 0.0);
native GetGroundNormal(Float:x, Float:y, &Float:nx, &Float:ny, &Float:nz, &Float:slope_degrees);
native GetGroundObjectRotation(Float:x, Float:y, Float:facing_angle, &Float:rx, &Float:ry);
native GetGroundObjectPose(Float:x, Float:y, Float:facing_angle, Float:z_offset, &Float:out_z, &Float:out_rx, &Float:out_ry, bool:interpolate = true);
native GetGroundTwoPointRotation(Float:x1, Float:y1, Float:x2, Float:y2, Float:z_offset, &Float:center_z, &Float:rx, &Float:ry, &Float:rz, bool:interpolate = true);
native GetGroundLinePoints(Float:start_x, Float:start_y, Float:end_x, Float:end_y, Float:spacing, Float:z_offset, Float:out_x[], Float:out_y[], Float:out_z[], Float:out_rx[], Float:out_ry[], Float:out_rz[], max_points = sizeof(out_x));
native bool:IsGroundUnderwater(Float:x, Float:y, &Float:water_depth = 0.0);
native GetGroundClearance(Float:x, Float:y, Float:z, &Float:clearance, &bool:is_underground = false);

// =========================================================================
// 4. Underground & Proximity Detection
// =========================================================================
native bool:IsPointUnderGround(Float:x, Float:y, Float:z, Float:tolerance = 0.2);
native bool:IsPlayerUnderGround(playerid, bool:ignore_interiors = true);
native bool:IsVehicleUnderGround(vehicleid, bool:ignore_interiors = true);
native bool:IsPlayerOnGround(playerid, Float:max_clearance = 0.35, bool:check_slope = true, bool:allow_water = false, bool:ignore_interiors = true);
native bool:IsVehicleOnGround(vehicleid, Float:max_clearance = 0.35, bool:check_upsidedown = true, bool:allow_water = false, bool:ignore_interiors = true);
native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
native Float:GetPlayerGroundDistance(playerid, bool:ignore_interiors = true);
native Float:GetVehicleGroundDistance(vehicleid, bool:ignore_interiors = true);
native bool:GA_SetVehInterior(vehicleid, interiorid);

// =========================================================================
// 5. Water Detection Subsystem
// =========================================================================
native bool:IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:IsPlayerInWater(playerid, weatherid = -1);
native bool:IsPlayerUnderWater(playerid, weatherid = -1);
native bool:IsVehicleInWater(vehicleid, weatherid = -1);
native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);
native bool:GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);

// =========================================================================
// 6. Dynamic Terrain & ColEditor
// =========================================================================
native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
native ApplyObjectToGroundAndreas(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, bool:force_replace = false, Float:custom_sizex = 0.0, Float:custom_sizey = 0.0, Float:custom_sizez = 0.0);
native bool:SaveGroundAndreasPatches(const filename[]);
native bool:LoadGroundAndreasPatches(const filename[]);

// =========================================================================
// 7. Terrain Analytics, Kinematics & Spawn Safety
// =========================================================================
native bool:IsGroundSafeForSpawn(Float:x, Float:y, Float:max_slope = 15.0);
native bool:GetSafeGroundSpawn(Float:search_x, Float:search_y, &Float:safe_x, &Float:safe_y, &Float:safe_z, Float:max_slope = 15.0, Float:search_radius = 50.0);
native bool:GetGroundImpactTime(Float:x, Float:y, Float:z, Float:vz, &Float:seconds_to_impact, &Float:impact_z, Float:gravity = 9.8);
native bool:IsPointInRadarShadow(Float:radar_x, Float:radar_y, Float:radar_z, Float:target_x, Float:target_y, Float:target_z);
native bool:GetAreaElevationStats(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:min_z, &Float:max_z, &Float:avg_z, &Float:max_slope_deg);
#endif

// =========================================================================
// Ensure Server Natives Exist in Script Symbol Table (T2 / N2)
// Compilers never optimize away referenced public functions or their natives.
// =========================================================================
forward __GroundAndreas_NativeRefs();
public __GroundAndreas_NativeRefs()
{
    new Float:f;
    GetPlayerPos(0, f, f, f);
    GetVehiclePos(0, f, f, f);
    GetVehicleModel(0);
    GetVehicleRotationQuat(0, f, f, f, f);
    GetPlayerInterior(0);
    GetVehicleVirtualWorld(0);
    GetPlayerVehicleID(0);
    #if defined _INC_open_mp
        GetWeather();
    #endif
}

// =========================================================================
// Automatic ALS Hooks for Vehicle Interior Tracking (T4 / N4)
// =========================================================================
#if !defined GROUNDANDREAS_NO_VEHICLE_HOOKS
    #if defined _INC_open_mp
        stock bool:GA_LinkVehicleToInterior(vehicleid, interiorid)
        {
            GA_SetVehInterior(vehicleid, interiorid);
            return bool:LinkVehicleToInterior(vehicleid, interiorid);
        }
    #else
        stock GA_LinkVehicleToInterior(vehicleid, interiorid)
        {
            GA_SetVehInterior(vehicleid, interiorid);
            return LinkVehicleToInterior(vehicleid, interiorid);
        }
    #endif
    #if defined _ALS_LinkVehicleToInterior
        #undef LinkVehicleToInterior
    #else
        #define _ALS_LinkVehicleToInterior
    #endif
    #define LinkVehicleToInterior GA_LinkVehicleToInterior

    #if defined _INC_open_mp
        stock GA_CreateVehicle(modelid, Float:x, Float:y, Float:z, Float:rotation, color1, color2, respawn_delay, bool:addsiren = false)
        {
            new veh = CreateVehicle(modelid, x, y, z, rotation, color1, color2, respawn_delay, addsiren);
            if (veh > 0 && veh <= 2000)
            {
                GA_SetVehInterior(veh, 0);
            }
            return veh;
        }
    #else
        stock GA_CreateVehicle(vehicletype, Float:x, Float:y, Float:z, Float:rotation, color1, color2, respawn_delay, addsiren = 0)
        {
            new veh = CreateVehicle(vehicletype, x, y, z, rotation, color1, color2, respawn_delay, addsiren);
            if (veh > 0 && veh <= 2000)
            {
                GA_SetVehInterior(veh, 0);
            }
            return veh;
        }
    #endif
    #if defined _ALS_CreateVehicle
        #undef CreateVehicle
    #else
        #define _ALS_CreateVehicle
    #endif
    #define CreateVehicle GA_CreateVehicle

    stock GA_AddStaticVehicle(modelid, Float:spawn_x, Float:spawn_y, Float:spawn_z, Float:z_angle, color1, color2)
    {
        new veh = AddStaticVehicle(modelid, spawn_x, spawn_y, spawn_z, z_angle, color1, color2);
        if (veh > 0 && veh <= 2000)
        {
            GA_SetVehInterior(veh, 0);
        }
        return veh;
    }
    #if defined _ALS_AddStaticVehicle
        #undef AddStaticVehicle
    #else
        #define _ALS_AddStaticVehicle
    #endif
    #define AddStaticVehicle GA_AddStaticVehicle

    #if defined _INC_open_mp
        stock GA_AddStaticVehicleEx(modelid, Float:spawn_x, Float:spawn_y, Float:spawn_z, Float:z_angle, color1, color2, respawn_delay, bool:addsiren = false)
        {
            new veh = AddStaticVehicleEx(modelid, spawn_x, spawn_y, spawn_z, z_angle, color1, color2, respawn_delay, addsiren);
            if (veh > 0 && veh <= 2000)
            {
                GA_SetVehInterior(veh, 0);
            }
            return veh;
        }
    #else
        stock GA_AddStaticVehicleEx(modelid, Float:spawn_x, Float:spawn_y, Float:spawn_z, Float:z_angle, color1, color2, respawn_delay, addsiren = 0)
        {
            new veh = AddStaticVehicleEx(modelid, spawn_x, spawn_y, spawn_z, z_angle, color1, color2, respawn_delay, addsiren);
            if (veh > 0 && veh <= 2000)
            {
                GA_SetVehInterior(veh, 0);
            }
            return veh;
        }
    #endif
    #if defined _ALS_AddStaticVehicleEx
        #undef AddStaticVehicleEx
    #else
        #define _ALS_AddStaticVehicleEx
    #endif
    #define AddStaticVehicleEx GA_AddStaticVehicleEx

    #if defined _INC_open_mp
        stock bool:GA_DestroyVehicle(vehicleid)
        {
            if (vehicleid > 0 && vehicleid <= 2000)
            {
                GA_SetVehInterior(vehicleid, 0);
            }
            return bool:DestroyVehicle(vehicleid);
        }
    #else
        stock GA_DestroyVehicle(vehicleid)
        {
            if (vehicleid > 0 && vehicleid <= 2000)
            {
                GA_SetVehInterior(vehicleid, 0);
            }
            return DestroyVehicle(vehicleid);
        }
    #endif
    #if defined _ALS_DestroyVehicle
        #undef DestroyVehicle
    #else
        #define _ALS_DestroyVehicle
    #endif
    #define DestroyVehicle GA_DestroyVehicle
#endif

// =========================================================================
// Automatic Hook for RemoveBuildingForPlayer
// Note: Changes heightmap globally on player removal. For consistent
// world geometry across all players, prefer GroundAndreasRemoveBuilding
// inside OnGameModeInit.
// =========================================================================
#if !defined GROUNDANDREAS_NO_REMOVE_BUILD && !defined GA_NO_REMOVE_BUILDING
    #if defined _INC_open_mp
        stock bool:GA_RemoveBuildingForPlayer(playerid, modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius)
        {
            #if defined GROUNDANDREAS_PREFIXED_NATIVES
                GA_RemoveBuilding(modelid, fX, fY, fZ, fRadius);
            #else
                GroundAndreasRemoveBuilding(modelid, fX, fY, fZ, fRadius);
            #endif
            return bool:RemoveBuildingForPlayer(playerid, modelid, fX, fY, fZ, fRadius);
        }
    #else
        stock GA_RemoveBuildingForPlayer(playerid, modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius)
        {
            #if defined GROUNDANDREAS_PREFIXED_NATIVES
                GA_RemoveBuilding(modelid, fX, fY, fZ, fRadius);
            #else
                GroundAndreasRemoveBuilding(modelid, fX, fY, fZ, fRadius);
            #endif
            return RemoveBuildingForPlayer(playerid, modelid, fX, fY, fZ, fRadius);
        }
    #endif

    #if defined _ALS_RemoveBuildingForPlayer
        #undef RemoveBuildingForPlayer
    #else
        #define _ALS_RemoveBuildingForPlayer
    #endif
    #define RemoveBuildingForPlayer GA_RemoveBuildingForPlayer
#endif
