#if defined _GroundAndreas_included
    #endinput
#endif
#define _GroundAndreas_included

native GroundAndreas_Init(const file_path[] = "scriptfiles/SAfull.hmap");
native GroundAndreas_Unload();
native bool:GroundAndreas_IsLoaded();

native GetGroundZPos(Float:x, Float:y, &Float:z, bool:interpolate = true, bool:water_surface = true);
native bool:IsWorldBlockingLine(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2, &Float:hit_x = 0.0, &Float:hit_y = 0.0, &Float:hit_z = 0.0);
native GetGroundNormal(Float:x, Float:y, &Float:nx, &Float:ny, &Float:nz, &Float:slope_degrees);
native GetGroundObjectRotation(Float:x, Float:y, Float:facing_angle, &Float:rx, &Float:ry);
native bool:IsGroundUnderwater(Float:x, Float:y, &Float:water_depth = 0.0);
native GetGroundClearance(Float:x, Float:y, Float:z, &Float:clearance, &bool:is_underground = false);
native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true);

native bool:IsPointUnderGround(Float:x, Float:y, Float:z);
native bool:IsPlayerUnderGround(playerid);
native bool:IsVehicleUnderGround(vehicleid);
native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
native Float:GetPlayerGroundDistance(playerid);
native Float:GetVehicleGroundDistance(vehicleid);

native bool:IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:IsPlayerInWater(playerid, weatherid = -1);
native bool:IsPlayerUnderWater(playerid, weatherid = -1);
native bool:IsVehicleInWater(vehicleid, weatherid = -1);
native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);
native GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);

native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
native ApplyObjectToGroundAndreas(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, bool:force_replace = false, Float:custom_sizex = 0.0, Float:custom_sizey = 0.0, Float:custom_sizez = 0.0);

#if !defined GROUNDANDREAS_NO_REMOVE_BUILDING
    stock GroundAndreas_RemoveBuildingForPlayer(playerid, modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius)
    {
        GroundAndreasRemoveBuilding(modelid, fX, fY, fZ, fRadius);
        return RemoveBuildingForPlayer(playerid, modelid, fX, fY, fZ, fRadius);
    }

    #if defined _ALS_RemoveBuildingForPlayer
        #undef RemoveBuildingForPlayer
    #else
        #define _ALS_RemoveBuildingForPlayer
    #endif
    #define RemoveBuildingForPlayer GroundAndreas_RemoveBuildingForPlayer
#endif
