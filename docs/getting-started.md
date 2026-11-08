# Getting Started with GroundAndreas

This guide covers the core architecture of **GroundAndreas**, step-by-step installation for both **open.mp** and **SA-MP 0.3.7**, seamless migration from legacy **MapAndreas**, and running your first high-precision elevation queries.

---

## 1. Architecture: Kernel Memory Mapping

Legacy heightmap plugins allocate a static 72 MB RAM buffer and read the entire dataset sequentially from disk on startup (`fread`). This causes:
* **1.0 to 3.0 second server boot lag**, stalling the main server thread.
* **72 MB of fixed physical memory overhead**, regardless of player count or map area usage.

### The GroundAndreas Advantage
GroundAndreas eliminates both issues using operating system virtual memory mapping:
* **Windows:** `CreateFileMapping` + `MapViewOfFile`
* **Linux:** `open` + `mmap` with `MAP_PRIVATE`

```
+--------------------------------------------------------------------------+
|                        Virtual Address Space (4 GB)                      |
|                                                                          |
|   [ GroundAndreas Header ] [ Sector 0 ] ... [ Sector N ]                 |
|            ^                                     ^                       |
+------------|-------------------------------------|-----------------------+
             |                                     | (Demand Paged)
+------------|------------------+      +-----------v-----------------------+
|  Physical RAM (~10 - 20 MB)   |      |  scriptfiles/SAfull.hmap (72 MB)  |
|  Only active player sectors   |      |  Clean on-disk heightfield data   |
+-------------------------------+      +-----------------------------------+
```

### Core Architectural Benefits
1. **Instantaneous Boot (< 1 ms):** The operating system maps the virtual address pointers immediately. The server boots in 0.001 seconds with zero I/O stutter.
2. **Demand Paging Working Set:** The kernel pages in only the 4 KB sectors requested by active players (e.g., Los Santos). In typical production servers, real physical RAM usage stays between **10 MB and 20 MB**.
3. **Copy-on-Write (COW):** Runtime modifications applied via `ApplyObjectToGroundAndreas` or `GroundAndreasRemoveBuilding` clone only the modified 4 KB memory pages in private RAM. The original `SAfull.hmap` on disk remains completely untouched and pristine.

---

## 2. Installation & Setup

### Step 1: Place Server Binaries

#### For open.mp (Recommended)
Copy the native component into your server's `components/` directory:
* **Windows:** `components/GroundAndreas.dll`
* **Linux:** `components/GroundAndreas.so`

> [!NOTE]
> open.mp automatically discovers and loads components located inside the `components/` folder upon startup. No configuration file edits are required.

#### For SA-MP 0.3.7
Copy the plugin binary into your server's `plugins/` directory:
* **Windows:** `plugins/GroundAndreas.dll`
* **Linux:** `plugins/GroundAndreas.so`

Add `GroundAndreas` to your `server.cfg`:
```ini
# Windows
plugins GroundAndreas
# Linux
plugins GroundAndreas.so
```

> [!NOTE]
> On open.mp, use either the component (recommended, placed in `components/`) or the legacy plugin (placed in `plugins/`), not both.

---

### Step 2: Install Heightmap Data

Copy the heightmap dataset to your server's `scriptfiles/` directory:
```
scriptfiles/SAfull.hmap
```

> [!IMPORTANT]
> The plugin automatically opens `scriptfiles/SAfull.hmap` upon server initialization. If placed elsewhere, you can initialize it manually with `GroundAndreas_Init("path/to/custom.hmap")`.

---

### Step 3: Install the Pawn Include

Copy `GroundAndreas.inc` into your include directory:
* **SA-MP / Pawno:** `pawno/include/GroundAndreas.inc`
* **open.mp / Qawno:** `qawno/include/GroundAndreas.inc`

Include it in your Pawn script:
```pawn
#include <open.mp> // or <a_samp>
#include <GroundAndreas>
```

#### Package Manager (sampctl)
```bash
sampctl package install Walter-Correa/GroundAndreas
```

---

## 3. Drop-In Migration from MapAndreas

GroundAndreas is designed as a direct, modern successor to MapAndreas. It provides superior precision, water-awareness, and speed with straightforward migration.

### Comparison & Migration Matrix

| Legacy MapAndreas | GroundAndreas Modern Standard | Key Differences & Improvements |
| :--- | :--- | :--- |
| `MapAndreas_Init(MAP_ANDREAS_MODE_FULL)` | Handled automatically at boot | Zero boot lag; no manual mode constants required. |
| `MapAndreas_FindZ_For2DCoord(x, y, &z)` | `GetGroundZPos(x, y, z)` | GroundAndreas defaults to smooth bilinear interpolation (1 m grid, 1 cm vertical resolution). |
| Discrete integer grid staircase ($1\text{m}$) | Bilinear interpolation (`interpolate = true`) | Eliminates jagged staircases on hills, mountain roads, and ramps. |
| Unhandled water elevations | Water system + 44 inland bodies | Physical surface and submersion detection across ocean and 44 inland bodies. |
| `MapAndreas_Unload()` | `GroundAndreas_Unload()` | Releases virtual memory mappings and resets state. |
| Heavy AMX loop overhead | `GetGroundZPosBatch(...)` | Processes coordinate arrays directly inside C++ in a single native call (< 1 ms for 10,000 queries). |

> [!TIP]
> GroundAndreas retains internal aliases for `GetGroundZ` and `GetGroundZBatch` inside its C++ dispatch table, ensuring backward compatibility with code expecting legacy names.

---

## 4. Your First Elevation Queries

### Basic Single-Point Query

```pawn
#include <a_samp>
#include <GroundAndreas>

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/groundz", true))
    {
        new Float:x, Float:y, Float:z, Float:groundZ;
        GetPlayerPos(playerid, x, y, z);

        // Query ground altitude with bilinear interpolation and sea-level clamping
        if (GetGroundZPos(x, y, groundZ))
        {
            new msg[128];
            format(msg, sizeof(msg), "Your Z: %.3f | Ground Z: %.3f | Distance to Ground: %.3f meters", z, groundZ, z - groundZ);
            SendClientMessage(playerid, 0x2ECC71FF, msg);
        }
        else
        {
            SendClientMessage(playerid, 0xE74C3CFF, "Error: Coordinates are out of world boundaries (-3000 to +3000).");
        }
        return 1;
    }
    return 0;
}
```

### High-Throughput Batch Processing

When processing large arrays of coordinates (e.g., pathfinding node generation, vehicle convoy routing, or streaming radar grids), invoking a native function per coordinate incurs AMX context-switch overhead.

`GetGroundZPosBatch` processes coordinate arrays directly within native C++ memory:

```pawn
#define BATCH_SIZE 500

new Float:g_NodesX[BATCH_SIZE];
new Float:g_NodesY[BATCH_SIZE];
new Float:g_OutputZ[BATCH_SIZE];

// Populate node coordinates...
// ...

// Process all 500 points in a single native call:
new count = GetGroundZPosBatch(g_NodesX, g_NodesY, g_OutputZ, BATCH_SIZE, true);
printf("GroundAndreas processed %d coordinates in < 1ms.", count);
```

---

## 5. Lifecycle Management

GroundAndreas initializes automatically upon server boot if `scriptfiles/SAfull.hmap` is present. If you need explicit programmatic control:

```pawn
// Check if the engine is currently loaded in memory
if (!GroundAndreas_IsLoaded())
{
    // Initialize manually from a specific file path
    GroundAndreas_Init("scriptfiles/SAfull.hmap");
}

// Unload and release virtual address mapping upon server shutdown
GroundAndreas_Unload();
```
