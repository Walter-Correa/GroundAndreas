# Complete API Reference

Exhaustive technical dictionary for all 40 natives, definitions, parameters, return values, and macros provided by **GroundAndreas**.

---

## Quick Navigation

* [1. Engine Lifecycle & Initialization](#1-engine-lifecycle--initialization)
* [2. Core Elevation Queries](#2-core-elevation-queries)
* [3. Raycasting, Normals, Slopes & Clearance](#3-raycasting-normals-slopes--clearance)
* [4. Underground Detection Subsystem](#4-underground-detection-subsystem)
* [5. Water Detection Subsystem](#5-water-detection-subsystem)
* [6. Dynamic Terrain & ColEditor](#6-dynamic-terrain--coleditor)
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
* **`interpolate`:** `true` (default) for continuous bilinear interpolation over a 1 m grid with 1 cm vertical resolution; `false` for raw 1m grid data.
* **`water_surface`:** Kept for API compatibility. Because the heightmap stores unsigned uint16 elevations (range $0.0\text{m}$ to $655.35\text{m}$), ocean cells always return $0.0\text{m}$ regardless of this flag.
* **Returns:** `1` on success, `0` if $(x, y)$ is out of world boundaries.

---

### `GetGroundZPosBatch`
```pawn
native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true, bool:water_surface = true);
```
Processes an entire array of coordinate queries inside native C++ memory without AMX context switching.
* **`points_x[]`:** Input array containing X coordinates.
* **`points_y[]`:** Input array containing Y coordinates.
* **`out_z[]`:** Output array receiving computed Z elevations.
* **`total_points`:** Total number of coordinates in the arrays.
* **`interpolate`:** `true` (default) for bilinear interpolation; `false` for discrete grid.
* **`water_surface`:** Kept for API compatibility; ocean cells always evaluate to $0.0\text{m}$.
* **Returns:** Total number of coordinates successfully processed.

---

### `GetGroundZPosArea`
```pawn
native bool:GetGroundZPosArea(
    Float:minx, Float:miny, Float:maxx, Float:maxy,
    &Float:avg_z,
    &Float:max_z = 0.0,
    &Float:min_z = 0.0,
    bool:water_surface = true
);
```
Computes regional ground elevation metrics across a rectangular area $[min_x, min_y] \times [max_x, max_y]$ (gangzones, turf areas).
* **`minx, miny, maxx, maxy`:** Bounding box world coordinates in meters. Inverted coordinates are automatically normalized.
* **`&avg_z`:** Output receiving the average ground elevation across all sampled nodes in the area.
* **`&max_z`:** Output receiving the highest ground elevation in the area.
* **`&min_z`:** Output receiving the lowest ground elevation in the area.
* **`water_surface`:** Kept for API compatibility; ocean cells evaluate to $0.0\text{m}$.
* **Returns:** `true` on success, `false` out of bounds or engine not loaded.

---

### `GetGroundZPosRadius`
```pawn
native bool:GetGroundZPosRadius(
    Float:x, Float:y, Float:radius,
    &Float:avg_z,
    &Float:max_z = 0.0,
    &Float:min_z = 0.0,
    bool:water_surface = true
);
```
Computes circular radial ground elevation metrics within a circle of radius $R$ centered at $(x, y)$.
* **`x, y`:** Center world coordinates.
* **`radius`:** Radius in meters ($R > 0.0$).
* **`&avg_z`:** Output receiving the average ground elevation in the radius.
* **`&max_z`:** Output receiving the highest ground elevation in the radius.
* **`&min_z`:** Output receiving the lowest ground elevation in the radius.
* **`water_surface`:** Kept for API compatibility; ocean cells evaluate to $0.0\text{m}$.
* **Returns:** `true` on success, `false` out of bounds or radius $\le 0.0$.

---

## 3. Raycasting, Normals, Slopes & Clearance

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

### `GetGroundObjectPose`
```pawn
native GetGroundObjectPose(Float:x, Float:y, Float:facing_angle, Float:z_offset, &Float:out_z, &Float:out_rx, &Float:out_ry, bool:interpolate = true);
```
Calculates combined ground elevation and slope rotation angles (Pitch and Roll) to place an object at $(x, y)$ oriented along `facing_angle`.
* **`facing_angle`:** Object heading in degrees ($0.0^\circ$ to $360.0^\circ$).
* **`z_offset`:** Vertical elevation offset to add above terrain surface (meters).
* **`&out_z`:** Output variable receiving final object elevation ($Z_{\text{ground}} + z_{\text{offset}}$).
* **`&out_rx, &out_ry`:** Output variables receiving Pitch ($RX$) and Roll ($RY$) in degrees.
* **`interpolate`:** `true` (default) for bilinear surface interpolation; `false` for raw cell sampling.
* **Returns:** `1` on success, `0` out of bounds or heightmap not loaded.

---

### `GetGroundTwoPointRotation`
```pawn
native GetGroundTwoPointRotation(
    Float:x1, Float:y1, Float:x2, Float:y2, Float:z_offset,
    &Float:center_z, &Float:rx, &Float:ry, &Float:rz,
    bool:interpolate = true
);
```
Calculates exact RenderWare 3D Euler angles ($RX, RY, RZ$) and midpoint elevation to span an object (barricade, bridge deck, ramp) directly between two arbitrary ground coordinates.
* **`x1, y1, x2, y2`:** World start and end coordinates.
* **`z_offset`:** Height offset added to the calculated midpoint Z (meters).
* **`&center_z`:** Output variable receiving the midpoint elevation ($\frac{z_1 + z_2}{2} + z_{\text{offset}}$).
* **`&rx, &ry, &rz`:** Output variables receiving Roll ($RX = 0.0^\circ$), Pitch ($RY = -\text{pitch}$), and Yaw ($RZ = \text{atan2}(dy, dx)$) in degrees.
* **`interpolate`:** `true` (default) for bilinear surface interpolation; `false` for raw grid.
* **Returns:** `1` on success, `0` if coordinates are identical, invalid, or out of bounds.

---

### `GetGroundLinePoints`
```pawn
native GetGroundLinePoints(
    Float:start_x, Float:start_y, Float:end_x, Float:end_y, Float:spacing, Float:z_offset,
    Float:out_x[], Float:out_y[], Float:out_z[],
    Float:out_rx[], Float:out_ry[], Float:out_rz[],
    max_points = sizeof(out_x)
);
```
Samples equidistant nodes along a 2D line segment between $(start\_x, start\_y)$ and $(end\_x, end\_y)$, computing the contoured ground elevation ($Z$) and surface slope orientation ($RX, RY, RZ$) at each point. Ideal for generating roadside barriers, fences, power poles, or cone chains.
* **`start_x, start_y`:** Start coordinates of the line segment.
* **`end_x, end_y`:** End coordinates of the line segment.
* **`spacing`:** Distance in meters between successive sampled points ($> 0.05\text{m}$).
* **`z_offset`:** Vertical elevation offset added to all sampled points.
* **`out_x[], out_y[], out_z[]`:** Output arrays receiving node coordinates.
* **`out_rx[], out_ry[], out_rz[]`:** Output arrays receiving surface orientation angles.
* **`max_points`:** Capacity of output arrays (defaults to `sizeof(out_x)`).
* **Returns:** Total number of points generated along the line.

---

### `IsGroundUnderwater`
```pawn
native bool:IsGroundUnderwater(Float:x, Float:y, &Float:water_depth = 0.0);
```
Checks if water exists at $(x, y)$ and the ground terrain is below the water surface level.
* **`&water_depth`:** Output receiving the water column depth ($Z_{\text{surface}} - Z_{\text{ground}}$). For inland lakes and pools, this reflects physical water depth against the mapped basin floor; in the ocean (where the heightmap floor is $0.0\text{m}$), this reflects the active wave swell height ($0.15\text{m}$, $0.50\text{m}$, or $0.80\text{m}$).
* **Returns:** `true` if water is present above the terrain, `false` on dry land.

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

## 4. Underground Detection Subsystem

### `IsPointUnderGround`
```pawn
native bool:IsPointUnderGround(Float:x, Float:y, Float:z, Float:tolerance = 0.2);
```
* **`tolerance`:** Penetration margin in meters (defaults to `0.2m`).
* **Returns:** `true` if $(x, y, z)$ is beneath the terrain surface ($Z < Z_{\text{ground}} - \text{tolerance}$).

---

### `IsPlayerUnderGround`
```pawn
native bool:IsPlayerUnderGround(playerid, bool:ignore_interiors = true);
```
Checks if the player's feet are beneath the terrain floor ($Z_{\text{feet}} = Z - 1.0\text{m}$).
* **`ignore_interiors`:** When `true` (default), gracefully bypasses the check if the player is in an interior ($> 0$).
* When the player is inside a vehicle, IsPlayerOnGround, IsPlayerUnderGround and GetPlayerGroundDistance evaluate the player's vehicle (same result as the vehicle natives).
* Automatically returns `false` if the player is in water (preventing false underground flags while swimming or diving in ocean waters where no seabed is modeled).
* **Returns:** `true` if player's feet are beneath ground, `false` otherwise.

---

### `IsVehicleUnderGround`
```pawn
native bool:IsVehicleUnderGround(vehicleid, bool:ignore_interiors = true);
```
Checks if the vehicle center pivot is more than $0.5\text{m}$ beneath the ground surface ($Z < Z_{\text{ground}} - 0.5\text{m}$).
* **`ignore_interiors`:** When `true` (default), gracefully bypasses the check if the vehicle is in an interior ($> 0$).
* Automatically returns `false` if the vehicle is in water.
* **Returns:** `true` if vehicle is beneath ground, `false` otherwise.

---

### `IsPlayerOnGround`
```pawn
native bool:IsPlayerOnGround(playerid, Float:max_clearance = 0.35, bool:check_slope = true, bool:allow_water = false, bool:ignore_interiors = true);
```
Determines if a player is physically standing on or in immediate contact with solid ground.
* **`playerid`:** Target player ID.
* **`max_clearance`:** Maximum vertical clearance in meters between player feet and terrain ($0.35\text{m}$ default).
* **`check_slope`:** When `true` (default), rejects ground contact if the slope exceeds $60.0^\circ$ (sheer cliff faces).
* **`allow_water`:** When `false` (default), swimming or water surface contact does not count as solid ground.
* **`ignore_interiors`:** When `true` (default), gracefully bypasses the check and returns `false` if the player is in an interior ($> 0$).
* When the player is inside a vehicle, IsPlayerOnGround, IsPlayerUnderGround and GetPlayerGroundDistance evaluate the player's vehicle (same result as the vehicle natives).
* **Returns:** `true` if standing on solid ground; `false` otherwise.

---

### `IsVehicleOnGround`
```pawn
native bool:IsVehicleOnGround(vehicleid, Float:max_clearance = 0.35, bool:check_upsidedown = true, bool:allow_water = false, bool:ignore_interiors = true);
```
Determines if a vehicle's wheels are physically resting on solid terrain, factoring in individual vehicle model base offsets.
* **`vehicleid`:** Target vehicle ID.
* **`max_clearance`:** Maximum vertical clearance in meters between tire contact level and terrain ($0.35\text{m}$ default).
* **`check_upsidedown`:** When `true` (default), returns `false` if the vehicle is flipped or inverted.
* **`allow_water`:** When `false` (default), floating or submerged vehicles do not qualify as on solid ground.
* **`ignore_interiors`:** When `true` (default), gracefully bypasses the check and returns `false` if the vehicle is in an interior ($> 0$).
* **Returns:** `true` if resting on solid ground; `false` otherwise.

---

### `GetPointGroundDistance`
```pawn
native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
```
* **Returns:** Vertical distance in meters ($Z - Z_{\text{ground}}$). Positive above ground; negative underground.

---

### `GetPlayerGroundDistance`
```pawn
native Float:GetPlayerGroundDistance(playerid, bool:ignore_interiors = true);
```
* **`ignore_interiors`:** When `true` (default), returns `0.0` if player is in an interior ($> 0$).
* When the player is inside a vehicle, IsPlayerOnGround, IsPlayerUnderGround and GetPlayerGroundDistance evaluate the player's vehicle (same result as the vehicle natives).
* **Returns:** Vertical distance in meters between the player's feet and the ground (`0.0` when standing/grounded, positive when airborne, negative when underground/in void).
* **Ocean Water Note:** Because the heightmap stores elevations clamped at sea level ($0.0\text{m}$), negative values in ocean waters represent depth beneath the sea surface rather than distance to a physical seabed.

---

### `GetVehicleGroundDistance`
```pawn
native Float:GetVehicleGroundDistance(vehicleid, bool:ignore_interiors = true);
```
* **`ignore_interiors`:** When `true` (default), returns `0.0` if vehicle is in an interior ($> 0$).
* **Returns:** Vertical distance in meters between the vehicle contact base/wheels and the ground (`0.0` when landed/grounded, positive when airborne, negative when underground/in void).
* **Ocean Water Note:** In ocean waters, negative values represent depth beneath sea level.

---

## 5. Water Detection Subsystem

Utilizes the dedicated **44-volume 3D water catalog** (covering all inland lakes, fountains, casino pools, reservoirs, and rivers) and ocean surface levels with dynamic weather-based wave amplitudes (0.15m calm, 0.50m rain/storm/fog/overcast (4, 7, 8, 12, 14–16), 0.80m weather 19 (sandstorm)).

### `IsPointInWater`
```pawn
native bool:IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
```
* **`weatherid`:** GTA SA weather ID (defaults to `-1` for automatic server weather detection).
* **Returns:** `true` if point makes contact with ocean or inland water body surface.

---

### `IsPointUnderWater`
```pawn
native bool:IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);
```
* **`weatherid`:** GTA SA weather ID (defaults to `-1` for automatic server weather detection).
* **Returns:** `true` if point is submerged beneath the water surface.

---

### `IsPlayerInWater`
```pawn
native bool:IsPlayerInWater(playerid, weatherid = -1);
```
* **`weatherid`:** GTA SA weather ID (defaults to `-1` for automatic server weather detection).
* **Returns:** `true` if player body makes contact with water ($Z_{\text{player}} \le Z_{\text{water}} + 0.35\text{m}$; immersion from waist depth $\approx 0.65\text{m}$) and the player is not beneath the water body floor.

---

### `IsPlayerUnderWater`
```pawn
native bool:IsPlayerUnderWater(playerid, weatherid = -1);
```
* **`weatherid`:** GTA SA weather ID (defaults to `-1` for automatic server weather detection).
* **Returns:** `true` if player's head is completely submerged ($Z + 0.90\text{m} \le Z_{\text{water}}$).

---

### `IsVehicleInWater`
```pawn
native bool:IsVehicleInWater(vehicleid, weatherid = -1);
```
* **`weatherid`:** GTA SA weather ID (defaults to `-1` for automatic server weather detection).
* **Returns:** `true` if vehicle wheels make contact with water surface.

---

### `IsVehicleUnderWater`
```pawn
native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);
```
* **`weatherid`:** GTA SA weather ID (defaults to `-1` for automatic server weather detection).
* **Returns:** `true` if vehicle roof is completely submerged ($Z + \frac{\text{height}}{2} \le Z_{\text{water}}$).

---

### `GetWaterLevel`
```pawn
native bool:GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);
```
* **`&water_z`:** Output variable receiving the water surface altitude at $(x, y)$.
* **`weatherid`:** GTA SA weather ID (defaults to `-1` for automatic server weather detection).
* **Returns:** `true` if water exists at coordinates, `false` if dry land.

---

## 6. Dynamic Terrain & ColEditor

### `GroundAndreasRemoveBuilding`
```pawn
native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
```
Reconstructs natural ground elevation beneath a demolished building using rooftop plateau detection (BFS flood fill + Andrew's Monotone Chain convex hull) and surrounding street lower envelope regression (iterative least-squares plane fitting).
* **Parameters:**
  * `modelid`: Building model ID (wildcard `-1` is unsupported and ignored).
  * `fX, fY, fZ`: Building world position coordinates.
  * `fRadius`: Search radius parameter from `RemoveBuildingForPlayer` (does not define the excavation area; excavation geometry is strictly determined by the detected rooftop contour).
* **Known Limitations:**
  1. Connected neighboring structures sharing the same roof height are carved together.
  2. Removing only an object's LOD model independently also triggers excavation.
  3. Buildings surrounded entirely by raised plazas/terraces stay at terrace height.
  4. Steep roof tiers ($> 1.5\text{m}$ step) outside the convex hull footprint are not lowered.
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
#define GROUNDANDREAS_VERSION_MINOR           1
#define GROUNDANDREAS_VERSION_PATCH           0
#define GROUNDANDREAS_VERSION_STRING          "1.1.0"
```

### Anti-Collision Prefixed Mode
Define before including `GroundAndreas.inc` to expose **only** prefixed natives (`GA_...`), preventing naming collisions with third-party libraries:
```pawn
#define GROUNDANDREAS_PREFIXED_NATIVES
#include <GroundAndreas>

// All natives are declared exclusively under the GA_ namespace:
// GA_Init, GA_Unload, GA_IsLoaded, GA_GetGroundZPos, GA_GetGroundNormal, etc.
```

### Automatic Hook Control

#### 1. Building Removal Hook (`GROUNDANDREAS_NO_REMOVE_BUILD` / `GA_NO_REMOVE_BUILDING`)
```pawn
// Define before including GroundAndreas to disable the automatic hook
#define GROUNDANDREAS_NO_REMOVE_BUILD
#include <GroundAndreas>
```
When neither `GROUNDANDREAS_NO_REMOVE_BUILD` nor `GA_NO_REMOVE_BUILDING` is defined, `GroundAndreas.inc` automatically hooks `RemoveBuildingForPlayer` to update the heightmap with deduplication caching.

#### 2. Vehicle Interior Tracking Hooks (`GROUNDANDREAS_NO_VEHICLE_HOOKS`)
```pawn
// Define before including GroundAndreas to disable vehicle interior ALS hooks
#define GROUNDANDREAS_NO_VEHICLE_HOOKS
#include <GroundAndreas>
```
When `GROUNDANDREAS_NO_VEHICLE_HOOKS` is **not** defined, `GroundAndreas.inc` automatically intercepts:
* `LinkVehicleToInterior`
* `CreateVehicle`
* `AddStaticVehicle`
* `AddStaticVehicleEx`
* `DestroyVehicle`

This ensures that whenever a vehicle is assigned to an interior in SA-MP, `GA_SetVehInterior(vehicleid, interiorid)` is automatically called, guaranteeing that `IsVehicleUnderGround` never throws false underground alerts inside garages or underground arenas.

### Manual Vehicle Interior Tracker
```pawn
native bool:GA_SetVehInterior(vehicleid, interiorid);
// In prefixed mode:
native bool:GA_SetVehicleInterior(vehicleid, interiorid);
native bool:GA_SetVehInterior(vehicleid, interiorid);
```
Directly registers or overrides a vehicle's interior ID in the internal tracker.

### Legacy Aliases
For backward compatibility with older scripts, the following aliases are resolved internally:
* `GetGroundZ` $\to$ `GetGroundZPos`
* `GetGroundZBatch` $\to$ `GetGroundZPosBatch`
* `GroundAndreas_SetVehicleInterior` $\to$ `GA_SetVehInterior`
