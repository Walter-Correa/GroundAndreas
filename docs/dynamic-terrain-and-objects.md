# Dynamic Terrain & Objects

This guide details how **GroundAndreas** dynamically modifies terrain at runtime. It covers **automatic terrain slope reconstruction when buildings are removed**, **stamping mapped 3D objects onto the heightmap**, and **baking modifications to disk for instantaneous reloads**.

---

## 1. Dynamic Terrain Reconstruction on Building Removal

When a GTA building is removed via `RemoveBuildingForPlayer`, the original `SAfull.hmap` still contains the building's walls and roof elevation. Any script checking ground height inside the demolished area would falsely return the elevation of the old rooftop.

`GroundAndreasRemoveBuilding` repairs the heightmap in memory by reconstructing the natural ground slope underneath the removed structure.

```
       [ Before: Roof in Heightmap ]                [ After: Reconstructed Slope ]

              +-----------+ (Roof Z)
              |  Removed  |
              |  Building |
    ----------+           +----------           -----------------------------------
    (Ground)                 (Ground)           (Least-Squares Perimeter Slope Fit)
```

### The Reconstruction Algorithm
1. **Perimeter Sampling:** Samples elevation points around the perimeter radius of the removed structure.
2. **Lower-Quartile Filtering:** In dense city areas (e.g., downtown Los Santos), adjacent buildings stand next to the removed one. GroundAndreas applies lower-quartile statistical filtering to discard rooftop samples from neighboring structures.
3. **Least-Squares Plane Regression:** Computes the best-fit 3D incline plane ($z = Ax + By + C$) passing through the filtered perimeter ground coordinates.
4. **Memory-Mapped Rasterization:** Updates the heightmap grid within the structure's footprint using Copy-on-Write memory paging. The original `SAfull.hmap` on disk remains unmodified.

### Signature
```pawn
native GroundAndreasRemoveBuilding(modelid, Float:fX, Float:fY, Float:fZ, Float:fRadius);
```

---

## 2. Automatic `RemoveBuildingForPlayer` Hook

`GroundAndreas.inc` features a built-in ALS hook that automatically intercepts SA-MP's native `RemoveBuildingForPlayer`:

```pawn
// This native call in your gamemode:
RemoveBuildingForPlayer(playerid, 1294, 2253.5, -1742.8, 1014.2, 50.0);

// Automatically triggers GroundAndreasRemoveBuilding on the server,
// updating the heightmap once with internal deduplication caching!
```

### Disabling the Automatic Hook
If your gamemode requires manual control over building removal in GroundAndreas, define `GROUNDANDREAS_NO_REMOVE_BUILDING` prior to including the library:

```pawn
#define GROUNDANDREAS_NO_REMOVE_BUILDING
#include <GroundAndreas>
```

---

## 3. Mapped Object Surface Stamping

Custom maps frequently introduce elevated roads, bridges, helipads, airport runways, and vehicle ramps. GroundAndreas allows you to **stamp dynamic 3D objects directly onto the heightfield** in memory with full rotation support.

### 3D Euler Incline Math
`ApplyObjectToGroundAndreas` calculates 3D coordinate transformations using Euler angles ($RX = \text{Pitch}, RY = \text{Roll}, RZ = \text{Yaw}$). For inclined ramps or tilted bridges, it derives the exact 3D plane equation:

$$Ax + By + Cz + D = 0 \implies z = -\frac{Ax + By + D}{C}$$

Every $(X, Y)$ coordinate falling within the oriented bounding box receives the exact surface elevation of the inclined object.

### Signature
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

* **`modelid`:** GTA object model ID.
* **`x, y, z`:** Center coordinates of the object.
* **`rx, ry, rz`:** Object Euler rotations in degrees.
* **`force_replace`:**
  * `false` (Default): Only stamps the surface if the object is *above* existing terrain (prevents burying natural hills).
  * `true`: Forcefully replaces ground elevation regardless of whether the object sits higher or lower.
* **`custom_sizex, custom_sizey, custom_sizez`:** Optional custom bounding dimensions. If omitted (`0.0`), GroundAndreas automatically looks up the dimensions from its internal SA-MP 0.3.7 mapping object catalog.

---

### Precompiled SA-MP 0.3.7 Object Catalog

GroundAndreas includes dimensions for commonly used SA-MP mapping models:
* **Runways & Slabs:** Models `16200` to `16205` (modular asphalt slabs, concrete roads).
* **Bridges & Piers:** Models `16210` to `16225`.
* **Helipads:** Models `16230` to `16235`.
* **Ramps & Inclines:** Stunt ramps, dirt jumps, wooden planks, and steel catwalks.

#### Stamping a Custom Mapped Ramp Example
```pawn
stock CreateWalkableRamp(modelid, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz)
{
    // Create the visual object in the game world
    new objId = CreateDynamicObject(modelid, x, y, z, rx, ry, rz);

    // Stamp its surface into GroundAndreas so vehicles and peds register the ramp height
    ApplyObjectToGroundAndreas(modelid, x, y, z, rx, ry, rz);
    return objId;
}
```

---

## 4. Patch Persistence & Disk Baking

If your server features extensive mapping (e.g. hundreds of removed buildings and custom road networks), running geometric regressions and plane rasterization repeatedly on every server reboot introduces unnecessary startup overhead.

GroundAndreas allows you to **bake all runtime heightmap patches into a compact binary file** (`.dat`) and load them instantaneously on startup.

### Signatures
```pawn
native bool:SaveGroundAndreasPatches(const filename[]);
native bool:LoadGroundAndreasPatches(const filename[]);
```

### Workflow

#### 1. Development / Baking Server
On your development server or during map setup, execute all removals and object applications once, then save the baked patches to disk:

```pawn
public OnGameModeInit()
{
    // Apply all mapping modifications
    LoadServerCustomMaps();

    // Bake memory modifications to a binary patch file
    SaveGroundAndreasPatches("scriptfiles/server_patches.dat");
    print("[GroundAndreas] Map patches baked successfully to scriptfiles/server_patches.dat");
    return 1;
}
```

#### 2. Production Server
On your production server, load the pre-baked patch file directly. It applies thousands of map modifications in **under 1 millisecond**:

```pawn
public OnGameModeInit()
{
    // Load pre-baked binary patches instantaneously
    if (fexist("scriptfiles/server_patches.dat"))
    {
        LoadGroundAndreasPatches("scriptfiles/server_patches.dat");
        print("[GroundAndreas] Loaded pre-baked terrain patches in < 1ms.");
    }
    return 1;
}
```
