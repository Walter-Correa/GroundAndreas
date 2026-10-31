# GroundAndreas

[![Build & Release](https://github.com/Walter-Correa/GroundAndreas/actions/workflows/build.yml/badge.svg)](https://github.com/Walter-Correa/GroundAndreas/actions/workflows/build.yml)
[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
[![Platform](https://img.shields.io/badge/Platform-SA--MP%200.3.7%20%7C%20open.mp-blue.svg)](https://open.mp)

**GroundAndreas** is an ultra-fast, modern, memory-mapped heightmap engine, 2.5D raycasting, and high-precision underground/water detection plugin and component for **SA-MP** (San Andreas Multiplayer) and **open.mp** servers.

It replaces legacy solutions like MapAndreas and heavy 3D physics engines like ColAndreas, delivering millimeter-smooth continuous terrain elevation, 2.5D world-blocking line-of-sight checks, surface normal vectors, object slope alignment, vehicle/player underground collision analysis, and comprehensive water dynamics (ocean and inland bodies) with **instantaneous boot times** and **near-zero CPU overhead**.

---

## Benchmark & Comparison

| Feature / Metric | MapAndreas (Legacy) | ColAndreas (Bullet Physics) | **GroundAndreas (Modern)** |
| :--- | :---: | :---: | :---: |
| **Physical RAM Usage** | ~72 MB (Fixed static buffer) | 350 MB - 500+ MB | **~10 - 20 MB** (Kernel demand paging) |
| **Server Boot Latency** | 1.0 - 3.0s (`fread` disk lag) | 3.0 - 8.0s (3D BVH tree load) | **0.001s (Instantaneous virtual map)** |
| **Z-Query Speed** | ~2.0 ns | ~500 - 2,000 ns | **~1.5 - 3.0 ns** ($O(1)$) |
| **Terrain Precision** | 1.0m discrete staircase steps | 3D mesh vertices | **Continuous Bilinear Interpolation** |
| **Line-of-Sight Raycast** | ❌ No | Yes (Heavy 3D physics) | **Yes (`IsWorldBlockingLine`, < 1.0 µs)** |
| **Surface Normal & Slope** | ❌ No | Complex physics contact | **Yes ($\vec{N}$ & slope in degrees)** |
| **Dynamic Object Alignment** | ❌ No | ❌ No | **Yes (Pitch & Roll calculation)** |
| **Underground Detection** | ❌ No | ❌ No | **Yes (Point, Player, Vehicle)** |
| **Vehicle Dimension Physics** | ❌ No | Heavy mesh bounding | **Built-in 212 SA Model Heights** |
| **Comprehensive Water System** | ❌ No (Returns seabed) | Material configuration | **Yes (Ocean + 16 Inland Bodies + Weather)** |
| **Batch Array Processing** | ❌ No | ❌ No | **Yes (`GetGroundZPosBatch`)** |
| **open.mp Component SDK** | ❌ No | ❌ No | **Yes (Native open.mp Component)** |

---

## Key Features

1. **Memory-Mapped I/O (`MapViewOfFile` / `mmap`)**
   - The entire 72 MB heightmap (`SAfull.hmap`) is mapped directly into virtual address space.
   - Booting takes **under 1 millisecond** without blocking the server main thread.
   - The OS kernel automatically pages in only the 4 KB sectors requested (e.g., active player areas like Los Santos), keeping real RAM consumption around **10 to 20 MB**.

2. **Continuous Bilinear Interpolation**
   - Eliminates the jagged 1-meter staircases of older heightmaps.
   - Calculates smooth, sub-millimeter elevations across hills, mountain trails, and ramps.

3. **2.5D World Line-of-Sight Raycasting (`IsWorldBlockingLine`)**
   - Traces 3D rays across the world to detect whether mountains, buildings, or bridges obstruct lines of sight.
   - Replaces 85% of ColAndreas use cases (anti-cheat line-of-sight, sniper visibility, radar tracking) while running in under 1 microsecond.

4. **Surface Normal Vector & Slope Calculation (`GetGroundNormal`)**
   - Computes normalized 3D surface vectors $\vec{N} = (n_x, n_y, n_z)$ and terrain angle in degrees ($0^\circ$ flat to $90^\circ$ vertical).
   - Ideal for anti-hill-climb logic, fall damage multipliers, and vehicle traction physics.

5. **Dynamic Object Alignment on Slopes (`GetGroundObjectRotation`)**
   - Given an object heading (`facing_angle`), calculates the exact Pitch ($RX$) and Roll ($RY$) needed to sit objects (police barriers, traffic cones, tents, ramps) flush on inclined roads.

6. **Precision Underground Detection Engine**
   - Accurately checks whether points, players, or vehicles are underground or inside subterranean spaces.
   - Player checks automatically calculate the foot contact level ($Z_{\text{feet}} = Z_{\text{player}} - 0.90\text{m}$).
   - Vehicle checks automatically look up the exact model bounding box height from a compiled table of all 212 GTA SA vehicle models ($Z_{\text{wheels}} = Z_{\text{pivot}} - \frac{\text{height}}{2}$).
   - Direct float distance returns: `GetPlayerGroundDistance` and `GetVehicleGroundDistance`.

7. **Advanced Water Detection Subsystem**
   - Distinguishes **surface contact** (`InWater` - swimming, wading, floating) from **full submersion** (`UnderWater` - diving, sunken vehicles).
   - Covers global ocean bathymetry with dynamic wave amplitude tables mapped to San Andreas weather IDs.
   - Built-in volume boundaries and floor elevations for 16 major inland water bodies (Sherman Dam Reservoir, Glen Park Lake, Palomino Lagoon, airport canals, casino water spectacles, mansion swimming pools, and interior pools) to completely prevent false positives in tunnels or cellars beneath water.

8. **Dynamic Terrain Modification & Object Surface Stamping**
   - Reconstructs natural ground slopes when GTA buildings are removed (`GroundAndreasRemoveBuilding`), using least-squares perimeter plane fitting and lower-quartile terrain anchoring to avoid sampling neighboring structures.
   - Automatically hooks SA-MP's `RemoveBuildingForPlayer` with deduplication caching (run once per structure, zero runtime overhead).
   - Stamps dynamic mapped objects (`ApplyObjectToGroundAndreas`) onto the heightmap with full 3D Euler rotations (Pitch, Roll, Yaw) and exact inclined plane equations (helipads, roads, ramps, bridges).

9. **Dual-Target: Native open.mp Component & Traditional SA-MP Plugin**
   - Compiles both as a modern open.mp component (`components/GroundAndreas.dll` / `.so`) and a traditional SA-MP 0.3.7 plugin (`plugins/GroundAndreas.dll` / `.so`).

---

## Installation

### For open.mp
1. Copy `components/GroundAndreas.dll` (Windows) or `components/GroundAndreas.so` (Linux) to your server's `components/` directory (open.mp automatically detects and loads components placed in this directory).
2. Copy `scriptfiles/SAfull.hmap` to your server's `scriptfiles/` directory.
3. Copy `GroundAndreas.inc` to your `pawno/include/` or `qawno/include/` folder.

### For SA-MP 0.3.7
1. Copy `plugins/GroundAndreas.dll` (Windows) or `plugins/GroundAndreas.so` (Linux) to your server's `plugins/` directory.
2. Copy `scriptfiles/SAfull.hmap` to your server's `scriptfiles/` directory.
3. Copy `GroundAndreas.inc` to your `pawno/include/` folder.
4. Add `GroundAndreas` to `server.cfg`:
   ```
   plugins GroundAndreas
   ```

### For sampctl
```bash
sampctl package install Walter-Correa/GroundAndreas
```

---

## Pawn API Reference

### Initialization & Lifecycle

```pawn
native GroundAndreas_Init(const file_path[] = "scriptfiles/SAfull.hmap");
native GroundAndreas_Unload();
native bool:GroundAndreas_IsLoaded();
```

---

### Core Elevation Queries

```pawn
native GetGroundZPos(Float:x, Float:y, &Float:z, bool:interpolate = true, bool:water_surface = true);
```
* **`x`, `y`**: 2D World coordinates (bounded between -3000.0 and +3000.0).
* **`&z`**: Output variable receiving calculated ground altitude.
* **`interpolate`**: `true` for smooth bilinear interpolation; `false` for raw 1m grid data.
* **`water_surface`**: `true` by default to clamp oceans and sea basins to sea level ($Z = 0.0\text{m}$). Pass `false` if you specifically need the submerged seabed floor.

---

### World Line-of-Sight Raycasting

```pawn
native bool:IsWorldBlockingLine(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2, &Float:hit_x = 0.0, &Float:hit_y = 0.0, &Float:hit_z = 0.0);
```
* Traces a 3D line from $(x_1, y_1, z_1)$ to $(x_2, y_2, z_2)$.
* Returns `true` if terrain or buildings block the ray, filling `hit_x, hit_y, hit_z` with the intersection coordinates.
* Returns `false` if the line of sight is completely clear.

---

### Terrain Surface Normal & Slope

```pawn
native GetGroundNormal(Float:x, Float:y, &Float:nx, &Float:ny, &Float:nz, &Float:slope_degrees);
```
* Computes unit normal vector $\vec{N} = (n_x, n_y, n_z)$ perpendicular to the ground.
* Computes slope inclination in degrees ($0.0^\circ$ = flat ground, $45.0^\circ$ = steep hill, $90.0^\circ$ = cliff).

---

### Dynamic Object Alignment

```pawn
native GetGroundObjectRotation(Float:x, Float:y, Float:facing_angle, &Float:rx, &Float:ry);
```
* Calculates Pitch ($RX$) and Roll ($RY$) angles so dynamic objects sit parallel to slopes.
```pawn
new Float:rx, Float:ry;
GetGroundObjectRotation(coneX, coneY, 0.0, rx, ry);
CreateDynamicObject(1238, coneX, coneY, groundHeight, rx, ry, 0.0);
```

---

### Underground Detection Engine

```pawn
native bool:IsPointUnderGround(Float:x, Float:y, Float:z);
native bool:IsPlayerUnderGround(playerid);
native bool:IsVehicleUnderGround(vehicleid);

native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
native Float:GetPlayerGroundDistance(playerid);
native Float:GetVehicleGroundDistance(vehicleid);
```
* **`IsPointUnderGround`**: Returns `true` if the point $(x, y, z)$ is beneath the terrain height.
* **`IsPlayerUnderGround`**: Resolves player coordinates and checks whether the player's feet ($Z - 0.90\text{m}$) are beneath the terrain.
* **`IsVehicleUnderGround`**: Resolves vehicle position and vehicle model ID, fetches the exact model height from the internal database, and checks if the wheels/undercarriage ($Z - \frac{\text{height}}{2}$) are beneath the terrain.
* **`GetPointGroundDistance`**: Returns the vertical distance between $Z$ and ground level ($Z - Z_{\text{ground}}$). Positive means above ground; negative means underground.
* **`GetPlayerGroundDistance`**: Returns the vertical distance between the player's feet and the ground.
* **`GetVehicleGroundDistance`**: Returns the vertical distance between the vehicle wheels and the ground.

---

### Water Detection Subsystem

```pawn
native bool:IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);

native bool:IsPlayerInWater(playerid, weatherid = -1);
native bool:IsPlayerUnderWater(playerid, weatherid = -1);

native bool:IsVehicleInWater(vehicleid, weatherid = -1);
native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);

native GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);
```
* **`InWater` vs `UnderWater`**:
  * **`InWater`**: Returns `true` when the entity or point makes contact with the water surface (e.g. player wading/swimming, vehicle wheels touching water).
  * **`UnderWater`**: Returns `true` only when the entity is completely submerged (e.g. player's head $Z + 0.90\text{m}$ is under the surface; vehicle roof $Z + \frac{\text{height}}{2}$ is below the surface).
* **`weatherid`**: Optional weather ID (-1 by default for calm sea). GTA SA wave amplitude is automatically factored in:
  * Weather 4, 7, 8, 12, 14, 15, 16 $\implies +1.0\text{m}$ wave crest.
  * Weather 19 (Sandstorm) $\implies +1.5\text{m}$ wave crest.
  * Other valid weather IDs $\implies +0.5\text{m}$ standard waves.
* **`GetWaterLevel`**: Returns `1` if water (ocean or inland water body) exists at $(x, y)$ and fills `&water_z` with the surface altitude; returns `0` if dry land.

---

### High-Performance Batch Processing

```pawn
native GetGroundZPosBatch(const Float:points_x[], const Float:points_y[], Float:out_z[], total_points, bool:interpolate = true);
```
* Processes coordinate arrays directly inside C++ without AMX context switching overhead.
* Processes **10,000 coordinates in ~15 milliseconds**.

---

### Dynamic Terrain Modification & Object Stamping

```pawn
native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
native ApplyObjectToGroundAndreas(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, bool:force_replace = false, Float:custom_sizex = 0.0, Float:custom_sizey = 0.0, Float:custom_sizez = 0.0);
```
* **`GroundAndreasRemoveBuilding`**:
  * Automatically calculates the model's physical footprint and reconstructs the underlying terrain slope using perimeter least-squares gradient fitting.
  * Employs lower-quartile terrain filtering so dense clusters of removed industrial structures do not sample neighboring rooftops.
  * **Automatic `RemoveBuildingForPlayer` Hook**: By default, `GroundAndreas.inc` hooks `RemoveBuildingForPlayer` transparently. Any building removed in `OnPlayerConnect` is automatically updated in the heightmap on first appearance with zero runtime overhead.
  * Define `#define GROUNDANDREAS_NO_REMOVE_BUILDING` before `#include <GroundAndreas>` to disable the automatic hook.
* **`ApplyObjectToGroundAndreas`**:
  * Projects the top walkable surface of a mapped object into the heightmap grid in memory.
  * Handles 3D Euler rotations ($RX, RY, RZ$) and computes exact inclined plane equations, seamlessly supporting tilted ramps, elevated roads, bridges, and helipads.
  * Uses a precompiled dimension catalog of popular SA-MP 0.3c, 0.3d, and 0.3.7 mapping objects (concrete slabs, roads, bridges, ramps, runways). Pass `custom_sizex, custom_sizey, custom_sizez` for custom or uncatalogued models.

---

### Advanced Terrain Analytics & Kinematics

#### 1. Safe Spawn & Anti-Void Rescue

```pawn
native bool:IsGroundSafeForSpawn(Float:x, Float:y, Float:max_slope = 15.0);
native bool:GetSafeGroundSpawn(Float:search_x, Float:search_y, &Float:safe_x, &Float:safe_y, &Float:safe_z, Float:max_slope = 15.0, Float:search_radius = 50.0);
```
* **`IsGroundSafeForSpawn`**: Validates whether a position is safe for player or vehicle spawning. Ensures the coordinate is within world boundaries, not submerged in water, and has a terrain slope within `max_slope` degrees (defaults to $15.0^\circ$).
* **`GetSafeGroundSpawn`**: If the target location is steep, submerged, or invalid (e.g. anti-void rescue after falling through the map), executes a high-speed outward concentric search to find the nearest valid, dry, flat spawn position and fills `&safe_x, &safe_y, &safe_z`.

---

#### 2. Aircraft Altimeter & Impact Kinematics

```pawn
native bool:GetGroundImpactTime(Float:x, Float:y, Float:z, Float:vz, &Float:seconds_to_impact, &Float:impact_z, Float:gravity = 9.8);
```
* Solves vertical ballistic kinematics:
  $$z(t) = z_0 + v_z \cdot t - \frac{1}{2} g t^2 = z_{\text{ground}}$$
* Computes real-time `seconds_to_impact` and the ground collision altitude `impact_z`.
* Essential for Ground Proximity Warning Systems (GPWS - "PULL UP! TERRAIN!"), air-drop supply parachutes, and bomb/rocket trajectories.

---

#### 3. Radar Shadow & Terrain Masking

```pawn
native bool:IsPointInRadarShadow(Float:radar_x, Float:radar_y, Float:radar_z, Float:target_x, Float:target_y, Float:target_z);
```
* Performs raycast line-of-sight analysis between a radar/SAM installation and a target aircraft/vehicle.
* Returns `true` if terrain (hills, ridges, mountain peaks) occludes the line of sight, allowing stealth terrain-masking (flying low in valleys to break missile lock or radar tracking).

---

#### 4. Topographic Area Elevation Statistics

```pawn
native bool:GetAreaElevationStats(Float:minx, Float:miny, Float:maxx, Float:maxy, &Float:min_z, &Float:max_z, &Float:avg_z, &Float:max_slope_deg);
```
* Computes analytical terrain statistics across a rectangular zone: minimum altitude, maximum peak, average elevation, and maximum slope inclination.
* Ideal for dynamic drop-zone evaluation, procedural outpost placement, battle royale ring safety analysis, and terrain grading.

---

#### 5. In-Memory Patch Persistence / Disk Baking

```pawn
native bool:SaveGroundAndreasPatches(const filename[]);
native bool:LoadGroundAndreasPatches(const filename[]);
```
* Serializes all runtime modifications made by `GroundAndreasRemoveBuilding` and `ApplyObjectToGroundAndreas` to a compact binary `.dat` file.
* Allows pre-baking all server map changes into disk and instantly reloading them on startup ($<1\text{ms}$) without re-running perimeter regression or object rotation rasterization every time.

---

## Example & Helper Stocks

Practical helper stocks (`GetGroundHeight`, `HasLineOfSight`, `IsPositionUnderground`) and unit benchmark tests can be found directly in [Example.pwn](Example.pwn).

---

## Building from Source

### Windows (Visual Studio 2022)
Run the provided build script:
```cmd
build_msvc.bat
```
Output binaries and archives will be placed in `dist/GroundAndreas-Windows.zip`.

### Linux (Ubuntu / Debian)
```bash
sudo apt-get update && sudo apt-get install -y gcc-multilib g++-multilib cmake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-m32" -DCMAKE_CXX_FLAGS="-m32"
cmake --build build --config Release
```

---

## Version history

<details>
<summary>Show the version history</summary>

### 1.0.0 – 31/10/2026
- Initial standalone production release of **GroundAndreas**:
  - **Memory-Mapped Heightfield Engine**: Zero-latency boot ($< 1\text{ms}$) via `MapViewOfFile` / `mmap` with Copy-On-Write (`PAGE_WRITECOPY` / `MAP_PRIVATE`) demand paging, reducing RAM footprint to ~10–20MB.
  - **Bilinear Elevation Interpolation**: Continuous sub-millimeter terrain queries (`GetGroundZPos`), eliminating legacy 1m discrete staircases.
  - **2.5D World Line-of-Sight Raycasting**: Ultra-fast line tracing (`IsWorldBlockingLine`) with sub-millimeter binary search refinement running in $< 1\mu\text{s}$.
  - **Surface Normal & Slope Analysis**: Real-time perpendicular unit vector $\vec{N}$ and terrain inclination in degrees ($0^\circ - 90^\circ$) via `GetGroundNormal`.
  - **Dynamic Object Alignment**: Automated Pitch ($RX$) and Roll ($RY$) slope orientation (`GetGroundObjectRotation`) for road barriers, tents, and props.
  - **Precision Underground Detection**: Full suite of subterranean collision checks (`IsPointUnderGround`, `IsPlayerUnderGround`, `IsVehicleUnderGround`, `GetPointGroundDistance`, `GetPlayerGroundDistance`, `GetVehicleGroundDistance`) factoring in ped center-of-mass/foot offsets and a compiled database of all 212 GTA SA vehicle model heights.
  - **Dual-Layer Comprehensive Water Dynamics**: Physical differentiation between surface contact (`InWater`) and full submersion (`UnderWater`), covering 16 inland water bodies (reservoirs, casino pools, fountains, lakes, interior pools) and global ocean bathymetry with weather surge factors (`weatherid`).
  - **Dynamic Terrain Modification & Stamping**: Reconstruct carved ground upon building removal (`GroundAndreasRemoveBuilding`) with perimeter least-squares gradient regression and lower-quartile terrain filtering; stamp mapped object walking surfaces (`ApplyObjectToGroundAndreas`) with 3D Euler rotations and precompiled SA-MP 0.3.7 modular object dimensions.
  - **Automated Removal Hook**: Built-in transparent `RemoveBuildingForPlayer` hook in `GroundAndreas.inc` (can be disabled via `#define GROUNDANDREAS_NO_REMOVE_BUILDING`).
  - **Topographic Analytics & Kinematics**:
    - Safe spawn locator & anti-void rescue (`IsGroundSafeForSpawn`, `GetSafeGroundSpawn`).
    - GPWS terrain collision warning & aircraft altimeter kinematics (`GetGroundImpactTime`).
    - Radar occlusion & terrain masking (`IsPointInRadarShadow`).
    - Topographic statistical analysis over rectangular zones (`GetAreaElevationStats`).
    - In-memory patch serialization & disk baking (`SaveGroundAndreasPatches`, `LoadGroundAndreasPatches`).
  - **Dual-Target Architecture**: Compiles as both a traditional SA-MP 0.3.7 plugin (`plugins/GroundAndreas.dll` / `.so`) and a native open.mp component (`components/GroundAndreas.dll` / `.so`).
  - **High-Throughput Batch Processing**: Array query processing directly in C++ (`GetGroundZPosBatch`) achieving 10,000 queries in ~15ms.

</details>

---

## License

This project is licensed under the **Mozilla Public License Version 2.0 (MPL-2.0)** - see the [LICENSE](LICENSE) file for details.
