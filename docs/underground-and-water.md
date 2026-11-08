# Underground & Water Detection

This guide covers the **Underground Detection Engine** and the **Comprehensive Water Dynamics Subsystem** in GroundAndreas. Both systems are engineered with physical entity awareness, avoiding the false positives common in simpler height-check scripts.

---

## 1. Underground Detection Engine

Standard scripts often perform naive underground checks using raw player or vehicle $Z$ coordinates ($Z_{\text{entity}} < Z_{\text{ground}}$). This causes widespread false positives because:
1. **Pedestrian Center-of-Mass:** In GTA San Andreas, a player's $(X, Y, Z)$ pivot resides at hip height, approximately $1.0\text{m}$ above foot level.
2. **Vehicle Dimensional Variance:** Vehicle models vary dramatically in height-from low-profile karts ($0.7\text{m}$) to commercial trucks and aircraft ($4.5\text{m}$). Checking only the vehicle center pivot flags tall vehicles on slopes as "underground" while failing to detect low-riding vehicles clipped beneath roads.

```
       [ Naive Pivot Check ]                     [ GroundAndreas Physical Check ]

             (X, Y, Z)  <- False Under                 (X, Y, Z)
   ============================ (Ground)               +-------+
             +-------+                                 | Vehicle|
             | Vehicle|                                +-------+
             +-------+                       ============================ (Ground)
                                                       (Wheels Contact Check)
```

### The 212 Vehicle Model Dimension Database
GroundAndreas features an internal, precompiled database containing physical bounding dimensions for all **212 GTA San Andreas vehicle models** (IDs 400 through 611).

* **Player Contact Point:** $Z_{\text{feet}} = Z_{\text{player}} - 1.0\text{m}$ (on foot).
* **Vehicle Contact Point:** $Z_{\text{wheels}} = Z_{\text{vehicle}} - \text{offset}$
* **In-Vehicle Player Resolution:** When the player is inside a vehicle, `IsPlayerOnGround`, `IsPlayerUnderGround` and `GetPlayerGroundDistance` evaluate the player's vehicle (same result as the vehicle natives).

#### Vehicle Ground Offset Hierarchy
Vehicle base ground offsets are resolved using a 3-tier hierarchy:
1. **Exceptions measured in-game, only for models the wheel formula does not fit (currently: Kart 571 -> 0.28 m, due to its very small wheels).**
2. **Physical Wheel Positions:** For all wheeled vehicles with collision data in open.mp SDK `vehicle_models.hpp`, the offset is dynamically calculated as $-Z_{\text{wheel\_lowest}} + 0.35\text{m}$ (where 0.35 m is an estimated average tire radius; validated in-game on the Elegy (exact); expected to stay within the 0.35 m on-ground tolerance for typical cars).
3. **Models without wheel data in vehicle_models.hpp: helicopters and airplanes use the preset table; boats, trains, tram and the Vortex hovercraft use 0.90 m (not validated; boats are always reported off-ground while in water).**

#### Calibrating Vehicle Offsets
To calibrate or verify the base ground offset of any vehicle model:
1. Load the demonstration filterscript (`FS_GroundAndreas_Demo`).
2. Enter the target vehicle and park it on flat, level terrain (such as the LS Airport runway or Verdant Meadows airstrip).
3. Check the real-time HUD telemetry: the `GetGroundClearance` reading displayed on the HUD corresponds directly to the physical ground offset of that vehicle model. Only report a model as an exception if its measured value differs from the formula by more than 0.15 m.

---

### Function Signatures

#### Boolean Checks
```pawn
native bool:IsPointUnderGround(Float:x, Float:y, Float:z, Float:tolerance = 0.2);
native bool:IsPlayerUnderGround(playerid, bool:ignore_interiors = true);
native bool:IsVehicleUnderGround(vehicleid, bool:ignore_interiors = true);
```
* **`IsPointUnderGround`:** Checks if an arbitrary 3D point is beneath the interpolated ground level ($Z < Z_{\text{ground}} - \text{tolerance}$).
* **`IsPlayerUnderGround`:** Checks if the player's feet are beneath the terrain floor ($Z_{\text{feet}} = Z - 1.0\text{m}$). Returns `false` if the player is in an interior (> 0) or swimming/diving in water (since the heightmap has no negative seabed elevations). When the player is inside a vehicle, IsPlayerOnGround, IsPlayerUnderGround and GetPlayerGroundDistance evaluate the player's vehicle (same result as the vehicle natives).
* **`IsVehicleUnderGround`:** Checks if the vehicle center pivot is more than 0.5m below the terrain ($Z < Z_{\text{ground}} - 0.5\text{m}$). Returns `false` if the vehicle is in an interior (> 0) or currently in water. Note: Virtual worlds (VW) used outdoors in roleplay servers do not trigger `ignore_interiors`.

#### Direct Float Distance Queries
```pawn
native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
native Float:GetPlayerGroundDistance(playerid, bool:ignore_interiors = true);
native Float:GetVehicleGroundDistance(vehicleid, bool:ignore_interiors = true);
```
* Returns the vertical distance between the entity's ground contact point and the terrain.
* **Positive value:** Entity is above ground (e.g. `+1.50` means 1.5 meters clearance).
* **Negative value:** Entity is submerged beneath terrain (e.g. `-2.10` means 2.1 meters underground).
* When `ignore_interiors = true` and the entity is in an interior (> 0), returns `0.0`.
* When the player is inside a vehicle, IsPlayerOnGround, IsPlayerUnderGround and GetPlayerGroundDistance evaluate the player's vehicle (same result as the vehicle natives).
* **Ocean Water Note:** Because the heightmap stores elevations as unsigned values clamped at sea level ($0.0\text{m}$), negative distance values returned while in the ocean represent depth beneath the sea surface rather than distance to a physical seabed.

#### Solid Ground Contact Verification
```pawn
native bool:IsPlayerOnGround(playerid, Float:max_clearance = 0.35, bool:check_slope = true, bool:allow_water = false, bool:ignore_interiors = true);
native bool:IsVehicleOnGround(vehicleid, Float:max_clearance = 0.35, bool:check_upsidedown = true, bool:allow_water = false, bool:ignore_interiors = true);
```
* **`IsPlayerOnGround`:** Checks if a player is physically standing on or in immediate contact with solid ground.
  * **`max_clearance`:** Maximum vertical clearance between player feet and terrain ($0.35\text{m}$ default).
  * **`check_slope`:** When `true` (default), rejects ground contact if the slope exceeds $60.0^\circ$ (steep cliffs).
  * **`allow_water`:** When `false` (default), swimming or water surface contact does not count as solid ground.
  * **`ignore_interiors`:** When `true` (default), returns `false` if the player is in an interior ($> 0$).
  * When the player is inside a vehicle, IsPlayerOnGround, IsPlayerUnderGround and GetPlayerGroundDistance evaluate the player's vehicle (same result as the vehicle natives).
* **`IsVehicleOnGround`:** Checks if a vehicle's wheels are resting on solid terrain, factoring in individual vehicle model base offsets.
  * **`max_clearance`:** Maximum distance between tire contact level and ground ($0.35\text{m}$ default).
  * **`check_upsidedown`:** When `true` (default), returns `false` if the vehicle is flipped or inverted.
  * **`allow_water`:** When `false` (default), water contact does not qualify as ground contact.
  * **`ignore_interiors`:** When `true` (default), returns `false` if the vehicle is in an interior ($> 0$).

> [!IMPORTANT]
> **2.5D Heightmap Limitations (Bridges, Tunnels & Overhangs)**
> GroundAndreas utilizes a 2.5D grid that records the single highest solid collision node per $(X, Y)$ coordinate. Multi-level highways, under-bridge spans, covered garages, and tunnels (e.g. Los Santos & San Fierro train tunnels) will return the bridge deck or tunnel roof as ground elevation. For anti-cheat and void detection, always preserve `ignore_interiors = true` and verify world context before taking punitive action.

---

### Practical Example: Anti-Void Protection

```pawn
public OnPlayerUpdate(playerid)
{
    // Detect players falling through the world map
    if (IsPlayerUnderGround(playerid))
    {
        new Float:dist = GetPlayerGroundDistance(playerid);

        // Player is more than 3.0 meters beneath the ground surface
        if (dist < -3.0)
        {
            new Float:x, Float:y, Float:z, Float:safeX, Float:safeY, Float:safeZ;
            GetPlayerPos(playerid, x, y, z);

            // Locate the nearest dry, flat terrain spot
            if (GetSafeGroundSpawn(x, y, safeX, safeY, safeZ))
            {
                SetPlayerPos(playerid, safeX, safeY, safeZ + 1.0);
                SendClientMessage(playerid, 0x3498DBFF, "GroundAndreas Anti-Void: Restored position to safe ground.");
            }
        }
    }
    return 1;
}
```

---

## 2. Water Detection Subsystem

Detecting water in GTA San Andreas involves more than checking if $Z < 0.0$:
* **Inland Elevation:** Lakes and reservoirs sit at high elevations (e.g., Sherman Dam reservoir sits at $Z \approx 40.0\text{m}$, Glen Park lake at $Z \approx 11.5\text{m}$).
* **Underground Spaces:** Subway lines, basements, and highway tunnels run directly underneath water bodies. A global height test falsely marks players inside tunnels as swimming.
* **Weather & Ocean Waves:** Storm weather generates dynamic ocean waves up to $0.80\text{m}$ high.
* **Surface Contact vs. Submersion:** Swimming or wading on the surface must be distinguished from full underwater diving or drowned vehicles.

---

### Surface Contact (`InWater`) vs. Submersion (`UnderWater`)

GroundAndreas strictly distinguishes between **contact** and **submersion**:

| Category | Player Condition | Vehicle Condition |
| :--- | :--- | :--- |
| **`InWater`** (Surface Contact) | Player body makes contact with water ($Z_{\text{player}} \le Z_{\text{water}} + 0.35\text{m}$; waist depth $\approx 0.65\text{m}$) and above water floor. | Vehicle wheels touch water surface ($Z_{\text{wheels}} \le Z_{\text{water}}$). |
| **`UnderWater`** (Full Submersion) | Player's head is completely submerged ($Z + 0.90\text{m} \le Z_{\text{water}}$). | Vehicle roof is completely submerged ($Z + \frac{\text{height}}{2} \le Z_{\text{water}}$). |

---

### Function Signatures

```pawn
native bool:IsPointInWater(Float:x, Float:y, Float:z, weatherid = -1);
native bool:IsPointUnderWater(Float:x, Float:y, Float:z, weatherid = -1);

native bool:IsPlayerInWater(playerid, weatherid = -1);
native bool:IsPlayerUnderWater(playerid, weatherid = -1);

native bool:IsVehicleInWater(vehicleid, weatherid = -1);
native bool:IsVehicleUnderWater(vehicleid, weatherid = -1);

native bool:GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);
```

* **`weatherid`:** Pass a specific weather ID ($0 - 45$), or `-1` to automatically query the current server weather (via `GetWeather` on open.mp; on SA-MP 0.3.7, which lacks a weather query native, it gracefully falls back to the calm sea level of $+0.15\text{m}$).
* **`GetWaterLevel`:** Returns `1` if water exists at $(x, y)$ and outputs the surface elevation `&water_z`. Returns `0` if dry land.

---

### Dynamic Ocean Wave Swell

Global ocean surface levels automatically compute wave swell based on GTA San Andreas weather patterns:

| Weather IDs | Environmental Condition | Water Surface Level ($Z$) |
| :---: | :--- | :---: |
| `-1` | Server weather (GetWeather on open.mp; calm sea on SA-MP 0.3.7) | Depends on weather (+0.15m on SA-MP) |
| `0 - 3, 5 - 6, 9 - 11, 13, 17 - 18, ...` | Calm sea default / clear skies | $+0.15\text{m}$ |
| `4, 7, 8, 12, 14, 15, 16` | Rain / storm / fog / overcast | $+0.50\text{m}$ |
| `19` | Weather 19 (sandstorm) | $+0.80\text{m}$ |

> [!NOTE]
> In ocean and coastal zones, any low ground cell with raw elevation $\le \text{wave amplitude}$ (up to $+0.80\text{m}$) is treated as submerged under active swells. Very low docks, piers, or beaches at sea level may evaluate as water during extreme weather.

---

### Built-in 44 Inland Water Bodies Catalog

GroundAndreas contains pre-indexed 3D bounding volumes with specific **Surface $Z$** and **Max Depth** thresholds for all 44 official inland water bodies from GTA San Andreas. If an entity is within the $(X, Y)$ perimeter but below the maximum depth floor (e.g. in a tunnel under the reservoir or beneath an elevated indoor pool), it is correctly identified as **dry**:

1. **Sherman Dam Reservoir (Tierra Robada):** High-altitude basin ($Z_{\text{surface}} = 40.0\text{m}$).
2. **Glen Park Lake (Los Santos):** Urban recreation lake ($Z_{\text{surface}} = 11.5\text{m}$).
3. **Palomino Creek Lagoon (Red County):** Rural fishing pond.
4. **Fisher's Lagoon:** Coastal inlet basin.
5. **San Fierro Bay Docks & Naval Basin:** Shipping canal basins.
6. **Las Venturas The Visage Casino:** Cascading exterior water attraction ($Z_{\text{surface}} = 14.5\text{m}$).
7. **Come-A-Lot Casino Moat:** Medieval castle pool.
8. **Pirates in Men's Pants Lagoon:** Buccaneer show basin.
9. **Madd Dogg's Mansion Swimming Pool:** Mulholland rooftop pool ($Z_{\text{surface}} = 71.0\text{m}$).
10. **Richman Mansion Estate Pools:** Hillside estate pools.
11. **Prickle Pine Residential Swimming Pools:** Las Venturas backyard pools.
12. **Rodeo Hotel & Spa Pools:** Los Santos boutique hotel pools.
13. **Verona Beach Canals:** Urban water channels.
14. **Bayside Marina Basin:** Northern nautical harbor.
15. **Los Santos International Airport Drainage Canals:** Concrete storm canals.
16. **Interior Swimming Pools & Pleasure Domes:** Gyms and interior resort water volumes.

---

### Practical Example: Engine Drowning & Underwater Rescue

```pawn
public OnPlayerUpdate(playerid)
{
    if (IsPlayerInAnyVehicle(playerid))
    {
        new vehicleid = GetPlayerVehicleID(playerid);

        // Vehicle is completely submerged under water
        if (IsVehicleUnderWater(vehicleid))
        {
            // Kill vehicle engine
            new engine, lights, alarm, doors, bonnet, boot, objective;
            GetVehicleParamsEx(vehicleid, engine, lights, alarm, doors, bonnet, boot, objective);
            if (engine == 1)
            {
                SetVehicleParamsEx(vehicleid, 0, lights, alarm, doors, bonnet, boot, objective);
                SendClientMessage(playerid, 0xE74C3CFF, "Your vehicle's engine flooded and stalled underwater!");
            }
        }
    }
    return 1;
}
```
