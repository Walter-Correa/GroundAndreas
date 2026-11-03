# Underground & Water Detection

This guide covers the **Underground Detection Engine** and the **Comprehensive Water Dynamics Subsystem** in GroundAndreas. Both systems are engineered with physical entity awareness, avoiding the false positives common in simpler height-check scripts.

---

## 1. Underground Detection Engine

Standard scripts often perform naive underground checks using raw player or vehicle $Z$ coordinates ($Z_{\text{entity}} < Z_{\text{ground}}$). This causes widespread false positives because:
1. **Pedestrian Center-of-Mass:** In GTA San Andreas, a player's $(X, Y, Z)$ pivot resides at hip height, approximately $0.90\text{m}$ above foot level.
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

* **Player Contact Point:** $Z_{\text{feet}} = Z_{\text{player}} - 0.90\text{m}$
* **Vehicle Contact Point:** $Z_{\text{wheels}} = Z_{\text{vehicle}} - \frac{\text{model\_height}}{2}$

---

### Function Signatures

#### Boolean Checks
```pawn
native bool:IsPointUnderGround(Float:x, Float:y, Float:z);
native bool:IsPlayerUnderGround(playerid);
native bool:IsVehicleUnderGround(vehicleid);
```
* **`IsPointUnderGround`:** Checks if an arbitrary 3D point is beneath the interpolated ground level ($Z < Z_{\text{ground}} - 0.05\text{m}$).
* **`IsPlayerUnderGround`:** Checks if the player's feet are beneath the terrain ($Z_{\text{feet}} < Z_{\text{ground}} - 0.30\text{m}$).
* **`IsVehicleUnderGround`:** Looks up the vehicle model ID, computes the lower wheel clearance, and checks if the wheels are beneath the terrain ($Z_{\text{wheels}} < Z_{\text{ground}} - 0.40\text{m}$).

#### Direct Float Distance Queries
```pawn
native Float:GetPointGroundDistance(Float:x, Float:y, Float:z);
native Float:GetPlayerGroundDistance(playerid);
native Float:GetVehicleGroundDistance(vehicleid);
```
* Returns the vertical distance between the entity's ground contact point and the terrain.
* **Positive value:** Entity is above ground (e.g. `+1.50` means 1.5 meters clearance).
* **Negative value:** Entity is submerged beneath terrain (e.g. `-2.10` means 2.1 meters underground).

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
* **Weather & Ocean Waves:** Storm weather generates dynamic ocean waves up to $1.5\text{m}$ high.
* **Surface Contact vs. Submersion:** Swimming or wading on the surface must be distinguished from full underwater diving or drowned vehicles.

---

### Surface Contact (`InWater`) vs. Submersion (`UnderWater`)

GroundAndreas strictly distinguishes between **contact** and **submersion**:

| Category | Player Condition | Vehicle Condition |
| :--- | :--- | :--- |
| **`InWater`** (Surface Contact) | Player's feet touch water surface ($Z_{\text{feet}} \le Z_{\text{water}}$). | Vehicle wheels touch water surface ($Z_{\text{wheels}} \le Z_{\text{water}}$). |
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

native GetWaterLevel(Float:x, Float:y, &Float:water_z, weatherid = -1);
```

* **`weatherid`:** Pass the current server weather ID, or `-1` for calm sea default.
* **`GetWaterLevel`:** Returns `1` if water exists at $(x, y)$ and outputs the surface elevation `&water_z`. Returns `0` if dry land.

---

### Dynamic Ocean Wave Swell

Global ocean bathymetry automatically computes wave swell based on GTA San Andreas weather patterns:

| Weather IDs | Environmental Condition | Water Surface Level ($Z$) |
| :---: | :--- | :---: |
| `-1` | Calm Sea Default (No weather modifier) | $0.0\text{m}$ |
| `0, 1, 2, 3, 5, 6, 9, 10, 11, 13, 17, 18, 20+` | Standard / Mild Weather | $+0.5\text{m}$ |
| `4, 7, 8, 12, 14, 15, 16` | Stormy / Foggy / Rain / Overcast | $+1.0\text{m}$ |
| `19` | Sandstorm / High Surge Waves | $+1.5\text{m}$ |

---

### Built-in 16 Inland Water Bodies Catalog

GroundAndreas contains pre-indexed 3D bounding volumes with specific **Surface $Z$** and **Floor $Z$** thresholds for all major inland water bodies. If an entity is within the $(X, Y)$ perimeter but below the Floor $Z$ (e.g. in a tunnel under the reservoir), it is correctly identified as **dry**:

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
