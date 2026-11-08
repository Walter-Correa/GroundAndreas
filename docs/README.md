# GroundAndreas - Documentation Hub

Welcome to the comprehensive technical documentation for **GroundAndreas** (SA-MP & open.mp). This documentation hub provides architectural deep-dives, integration tutorials, mathematical references, and complete API guides covering every subsystem of the high-performance heightmap engine.

---

## Documentation Sections

| Guide | Description |
| :--- | :--- |
| **[Getting Started](getting-started.md)** | Memory-mapped virtual architecture (`SAfull.hmap`), zero-latency setup, and drop-in migration from MapAndreas. |
| **[Elevation & World Raycasting](elevation-and-raycasting.md)** | Continuous bilinear interpolation, high-throughput batch queries, 2.5D line-of-sight raycasting, and slope normal vectors. |
| **[Underground & Water Detection](underground-and-water.md)** | Subterranean collision detection (points, players, and 212 vehicle models) and dual-layer ocean & inland water dynamics. |
| **[Dynamic Terrain & Objects](dynamic-terrain-and-objects.md)** | Terrain reconstruction upon building removal, automated `RemoveBuildingForPlayer` hook, object surface stamping, and disk baking. |
| **[Terrain Analytics & Kinematics](terrain-analytics-and-kinematics.md)** | Safe spawn search, GPWS aircraft impact kinematics under gravity, radar shadow occlusion, and topographic zone statistics. |
| **[Complete API Reference](api-reference.md)** | Exhaustive technical dictionary with all 40 natives, parameter types, default values, return values, and macros. |

---

## Quick Navigation Matrix

Find immediate guidance based on your development goal:

* **I am migrating from legacy MapAndreas:**
  * Read [Drop-In Migration from MapAndreas](getting-started.md#drop-in-migration-from-mapandreas).
  * Check the compatibility notes and see how `GetGroundZPos` replaces legacy stepped queries.
* **I need smooth vehicle wheel physics or mountain foot paths without staircases:**
  * Read [Continuous Bilinear Interpolation](elevation-and-raycasting.md#1-continuous-bilinear-interpolation).
  * Use `GetGroundZPos(x, y, z, true);` (default).
* **I want to align cones, police road barriers, ramps, or tents flush against sloped roads:**
  * Read [Dynamic Object Slope Alignment](elevation-and-raycasting.md#4-dynamic-object-slope-alignment).
  * Use `GetGroundObjectRotation(x, y, facing_angle, rx, ry);`.
* **I want to check if a sniper or anti-cheat line of sight is obstructed by terrain or buildings:**
  * Read [2.5D World Line-of-Sight Raycasting](elevation-and-raycasting.md#3-25d-world-line-of-sight-raycasting).
  * Use `IsWorldBlockingLine(x1, y1, z1, x2, y2, z2, hit_x, hit_y, hit_z);`.
* **I want to detect whether a player or vehicle fell through the world (anti-void/anti-fallthrough):**
  * Read [Underground Detection Subsystem](underground-and-water.md#1-underground-detection-engine).
  * Use `IsPlayerUnderGround(playerid)` or `IsVehicleUnderGround(vehicleid)`.
  * Rescue them smoothly using `GetSafeGroundSpawn(x, y, safe_x, safe_y, safe_z);`.
* **I want to know if an entity is touching water or completely submerged:**
  * Read [Water Detection Subsystem](underground-and-water.md#2-water-detection-subsystem).
  * Use `IsPlayerInWater` / `IsVehicleInWater` (surface contact) vs `IsPlayerUnderWater` / `IsVehicleUnderWater` (submersion).
* **I removed buildings in GTA San Andreas and need the terrain hole to be filled:**
  * Read [Dynamic Terrain Modification on Building Removal](dynamic-terrain-and-objects.md#1-dynamic-terrain-reconstruction-on-building-removal).
  * The automatic hook in `GroundAndreas.inc` handles `RemoveBuildingForPlayer` out of the box.
* **I placed a custom mapping ramp, road, bridge, or helipad and want it walkable by GroundAndreas:**
  * Read [Mapped Object Surface Stamping](dynamic-terrain-and-objects.md#2-mapped-object-surface-stamping).
  * Use `ApplyObjectToGroundAndreas(modelid, x, y, z, rx, ry, rz);`.
* **I am building aircraft instruments (GPWS "PULL UP" warnings or parachute drops):**
  * Read [Ballistic Impact Kinematics](terrain-analytics-and-kinematics.md#2-aircraft-altimeter--impact-kinematics).
  * Use `GetGroundImpactTime(x, y, z, vz, seconds_to_impact, impact_z);`.
* **I want to calculate the average, highest, or lowest terrain elevation in a gangzone or circular radius:**
  * Read [Regional & Radial Ground Elevation](elevation-and-raycasting.md#getgroundzposarea).
  * Use `GetGroundZPosArea(minx, miny, maxx, maxy, avg_z, max_z, min_z);` or `GetGroundZPosRadius(x, y, radius, avg_z, max_z, min_z);`.

---

## Compatibility & System Requirements

* **Game Platforms:** SA-MP (0.3.7-R1 through 0.3.7-R5), open.mp (all builds).
* **Target Architectures:** x86 32-bit (Win32 DLL and Linux i686 Shared Object).
* **Memory Architecture:** Zero-overhead memory mapping (`MapViewOfFile` on Windows, `mmap` on Linux).
  * **Disk Requirement:** 72 MB (`scriptfiles/SAfull.hmap`).
  * **Physical RAM Usage:** Only ~10 to 20 MB working set (kernel demand paging).
* **Pawn Compiler Support:**
  * Community Pawn Compiler (Pawn 3.10.x by Southclaws/Zeex) - *Recommended*.
  * Official Pawn 3.2.3664 (legacy SA-MP default).
* **Dependencies:** **Zero external dependencies**. Does not require ColAndreas, MapAndreas, Streamer, or YSI.

---

> [!NOTE]
> All code snippets and API signatures in these documents are production-ready, fully typed, and verified against GroundAndreas v1.1.0.
