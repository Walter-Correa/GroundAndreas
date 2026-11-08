# GroundAndreas

[![GitHub release](https://img.shields.io/github/v/release/Walter-Correa/GroundAndreas?include_prereleases&color=blue)](https://github.com/Walter-Correa/GroundAndreas/releases)
[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
[![Platform](https://img.shields.io/badge/platform-SA--MP%20%7C%20open.mp-orange.svg)](https://open.mp)

**GroundAndreas** is an ultra-fast, modern, memory-mapped heightmap engine, 2.5D world line-of-sight raycasting, and high-precision underground/water detection plugin and native component engineered specifically for **San Andreas Multiplayer (SA-MP)** and **open.mp**.

It delivers continuous bilinear interpolation over a 1 m grid with 1 cm vertical resolution, fast 2.5D world raycasting, surface normal vectors, dynamic object slope alignment, physical underground collision analysis (covering pedestrians and all 212 vehicle models), dual-layer ocean and inland water dynamics, dynamic building removal terrain reconstruction, and 3D object slope stamping-all with **instantaneous boot times (< 1ms)** and **near-zero CPU overhead**.

> [!NOTE]
> This project serves as the modern, high-performance successor to legacy solutions like MapAndreas and heavy 3D physics engines like ColAndreas. Rebuilt from the ground up, it introduces operating system memory-mapped virtual address paging (`MapViewOfFile` / `mmap`), continuous bilinear surface interpolation, and analytical topographic kinematics, covering many common ColAndreas raycasting use cases without its 350+ MB memory footprint or startup lag.

---

## Comparison: MapAndreas vs. ColAndreas vs. GroundAndreas (Approximate estimates, not formally benchmarked)

| Feature / Metric | MapAndreas (Legacy) | ColAndreas (Bullet Physics) | GroundAndreas (Modern) |
| :--- | :---: | :---: | :---: |
| **Physical RAM Usage** | ~72 MB (Fixed static buffer) | 350 MB - 500+ MB | **~10 - 20 MB** (Kernel demand paging) |
| **Server Boot Latency** | 1.0 - 3.0s (`fread` disk lag) | 3.0 - 8.0s (3D BVH tree load) | **0.001s (Instantaneous virtual map)** |
| **Z-Query Speed** | ~2.0 ns (Raw buffer) | ~500 - 2,000 ns | **O(1) Raw Memory (~2.0 ns)** / **AMX Native (~50 ns)** |
| **Terrain Precision** | 1.0m discrete staircase steps | 3D mesh vertices | **Continuous Bilinear Interpolation (1 m grid, 1 cm vertical resolution)** |
| **Line-of-Sight Raycast** | ❌ No | Yes (Heavy 3D physics) | **✅ Yes (`IsWorldBlockingLine`, a few µs)** |
| **Surface Normal & Slope** | ❌ No | Complex physics contact | **✅ Yes ($\vec{N}$ & slope in degrees)** |
| **Dynamic Object Alignment** | ❌ No | Possible via raycasts | **✅ Yes (Pitch & Roll calculation)** |
| **Underground Detection** | ❌ No | Possible via raycasts | **✅ Yes (Point, Player, Vehicle)** |
| **Vehicle Dimension Physics** | ❌ No | Heavy mesh bounding | **✅ Yes (Built-in 212 SA Model Heights)** |
| **Comprehensive Water System** | ❌ No (No water volumes) | Material configuration | **✅ Yes (Ocean + 44 Inland Bodies + Weather)** |
| **Dynamic Building Removal** | ❌ No | Manual mesh removal | **✅ Yes (Automatic slope reconstruction)** |
| **3D Object Surface Stamping** | ❌ No | Manual mesh insertion | **✅ Yes (Euler rotated ramps/slabs)** |
| **Batch Array Processing** | ❌ No | ❌ No | **✅ Yes (`GetGroundZPosBatch`)** |
| **open.mp Native Component** | ❌ No | ❌ No | **✅ Yes (Direct `components/` loading)** |

> [!NOTE]
> **2.5D Heightmap Architecture & Technical Notes**
> * **Highest Solid Surface:** The terrain is modeled as a 6000x6000 grid of 16-bit unsigned elevation values (0.01m resolution, range 0.0m to 655.35m). Each cell records the single highest solid collision surface.
> * **Bridges, Tunnels & Overhangs:** Subterranean locations, multi-level highways, under-bridge spans, and covered interiors return the upper roof/deck elevation as ground Z.
> * **Interior Awareness:** Detection natives (`IsPlayerUnderGround`, `IsVehicleUnderGround`, `IsPlayerOnGround`, `IsVehicleOnGround`, `GetPlayerGroundDistance`, `GetVehicleGroundDistance`) include `bool:ignore_interiors = true` by default. When an entity is located in an interior (> 0), heightmap checks are safely bypassed to prevent false subterranean flags. Note: Virtual worlds (VW) used outdoors in roleplay servers do not trigger `ignore_interiors`.
> * **Underwater Floor:** Because heightmap values are unsigned `uint16`, negative sea floor elevations (< 0.0m) are not representable. Water queries utilize the dedicated 3D water polygon database.

---

## Documentation Hub

Comprehensive, modular technical guides and references are available in the [`docs/`](docs/) directory:

| Guide | Description |
| :--- | :--- |
| **[🚀 Getting Started](docs/getting-started.md)** | Step-by-step installation, memory-mapped virtual architecture, and drop-in migration from MapAndreas. |
| **[📐 Elevation & World Raycasting](docs/elevation-and-raycasting.md)** | Continuous bilinear interpolation, batch arrays, 2.5D line-of-sight raycasting, and slope normal vectors. |
| **[🌊 Underground & Water Detection](docs/underground-and-water.md)** | Subterranean collision detection (pedestrians & 212 vehicles) and dual-layer water dynamics. |
| **[🏗️ Dynamic Terrain & Objects](docs/dynamic-terrain-and-objects.md)** | Terrain reconstruction on building removal, automatic hook, object surface stamping, and disk baking. |
| **[📊 Terrain Analytics & Kinematics](docs/terrain-analytics-and-kinematics.md)** | Safe spawn discovery, GPWS aircraft impact kinematics under gravity, radar shadow, and zone statistics. |
| **[📖 Complete API Reference](docs/api-reference.md)** | Exhaustive technical dictionary covering all 40 natives, parameter types, defaults, and macros. |

---

## Quick Start & Installation

### 1. File Placement

#### For open.mp (Component)
1. Copy `components/GroundAndreas.dll` (Windows) or `components/GroundAndreas.so` (Linux) to your server's `components/` directory (open.mp automatically detects and loads components placed in this directory).
2. Copy `scriptfiles/SAfull.hmap` to your server's `scriptfiles/` directory.
3. Copy `GroundAndreas.inc` to your `qawno/include/` or `pawno/include/` directory.

#### For SA-MP 0.3.7 (Plugin)
1. Copy `plugins/GroundAndreas.dll` (Windows) or `plugins/GroundAndreas.so` (Linux) to your server's `plugins/` directory.
2. Copy `scriptfiles/SAfull.hmap` to your server's `scriptfiles/` directory.
3. Copy `GroundAndreas.inc` to your `pawno/include/` directory.
4. Add `GroundAndreas` to `server.cfg`:
   ```ini
   # Windows
   plugins GroundAndreas
   # Linux
   plugins GroundAndreas.so
   ```

> [!NOTE]
> On open.mp, use either the component (recommended, placed in `components/`) or the legacy plugin (placed in `plugins/`), not both.

#### Via sampctl
```bash
sampctl package install Walter-Correa/GroundAndreas
```

#### Interactive Demonstration Filterscript (`FS_GroundAndreas_Demo`)
The release package includes a precompiled, plug-and-play demonstration filterscript (`filterscripts/FS_GroundAndreas_Demo.amx`):
* Comes precompiled with `a_samp` and runs out-of-the-box on both **SA-MP 0.3.7** and **open.mp** without requiring any extra plugins (such as Pawn.CMD).
* To use it, add `FS_GroundAndreas_Demo` to your `filterscripts` line in `server.cfg` (SA-MP) or `pawn.legacy_filterscripts` in `config.json` (open.mp).
* If you wish to recompile from source (`FS_GroundAndreas_Demo.pwn`), it defaults to pure `a_samp`. To compile natively against open.mp, define `GA_DEMO_USE_OPENMP` before including.

---

### 2. Include in Your Script

```pawn
#include <open.mp> // or <a_samp>
#include <GroundAndreas>
```

---

### 3. Basic Example

Query continuous ground elevation, verify line-of-sight clearance, and align an object flush to a slope in just a few lines:

```pawn
#include <a_samp>
#include <GroundAndreas>

// Command to spawn a road barrier flush against any terrain slope
public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/barrier", true))
    {
        new Float:x, Float:y, Float:z, Float:facing;
        GetPlayerPos(playerid, x, y, z);
        GetPlayerFacingAngle(playerid, facing);

        // 1. Calculate ground height with continuous bilinear interpolation
        new Float:groundZ;
        GetGroundZPos(x, y, groundZ);

        // 2. Compute exact Pitch (RX) and Roll (RY) to sit flush on the slope
        new Float:rx, Float:ry;
        GetGroundObjectRotation(x, y, facing, rx, ry);

        // 3. Create object flush with the sloped road
        CreateObject(1427, x, y, groundZ, rx, ry, facing);
        SendClientMessage(playerid, 0x2ECC71FF, "Police barrier deployed flush with terrain slope!");
        return 1;
    }
    return 0;
}

// Anti-cheat verification on weapon shots
// NOTE: Due to 2.5D heightmap architecture, shooters standing beneath bridges, awnings,
// or overpasses will start "beneath" the deck geometry. Log suspicious shots rather than
// blindly rejecting them to avoid false-positive rejections for legitimate players.
public OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, Float:fX, Float:fY, Float:fZ)
{
    if (hittype == BULLET_HIT_TYPE_PLAYER)
    {
        new Float:pX, Float:pY, Float:pZ, Float:tX, Float:tY, Float:tZ;
        GetPlayerCameraPos(playerid, pX, pY, pZ);
        GetPlayerPos(hitid, tX, tY, tZ);

        new Float:hitX, Float:hitY, Float:hitZ;
        // Verify if a mountain or building occludes the bullet line of sight (a few microseconds)
        if (IsWorldBlockingLine(pX, pY, pZ, tX, tY, tZ + 0.5, hitX, hitY, hitZ))
        {
            printf("[Anti-Cheat] Suspicious shot by player %d: terrain obstruction detected at (%.1f, %.1f, %.1f)", playerid, hitX, hitY, hitZ);
            // Log for administrative review (do not outright reject due to 2.5D bridges/overhangs)
        }
    }
    return 1;
}
```

---

## Key Technical Highlights

1. **Kernel Memory-Mapped I/O (`MapViewOfFile` / `mmap`)**
   - The entire 72 MB heightmap (`SAfull.hmap`) is mapped directly into virtual address space.
   - Booting takes **under 1 millisecond** without blocking the server main thread.
   - Operating system demand paging loads only active sectors into RAM, maintaining a working set of **~10 to 20 MB**.
   - Copy-on-Write (COW) ensures runtime modifications never alter the clean on-disk file.

2. **Continuous Bilinear Interpolation**
   - Eliminates the jagged 1-meter staircases of legacy heightmaps.
   - Computes continuous bilinear interpolation over a 1 m grid with 1 cm vertical resolution across hills, mountain passes, and ramps ($Z(u, v) = (1 - u)(1 - v) Z_{00} + u(1 - v) Z_{10} + (1 - u) v Z_{01} + u v Z_{11}$).

3. **2.5D World Line-of-Sight Raycasting (`IsWorldBlockingLine`)**
   - Traces 3D rays across the world with ~2 cm binary search refinement running in **a few microseconds** for typical distances (≈ 0.5 m sampling, cost proportional to length).
   - Covers many common ColAndreas use cases (anti-cheat sniper verification, line of sight, radar occlusion) without the heavy 350+ MB memory footprint.

4. **Surface Normal Vector & Slope Calculation (`GetGroundNormal`)**
   - Computes normalized 3D surface vectors $\vec{N} = (n_x, n_y, n_z)$ and terrain angle in degrees ($0^\circ$ flat to $90^\circ$ cliff).
   - Ideal for vehicle traction physics, fall damage multipliers, and anti-hillclimb sliding logic.

5. **Dynamic Object Alignment & 3D Vector Spanning**
   - Automatically calculates Pitch ($RX$) and Roll ($RY$) angles from surface normals (`GetGroundObjectRotation`) for road barriers, tents, and cones to sit flush on inclined terrain.
   - Spans arbitrary 3D vectors between any two ground coordinates (`GetGroundTwoPointRotation`) with exact Euler pitch and yaw angles to align road barriers, bridge decks, and ramps.
   - Samples contoured multi-node elevations along line segments (`GetGroundLinePoints`) for curbs, guard rails, and fences.

6. **Precision Underground Detection Engine**
   - Detects whether points, players, or vehicles are beneath terrain or inside underground spaces.
   - Factors in pedestrian foot contact offsets ($Z_{\text{feet}} = Z - 1.0\text{m}$) and an internal database with a per-model wheel/base offset table (212 models).
   - Returns float clearances directly: `GetPlayerGroundDistance` and `GetVehicleGroundDistance`.
   - Built-in interior awareness (`ignore_interiors = true`) and automated vehicle interior ALS tracking (`GA_SetVehInterior`) preventing false subterranean alerts in interiors.

7. **Advanced Water Dynamics Subsystem**
   - Strictly distinguishes **surface contact** (`InWater` - swimming, wading, floating) from **full submersion** (`UnderWater` - diving, sunken vehicles).
   - Features dynamic ocean wave swell factoring in GTA SA weather IDs (0.15m calm, 0.50m rain/storm/fog/overcast (4, 7, 8, 12, 14–16), 0.80m weather 19 (sandstorm)).
   - Built-in 3D volumes and floor levels for **44 inland water bodies** (reservoirs, casino pools, fountains, lakes, interior pools) to avoid false positives in tunnels or basements beneath water.

8. **Dynamic Terrain Modification & Object Surface Stamping**
   - Reconstructs natural ground elevation when GTA buildings are removed (`GroundAndreasRemoveBuilding`) using rooftop plateau detection (BFS flood fill + Andrew's Monotone Chain convex hull) and surrounding street lower envelope regression (iterative least-squares plane fitting).
   - Small props (thin poles, street lights, benches, fences) and wildcard model ID -1 are safely ignored; carved area caching prevents duplicate LOD or component removals from degrading neighboring terrain.
   - Transparently hooks `RemoveBuildingForPlayer` in `GroundAndreas.inc` with deduplication caching.
   - Stamps dynamic mapped objects (`ApplyObjectToGroundAndreas`) onto the heightmap with 3D Euler rotations ($RX, RY, RZ$) and inclined plane math (helipads, roads, ramps, bridges).
   - In-memory patch baking to disk (`SaveGroundAndreasPatches`, `LoadGroundAndreasPatches`) for instantaneous startup loading.

---

## Building from Source

### Windows (Visual Studio 2022)
Run the automated build script:
```cmd
build_msvc.bat
```
Output binaries and archives will be placed in `dist/GroundAndreas-Windows.zip`.

### Linux (Ubuntu / Debian i686)
```bash
sudo apt-get update && sudo apt-get install -y gcc-multilib g++-multilib cmake zip
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-m32" -DCMAKE_CXX_FLAGS="-m32"
cmake --build build --config Release
```
Output binaries: `build/GroundAndreas.so` (plugin) and `build/components/GroundAndreas.so` (open.mp component).

---

## Version history

<details>
<summary>Show the version history</summary>

### 1.1.0 - 09/11/2026
> [!WARNING]
> **Breaking Changes & Upgrading Notes from 1.0.0:**
> - `GetPlayerGroundDistance`: Now measures elevation from the player's physical feet ($Z_{\text{feet}} = Z - 1.0\text{m}$) rather than the ped root center-of-mass, resulting in clearance values $\approx 1.0\text{m}$ lower.
> - `IsPlayerUnderWater`: Submergence threshold lowered to ped root $+ 0.90\text{m}$ (physical head height) instead of $+ 1.45\text{m}$.
> - Macro rename: `GROUNDANDREAS_NO_REMOVE_BUILDING` was renamed to `GROUNDANDREAS_NO_REMOVE_BUILD` for pawncc $\le 31$ char symbol compliance.
> - Building removal: `GroundAndreasRemoveBuilding` / `RemoveBuildingForPlayer` replaces the legacy bounding-circle method with Rooftop Plateau Detection (BFS flood fill + Andrew's Monotone Chain convex hull) and surrounding street lower envelope regression (iterative least-squares plane fitting). `fRadius` does not define excavation area; wildcard `modelid = -1` and small street props are ignored.

- **Rooftop Plateau Detection for Building Removal**:
  - Replaces bounding-circle radial ray-marching with topological rooftop plateau detection (BFS flood fill with height delta $\le 1.5\text{m}$, drop threshold $\ge \max(8, 10\%\text{ perimeter})$, and safety abort limits $\le 120\text{m}$ / 20,000 cells).
  - Traces building footprint mask via Andrew's Monotone Chain 2D convex hull with a 2-meter dilation buffer.
  - Reconstructs street grade from surrounding terrain samples (2m to 6m outside mask, sea-level filter, iterative least-squares plane fitting up to 4 passes peeling elevated terraces with slope clamps $\pm 0.30$).
  - Carved area caching (`m_carvedAreas`) prevents subsequent LODs, glass windows, or interior components from altering or degrading adjacent structures.
  - Documented known limitations: (1) connected neighboring structures sharing the same roof height are carved together; (2) removing an isolated LOD also triggers building excavation; (3) buildings surrounded exclusively by raised terraces stay at terrace level; (4) roof steps $> 1.5\text{m}$ outside the convex hull footprint are not lowered.
- **Hook Return Tag Compatibility**:
  - Fixed Pawn warning 213 (tag mismatch) when compiling user scripts under `a_samp` with `LinkVehicleToInterior`, `DestroyVehicle`, and `RemoveBuildingForPlayer` hooks.
- **3D Spatial Vector & Alignment Natives**:
  - `GetGroundTwoPointRotation`: Bridges any two arbitrary $(X, Y)$ ground coordinates with exact 3D pitch and yaw RenderWare Euler angles (`center_z`, `rx`, `ry`, `rz`).
  - `GetGroundLinePoints`: Multi-node elevation and slope path sampler along a line segment for roadside barrier rails, cone chains, fences, and dynamic lighting.
  - `GetGroundZPosArea` & `GetGroundZPosRadius`: Statistical topographic analytics across rectangular gangzones and circular zones (average, minimum, and maximum elevations).
- **Anti-Collision Prefixed Mode (`GROUNDANDREAS_PREFIXED_NATIVES`)**:
  - Optional preprocessor mode providing a clean, conflict-free `GA_...` native namespace (`GA_GetGroundZPos`, `GA_Init`, `GA_GetGroundNormal`, etc.) for complex roleplay gamemodes and third-party script compatibility.
- **Interior-Aware Subterranean Engine & Automatic Vehicle Interior ALS Tracking**:
  - Added `bool:ignore_interiors = true` to all ped and vehicle underground, on-ground and ground-distance queries, eliminating false subterranean flags when inside GTA interiors ($> 0$).
  - When the player is inside a vehicle, IsPlayerOnGround, IsPlayerUnderGround and GetPlayerGroundDistance evaluate the player's vehicle (same result as the vehicle natives).
  - Vehicle base ground offsets are now derived from the wheel positions in open.mp's vehicle model data ($-Z_{\text{wheel}} + 0.35\text{m}$), with in-game measured exceptions only where the formula does not fit (Kart `571`: $0.28\text{m}$) and preset values for vehicles without wheel data.
  - Ped ground queries now correctly measure from the player's feet (accounting for the $\approx 1.0\text{m}$ ped root height); `IsPlayerOnGround` now accurately detects grounded players with the default $0.35\text{m}$ clearance, and `GetPlayerGroundDistance` now measures from the player's feet ($\approx 1.0\text{m}$ lower than before).
  - Underground queries (`IsPlayerUnderGround`, `IsVehicleUnderGround`) no longer flag swimming/diving players or submerged vehicles in the ocean.
  - Full automatic ALS hooks for `LinkVehicleToInterior`, `CreateVehicle`, `AddStaticVehicle`, `AddStaticVehicleEx`, and `DestroyVehicle` (`GA_SetVehInterior`) to track vehicle interiors in SA-MP, with zero-overhead native `veh->getInterior()` resolution in open.mp components.
- **Expanded Water Catalog (44 Inland Bodies) & Weather-Based Dynamic Waves**:
  - Expanded inland water catalog from 16 to 44 fully cataloged 3D water body volumes across San Andreas (fountains, pools, lakes, rivers, reservoirs, and canals).
  - Dynamic 3-tier wave amplitude physics driven by server weather (0.15m calm, 0.50m rain/storm/fog/overcast (4, 7, 8, 12, 14–16), 0.80m weather 19 (sandstorm)) with process-wide server weather caching.
  - `IsPlayerUnderWater` now triggers as soon as the player's head is submerged (submergence threshold lowered from ped root + 1.45m to ped root + 0.90m, matching physical ped head level).
- **Heightmap Patch Persistence & Security Hardening**:
  - Replaced bit-by-bit patch serialization with ultra-fast 64-bit word population counting (`std::bitset<64>`).
  - Unified dirty-cell bitset initialization (`HeightMap::OnLoaded`) across all load paths, guaranteeing reliable patch saving.
  - Strict path traversal security validation (`IsPathSafe`) preventing directory escape attacks (`../`).
  - Strict file-size validation for the dynamic object database (`objects.dat`).
  - Clamped bounding-box sweeping in `RemoveBuilding` and `RasterizeObject` to prevent infinite loops on coordinates near or outside map boundaries.
- **Symbol Export Resilience**:
  - Added `__GroundAndreas_NativeRefs()` public callback in `GroundAndreas.inc` to prevent aggressive compiler symbol stripping of server AMX exports.
- **Interactive Demonstration Suite (`FS_GroundAndreas_Demo.pwn`)**:
  - Completely revamped plug-and-play in-game demonstration filterscript organized into 6 thematic hubs with real-time text telemetry HUD and live performance benchmarks.
- **Compiler & CI Hardening**:
  - Enforced static C++ runtime linkage (`/MT`) on Windows importing only `KERNEL32.dll`.
  - Added Linux CI verification for minimal GLIBC baseline and zero dynamic `libstdc++` dependencies.
  - Automated version consistency checks across `CMakeLists.txt`, `pawn.json`, and `GroundAndreas.inc`.
  - All public symbols and hooks trimmed to $\le 31$ characters for full pawncc 3.2.3664 compatibility (`GA_SetVehInterior`, `GA_NO_REMOVE_BUILDING`); `GROUNDANDREAS_NO_REMOVE_BUILDING` was renamed to `GROUNDANDREAS_NO_REMOVE_BUILD` (the legacy 32-character name is no longer recognized).

### 1.0.0 - 31/10/2026
- Initial standalone production release of **GroundAndreas**:
  - **Memory-Mapped Heightfield Engine**: Zero-latency boot ($< 1\text{ms}$) via `MapViewOfFile` / `mmap` with Copy-On-Write (`PAGE_WRITECOPY` / `MAP_PRIVATE`) demand paging, reducing RAM footprint to ~10-20MB (`GroundAndreas_Init`, `GroundAndreas_Unload`, `GroundAndreas_IsLoaded`).
  - **Core Elevation Queries**: Continuous terrain elevation queries (`GetGroundZPos`) and high-throughput C++ batch processing (`GetGroundZPosBatch`).
  - **2.5D World Line-of-Sight Raycasting**: Fast line tracing (`IsWorldBlockingLine`) with binary search refinement.
  - **Surface Normal & Slope Analysis**: Real-time perpendicular unit vector $\vec{N}$ and terrain inclination in degrees ($0^\circ - 90^\circ$) via `GetGroundNormal`.
  - **Dynamic Object Alignment**: Automated Pitch ($RX$) and Roll ($RY$) slope orientation (`GetGroundObjectRotation`) for road barriers, tents, and props.
  - **Vertical Clearance & Submerged Floor**: Ground distance and water column depth detection (`GetGroundClearance`, `IsGroundUnderwater`).
  - **Precision Underground Detection**: Full suite of subterranean collision checks (`IsPointUnderGround`, `IsPlayerUnderGround`, `IsVehicleUnderGround`, `GetPointGroundDistance`, `GetPlayerGroundDistance`, `GetVehicleGroundDistance`) factoring in ped center-of-mass/foot offsets and a compiled database of all 212 GTA SA vehicle model heights.
  - **Dual-Layer Comprehensive Water Dynamics**: Physical differentiation between surface contact (`InWater`) and full submersion (`UnderWater`), covering 16 inland water bodies and ocean surface water levels with weather surge factors (`weatherid`, `GetWaterLevel`).
  - **Dynamic Terrain Modification & Stamping**: Reconstruct carved ground upon building removal (`GroundAndreasRemoveBuilding`) with perimeter least-squares gradient regression; stamp mapped object walking surfaces (`ApplyObjectToGroundAndreas`) with 3D Euler rotations and precompiled SA-MP 0.3.7 modular object dimensions; in-memory patch serialization & disk baking (`SaveGroundAndreasPatches`, `LoadGroundAndreasPatches`).
  - **Automated Removal Hook**: Built-in transparent `RemoveBuildingForPlayer` hook in `GroundAndreas.inc` (can be disabled via `#define GROUNDANDREAS_NO_REMOVE_BUILDING`).
  - **Topographic Analytics & Kinematics**:
    - Safe spawn locator & anti-void rescue (`IsGroundSafeForSpawn`, `GetSafeGroundSpawn`).
    - GPWS terrain collision warning & aircraft altimeter kinematics (`GetGroundImpactTime`).
    - Radar occlusion & terrain masking (`IsPointInRadarShadow`).
    - Topographic statistical analysis over rectangular zones (`GetAreaElevationStats`).
  - **Dual-Target Architecture**: Compiles as both a traditional SA-MP 0.3.7 plugin (`plugins/GroundAndreas.dll` / `.so`) and a native open.mp component (`components/GroundAndreas.dll` / `.so`).

</details>

---

## Author & Credits

* [@Walter-Correa](https://github.com/Walter-Correa) - Concept and development of **GroundAndreas**, with the assistance of **Artificial Intelligence**.
* **MapAndreas Team** - Original creators of MapAndreas and the underlying 6000x6000 heightmap dataset (`SAfull.hmap`).

---

## License

This project is licensed under the **Mozilla Public License Version 2.0 (MPL-2.0)** - see the [LICENSE](LICENSE) file for details.
