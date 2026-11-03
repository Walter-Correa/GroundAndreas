# Complete API Reference

Exhaustive technical dictionary for all 32 natives, definitions, parameters, return values, and macros provided by **GroundAndreas**.

---

## Quick Navigation

* [1. Engine Lifecycle & Initialization](#1-engine-lifecycle--initialization)
* [2. Core Elevation Queries](#2-core-elevation-queries)
* [3. Raycasting, Normals & Slopes](#3-raycasting-normals--slopes)
* [4. Underground Detection Subsystem](#4-underground-detection-subsystem)
* [5. Water Detection Subsystem](#5-water-detection-subsystem)
* [6. Dynamic Terrain & Object Stamping](#6-dynamic-terrain--object-stamping)
* [7. Topographic Analytics & Kinematics](#7-topographic-analytics--kinematics)
* [8. Constants, Definitions & ALS Hooks](#8-constants-definitions--als-hooks)

---

## 1. Engine Lifecycle & Initialization

### `GroundAndreas_Init`
```pawn
native GroundAndreas_Init(const file_path[] = "scriptfiles/SAfull.hmap");
```
Opens and maps the heightmap binary file into the process virtual address space.
* **`file_path`:** Relative or absolute path to the `.hmap` file (defaults to `scriptfiles/SAfull.hmap`).
* **Returns:** `1` on successful mapping, `0` on file open/mapping failure.
* **Note:** Called automatically upon server startup if `scriptfiles/SAfull.hmap` exists.

---

### `GroundAndreas_Unload`
```pawn
native GroundAndreas_Unload();
```
Unmaps the heightmap from virtual memory and releases OS file mapping handles.
* **Returns:** `1` on success, `0` if engine was not loaded.

---

### `GroundAndreas_IsLoaded`
```pawn
native bool:GroundAndreas_IsLoaded();
```
Queries whether the heightmap engine is active and ready to process elevation queries.
* **Returns:** `true` if mapped and active, `false` otherwise.

---

## 2. Core Elevation Queries

### `GetGroundZPos`
```pawn
native GetGroundZPos(Float:x, Float:y, &Float:z, bool:interpolate = true, bool:water_surface = true);
```
Queries the ground altitude at coordinate $(x, y)$.
* **`x`, `y`:** 2D world coordinates (bounded between $-3000.0$ and $+3000.0$).
* **`&z`:** Output variable receiving the calculated height.
* **`interpolate`:** `true` (default) for sub-millimeter bilinear surface interpolation; `false` for raw 1m grid data.
* **`water_surface`:** `true` (default) to clamp ocean elevations to sea level ($Z = 0.0\text{m}$); `false` to return the submerged seabed floor.
* **Returns:** `1` on success, `0` if $(x, y)$ is out of world boundaries.

---

### `GetGroundZPosBatch`
```pawn
native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true);
```
Processes an entire array of coordinate queries inside native C++ memory without AMX context switching.
* **`points_x[]`:** Input array containing X coordinates.
* **`points_y[]`:** Input array containing Y coordinates.
* **`out_z[]`:** Output array receiving computed Z elevations.
* **`total_points`:** Total number of coordinates in the arrays.
* **`interpolate`:** `true` for bilinear interpolation; `false` for discrete grid.
* **Returns:** Total number of coordinates successfully processed.

---

### `GetGroundClearance`
```pawn
native GetGroundClearance(Float:x, Float:y, Float:z, &Float:clearance, &bool:is_underground = false);
```
Computes the vertical clearance between point $Z$ and the terrain floor.
* **`clearance`:** Output vertical distance ($z - z_{\text{ground}}$). Positive indicates above ground; negative indicates underground.
* **`is_underground`:** Output boolean set to `true` if $z < z_{\text{ground}}$.
* **Returns:** `1` on success, `0` out of bounds.

---

### `IsGroundUnderwater`
```pawn
native bool:IsGroundUnderwater(Float:x, Float:y, &Float:water_depth = 0.0);
```
Checks if the natural terrain floor at $(x, y)$ is lower than the ocean line ($Z < 0.0\text{m}$).
* **`&water_depth`:** Output receiving the depth of the ocean water column ($0.0 - z_{\text{terrain}}$).
* **Returns:** `true` if submerged beneath sea level, `false` if dry land.

---

## 3. Raycasting, Normals & Slopes

### `IsWorldBlockingLine`
```pawn
native bool:IsWorldBlockingLine(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2, &Float:hit_x = 0.0, &Float:hit_y = 0.0, &Float:hit_z = 0.0);
```
Traces a 3D ray between two points to verify if natural terrain or buildings occlude the line of sight.
* **`x1, y1, z1`:** Origin coordinates.
* **`x2, y2, z2`:** Destination coordinates.
* **`&hit_x, &hit_y, &hit_z`:** Output variables receiving the exact collision coordinates.
* **Returns:** `true` if occluded by world geometry; `false` if line of sight is clear.

---

### `GetGroundNormal`
```pawn
native GetGroundNormal(Float:x, Float:y, &Float:nx, &Float:ny, &Float:nz, &Float:slope_degrees);
```
Computes the perpendicular unit normal vector $\vec{N}$ and the terrain slope inclination angle.
* **`&nx, &ny, &nz`:** Output variables receiving the 3D unit normal vector components ($\|\vec{N}\| = 1$).
* **`&slope_degrees`:** Output variable receiving the slope angle in degrees ($0.0^\circ$ = flat, $90.0^\circ$ = vertical cliff).
* **Returns:** `1` on success, `0` out of bounds.

---

### `GetGroundObjectRotation`
```pawn
native GetGroundObjectRotation(Float:x, Float:y, Float:facing_angle, &Float:rx, &Float:ry);
```
Calculates Pitch ($RX$) and Roll ($RY$) rotation angles to align dynamic objects flush against sloped terrain for any given compass heading (`facing_angle`).
* **`facing_angle`:** Object heading in degrees ($0.0^\circ$ to $360.0^\circ$).
* **`&rx, &ry`:** Output variables receiving Pitch ($RX$) and Roll ($RY$) in degrees.
* **Returns:** `1` on success, `0` out of bounds.

---

## 4. Underground Detection Subsystem

### `IsPointUnderGround`
```pawn
native bool:IsPointUnderGround(Float:x, Float:y, Float:z);
```
* **Returns:** `true` if $(x, y, z)$ is beneath the terrain surface ($Z < Z_{\text{ground}} - 0.05\text{m}$).

---

### `IsPlayerUnderGround`
```pawn
native bool:IsPlayerUnderGround(playerid);
```
Checks if the player's feet are beneath the terrain floor ($Z_{\text{feet}} = Z - 0.90\text{m}$).
* **Returns:** `true` if player's feet are beneath ground, `false` otherwise.

---

### `IsVehicleUnderGround`
```pawn
native bool:IsVehicleUnderGround(vehicleid);
```
Looks up the vehicle's model ID, fetches its exact physical height from the internal database, and checks if the wheels are beneath the terrain ($Z_{\text{wheels}} = Z - \frac{\text{height}}{2}$).
* **Returns:** `true` if vehicle undercarriage/wheels are beneath ground, `false` otherwise.

---

### `GetPointGroundDistance`
```pawn
native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
```
* **Returns:** Vertical distance in meters ($Z - Z_{\text{ground}}$). Positive above ground; negative underground.

---

### `GetPlayerGroundDistance`
```pawn
native Float:GetPlayerGroundDistance(playerid);
```
* **Returns:** Vertical distance in meters between the player's feet and the ground.

---

### `GetVehicleGroundDistance`
```pawn
native Float:GetVehicleGroundDistance(vehicleid);
```
* **Returns:** Vertical distance in meters between the vehicle's wheels and the ground.

---

## 5. Water Detection Subsystem

### `IsPointInWater`
```pawn
native bool:IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
```
* **Returns:** `true` if point makes contact with ocean or inland water body surface.

---

### `IsPointUnderWater`
```pawn
native bool:IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);
```
* **Returns:** `true` if point is submerged beneath the water surface.

---

### `IsPlayerInWater`
```pawn
native bool:IsPlayerInWater(playerid, weatherid = -1);
```
* **Returns:** `true` if player's feet touch water (swimming, wading).

---

### `IsPlayerUnderWater`
```pawn
native bool:IsPlayerUnderWater(playerid, weatherid = -1);
```
* **Returns:** `true` if player's head is completely submerged ($Z + 0.90\text{m} \le Z_{\text{water}}$).

---

### `IsVehicleInWater`
```pawn
native bool:IsVehicleInWater(vehicleid, weatherid = -1);
```
* **Returns:** `true` if vehicle wheels make contact with water surface.

---

### `IsVehicleUnderWater`
```pawn
native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);
```
* **Returns:** `true` if vehicle roof is completely submerged ($Z + \frac{\text{height}}{2} \le Z_{\text{water}}$).

---

### `GetWaterLevel`
```pawn
native GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);
```
* **`&water_z`:** Output variable receiving the water surface altitude at $(x, y)$.
* **Returns:** `1` if water exists at coordinates, `0` if dry land.

---

## 6. Dynamic Terrain & Object Stamping

### `GroundAndreasRemoveBuilding`
```pawn
native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
```
Reconstructs natural ground slope beneath a demolished building using perimeter least-squares gradient fitting and lower-quartile terrain filtering.
* **Returns:** `1` on success, `0` on error.

---

### `ApplyObjectToGroundAndreas`
```pawn
native ApplyObjectToGroundAndreas(
    modelid,
    Float:x, Float:y, Float:z,
    Float:rx, Float:ry, Float:rz,
    bool:force_replace = false,
    Float:custom_sizex = 0.0,
    Float:custom_sizey = 0.0,
    Float:custom_sizez = 0.0
);
```
Stamps an inclined dynamic 3D object (ramp, bridge, road, helipad) onto the heightfield with full Euler rotations.
* **`force_replace`:** `true` overwrites existing ground regardless of height; `false` (default) only stamps if higher than ground.
* **`custom_sizex, custom_sizey, custom_sizez`:** Optional custom model dimensions (`0.0` uses internal catalog).
* **Returns:** `1` on success, `0` on error.

---

### `SaveGroundAndreasPatches`
```pawn
native bool:SaveGroundAndreasPatches(const filename[]);
```
Serializes all active runtime heightfield modifications into a compact binary `.dat` file.
* **Returns:** `true` on successful save, `false` otherwise.

---

### `LoadGroundAndreasPatches`
```pawn
native bool:LoadGroundAndreasPatches(const filename[]);
```
Loads and applies pre-baked binary heightfield patches into memory in $< 1\text{ms}$.
* **Returns:** `true` on successful load, `false` on file error.

---

## 7. Topographic Analytics & Kinematics

### `IsGroundSafeForSpawn`
```pawn
native bool:IsGroundSafeForSpawn(Float:x, Float:y, Float:max_slope = 15.0);
```
Validates if $(x, y)$ is within world bounds, dry (not in water), and has a slope $\le \text{max\_slope}^\circ$.
* **Returns:** `true` if safe, `false` otherwise.

---

### `GetSafeGroundSpawn`
```pawn
native bool:GetSafeGroundSpawn(
    Float:search_x, Float:search_y,
    &Float:safe_x, &Float:safe_y, &Float:safe_z,
    Float:max_slope = 15.0,
    Float:search_radius = 50.0
);
```
Executes an outward concentric search around $(search_x, search_y)$ to find the nearest safe, dry, flat spawn location.
* **Returns:** `true` if found, `false` if entire radius is invalid.

---

### `GetGroundImpactTime`
```pawn
native bool:GetGroundImpactTime(
    Float:x, Float:y, Float:z,
    Float:vz,
    &Float:seconds_to_impact,
    &Float:impact_z,
    Float:gravity = 9.8
);
```
Solves vertical ballistic kinematics under gravity to predict collision time and altitude.
* **`vz`:** Vertical velocity in units/second.
* **`&seconds_to_impact`:** Estimated seconds until terrain collision.
* **`&impact_z`:** Collision altitude.
* **Returns:** `true` if impact will occur; `false` if ascending or no impact.

---

### `IsPointInRadarShadow`
```pawn
native bool:IsPointInRadarShadow(
    Float:radar_x, Float:radar_y, Float:radar_z,
    Float:target_x, Float:target_y, Float:target_z
);
```
Performs raycast line-of-sight occlusion between a radar/SAM emitter and a target aircraft.
* **Returns:** `true` if terrain occludes line of sight (target is masked in shadow); `false` if exposed.

---

### `GetAreaElevationStats`
```pawn
native bool:GetAreaElevationStats(
    Float:minx, Float:miny, Float:maxx, Float:maxy,
    &Float:min_z, &Float:max_z, &Float:avg_z, &Float:max_slope_deg
);
```
Computes regional topographic metrics across bounding box $[min_x, min_y] \times [max_x, max_y]$.
* **`&min_z, &max_z`:** Lowest and highest elevations in the zone.
* **`&avg_z`:** Mean ground elevation across all sampled grid cells.
* **`&max_slope_deg`:** Steepest slope inclination angle encountered (degrees).
* **Returns:** `true` on success, `false` out of bounds.

---

## 8. Constants, Definitions & ALS Hooks

### Version Definitions
```pawn
#define GROUNDANDREAS_VERSION_MAJOR           1
#define GROUNDANDREAS_VERSION_MINOR           0
#define GROUNDANDREAS_VERSION_PATCH           0
#define GROUNDANDREAS_VERSION_STRING          "1.0.0"
```

### Automatic Hook Control
```pawn
// Define before including GroundAndreas to disable the automatic hook
#define GROUNDANDREAS_NO_REMOVE_BUILDING
#include <GroundAndreas>
```

When `GROUNDANDREAS_NO_REMOVE_BUILDING` is **not** defined, `GroundAndreas.inc` automatically hooks `RemoveBuildingForPlayer` to update the heightmap with deduplication caching.

### Legacy C++ Aliases
For backward compatibility with older scripts, the following aliases are resolved internally:
* `GetGroundZ` $\to$ `GetGroundZPos`
* `GetGroundZBatch` $\to$ `GetGroundZPosBatch`
