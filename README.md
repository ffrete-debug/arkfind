# ArkFind

A C++ plugin for ARK: Survival Evolved dedicated servers, built on the
[ServerAPI / ArkApi](https://github.com/ServersHub/ServerAPI) plugin framework.

A player types a chat command, gets a numbered list of every creature nearby, picks one, and then
receives live turn-by-turn directions in chat until they are standing on top of it.

```
/finddino wyvern
[ArkFind] Found 3 match(es) for 'wyvern'. Use /findpick <n> to track one:
1) Lightning Wyvern lvl 190 F - 612m
2) Poison Wyvern lvl 145 M - 1.24km
3) Voidwyrm [AE] lvl 180 F - 1.87km
[ArkFind] Type /findpick <n> to start directions, /findstop to cancel.

/findpick 1
[ArkFind] Now tracking Lightning Wyvern. Walk and I will keep calling the turns.
/^ Lightning Wyvern - 612m slightly right (NE), above you [lat 41.2 lon 63.8]
^ Lightning Wyvern - 388m straight ahead (NE), above you [lat 41.2 lon 63.8]
FOUND Lightning Wyvern - it is right here (11m).
```

## Mod creatures work automatically

ArkFind has **no hardcoded species list**. Every name it searches and prints comes from the
creature instance itself:

* `DisplayName` is read from `APrimalDinoCharacter::DescriptiveName` — whatever the game shows the
  player is what you can type.
* `BlueprintPath` is the creature's own blueprint path, used as a stable fallback identity when
  `DescriptiveName` is empty.
* `ClassName` is the trailing class tag of that path (e.g. `Rex_Character_BP`).
* `ModTag` is parsed out of the blueprint path — `/Game/Mods/AE/...` yields `AE`, and vanilla
  creatures yield an empty tag.

A query is scored against all four of those strings, so a modded creature is findable by its
in-game name, its class tag, its blueprint path, or `"<modtag> <name>"`. Install a new creature mod
and it is searchable immediately, with no config edit and no plugin update.

Matching is fuzzy and ranked: exact match beats prefix beats word-prefix beats substring beats
loose subsequence. Results are sorted by match quality first, then by distance — so searching
`rex` gives you the nearest actual **Rex** rather than the nearest `Rex Bone Costume`.

## Install

1. Install ServerAPI (ArkApi) on your dedicated server if you have not already.
2. Copy the `ArkFind` folder into `ShooterGame/Binaries/Win64/ArkApi/Plugins/` so you end up with:

   ```
   ArkApi/Plugins/ArkFind/
       ArkFind.dll
       config.json
   ```
3. Restart the server (or load the plugin through ArkApi).
4. Optionally edit `config.json` and apply it live with `/findcfg reload` — no restart needed.

## Chat commands

| Command | What it does | Example output |
| --- | --- | --- |
| `/finddino [name]` | Scans within `SearchRadiusMeters` and prints a numbered list of matches. With **no** name it lists everything in range (nearest first). | `1) Rex lvl 145 M - 412m` |
| `/findpick <n>` | Starts live tracking of entry `n` from your last `/finddino` list. Accepts `3`, `#3` or `pick 3`. | `[ArkFind] Now tracking Rex.` |
| `/findstop` | Stops tracking and clears your guidance messages. | `[ArkFind] Stopped tracking.` |
| `/findhere` | Prints your own position — map lat/lon plus raw world X/Y/Z. Handy for reporting a spot. | `[ArkFind] You are at lat 41.2 lon 63.8 (x 12500 y -8100 z 320).` |
| `/findcfg reload` | Re-reads `config.json` from disk. Gated by `RequireAdmin`. | `[ArkFind] Configuration reloaded.` |

### Reading the pick list

`FormatHitLine` produces, in order: index, display name, `[ModTag]` when the creature comes from a
mod, `lvl <level>`, `F` or `M`, `(tamed)` when the creature is tamed, then the distance.

```
1) Rex lvl 145 M - 412m
2) Voidwyrm [AE] lvl 190 F - 1.24km
3) Argentavis lvl 60 F (tamed) - 88m
```

Distances under 1 km print as whole metres (`412m`); anything further prints as kilometres with two
decimals (`1.24km`).

### Reading the direction line

`FormatDirection` produces: an arrow oriented to where you are **looking**, the target name, the
distance, a turn hint, the absolute compass direction in parentheses, an optional vertical hint, and
optional map coordinates.

```
/^ Lightning Wyvern - 612m slightly right (NE), above you [lat 41.2 lon 63.8]
```

* Arrows: `^` ahead, `/^` and `^\` off-forward, `>` and `<` beside you, `\v` and `v/` off-behind,
  `v` behind.
* Turn hints: `straight ahead`, `slightly left/right`, `to your left/right`, `hard left/right`,
  `behind you`.
* Vertical hints: `above you` / `below you`, omitted when the height difference is inside
  `VerticalToleranceMeters`.
* Bearings are computed for ARK's axes, where `+X` is east and `+Y` is south (so north is `-Y`).

Once you are inside `ArrivalRadiusMeters` the line becomes:

```
FOUND Lightning Wyvern - it is right here (11m).
```

## Configuration reference

Every key lives at the top level of `config.json` unless noted. Distances are given in **metres**
in config and converted to ARK's internal centimetres by the plugin.

| Key | Default | Meaning |
| --- | --- | --- |
| `SearchRadiusMeters` | `2000` | Maximum 3D distance from the player that a creature can be and still appear in results. `0` means no distance limit, i.e. every loaded creature on the map. Larger radii scan more actors and cost more CPU per search. |
| `MaxResults` | `15` | Hard cap on how many entries the numbered pick list contains, applied after ranking. `0` means no cap. |
| `MatchThreshold` | `0.35` | Minimum name-match score (0.0–1.0) for a creature to count as a hit. The scorer works in fixed bands, so the threshold picks which kinds of match survive: exact `1.0`, prefix `0.85–0.95`, word-prefix `0.70–0.80`, substring `0.55–0.65`, fuzzy subsequence `0.35–0.45`. So `0.5` disables fuzzy matches, `0.7` also disables substring matches. A score of `0.0` never matches, so `0.0` does not mean "list everything". Ignored when `/finddino` is used with no name. |
| `IncludeTamed` | `false` | When `false`, tamed creatures are filtered out so you only see wild ones. Set `true` to also list tames. |
| `MinLevel` | `0` | Skip creatures below this level. `0` disables the lower bound. |
| `MaxLevel` | `0` | Skip creatures above this level. `0` disables the upper bound. |
| `UpdateIntervalSeconds` | `3` | How often the tracker recomputes and sends a fresh direction line while you are tracking. |
| `ArrivalRadiusMeters` | `15` | Distance at which you are considered to have arrived; the direction line switches to the "FOUND" message. |
| `VerticalToleranceMeters` | `8` | Height difference that still counts as level. Beyond it the line appends `above you` or `below you`. |
| `ShowMapCoords` | `true` | Append `[lat … lon …]` map coordinates to direction lines. Set `false` for a terser line or on maps where you have not calibrated `Map.*`. |
| `TrackingTimeoutSeconds` | `600` | Give up on a target after this long, so a forgotten `/findpick` does not spam a player forever. |
| `RequireAdmin` | `false` | When `true`, the commands are restricted to admin-authorised players. |
| `Map.LatOrigin` | `-342900.0` | World **Y** coordinate, in centimetres, that corresponds to latitude 0. |
| `Map.LonOrigin` | `-342900.0` | World **X** coordinate, in centimetres, that corresponds to longitude 0. |
| `Map.LatScale` | `6858.0` | World centimetres per latitude unit: latitude is `(Y - LatOrigin) / LatScale`. |
| `Map.LonScale` | `6858.0` | World centimetres per longitude unit: longitude is `(X - LonOrigin) / LonScale`. |
| `Messages.Prefix` | `"[ArkFind] "` | Prepended to plugin chat replies. |
| `Messages.NoPermission` | see file | Sent when `RequireAdmin` is on and a non-admin runs a command. |
| `Messages.Searching` | see file | Acknowledgement sent when a scan starts. Placeholders: `{query}`, `{radius}`. |
| `Messages.NoResults` | see file | Sent when nothing matched. Placeholders: `{query}`, `{radius}`. |
| `Messages.ResultsHeader` | see file | Header above the numbered list. Placeholders: `{count}`, `{query}`. |
| `Messages.ResultsFooter` | see file | Footer reminding the player how to pick. |
| `Messages.NoSearchYet` | see file | Sent when `/findpick` is used before any `/finddino`. |
| `Messages.InvalidSelection` | see file | Sent when the `/findpick` argument is not a valid index. Placeholders: `{argument}`, `{count}`. |
| `Messages.TrackingStarted` | see file | Confirmation that tracking began. Placeholder: `{name}`. |
| `Messages.TrackingStopped` | see file | Confirmation for `/findstop`. |
| `Messages.NotTracking` | see file | Sent when `/findstop` is used while nothing is being tracked. |
| `Messages.Arrived` | see file | Sent once when the player reaches the target. Placeholder: `{name}`. |
| `Messages.TargetLost` | see file | Sent when the target dies or can no longer be found. Placeholder: `{name}`. |
| `Messages.TrackingTimedOut` | see file | Sent when `TrackingTimeoutSeconds` elapses. Placeholders: `{name}`, `{seconds}`. |
| `Messages.HereLocation` | see file | Reply to `/findhere`. Placeholders: `{lat}`, `{lon}`, `{x}`, `{y}`, `{z}`. |
| `Messages.ConfigReloaded` | see file | Reply to a successful `/findcfg reload`. |
| `Messages.ConfigReloadFailed` | see file | Reply when `config.json` could not be parsed. |

## Per-map GPS values

`Map.LatOrigin`, `Map.LonOrigin`, `Map.LatScale` and `Map.LonScale` are **not universal**. The
shipped defaults (`-342900, -342900, 6858, 6858`) are The Island's values, and they are also close
enough for the other official 8×8 km maps. Larger or offset maps — Ragnarok, Valguero, Crystal
Isles, Lost Island, Genesis, and most custom mod maps — use a different world origin and a different
scale, so the `[lat … lon …]` readout will be wrong until you adjust these four numbers.

The conversion is exactly `lat = (Y - LatOrigin) / LatScale` and `lon = (X - LonOrigin) / LonScale`,
so the scales are **world centimetres per GPS unit**, not the map's extent in metres. The defaults
are consistent with each other: `342900 / 6858 = 50.0`, which puts world origin at the centre of the
map, matching the in-game GPS.

To calibrate a map:

1. Stand somewhere on it and run `/findhere` — it prints both the computed lat/lon and the raw
   world X/Y/Z in centimetres.
2. Compare against the lat/lon on an in-game GPS at the same spot.
3. `LatOrigin` / `LonOrigin` are the world-centimetre coordinates of lat 0 / lon 0. `LatScale` /
   `LonScale` are centimetres per GPS unit: for a map centred on world origin whose GPS runs 0–100,
   that is `-LatOrigin / 50`.
4. Repeat at a second, distant point to confirm both the offset and the scale.

If you would rather not calibrate, set `ShowMapCoords` to `false`. Everything else — distances,
arrows, turn hints and compass directions — is computed from raw world coordinates and stays correct
on every map regardless of these settings.

## Building

**The plugin DLL is Windows-only.** ARK dedicated servers load ArkApi plugins as native Win64 DLLs,
so building `ArkFind.dll` requires:

* Windows
* MSVC toolset **v143** (Visual Studio 2022), x64, C++17
* The ServerAPI / ArkApi SDK headers on the include path

There is no Linux build of the DLL and there cannot be one — the plugin links against the Windows
ARK server binary.

**The pure-logic layer builds and runs anywhere.** `Geo`, `NameMatch`, `Session` and `Text`
deliberately depend on nothing but the standard library and the SDK-free `DinoInfo` struct, so the
unit tests compile with a single g++ invocation on Linux, macOS or Windows:

```sh
g++ -std=c++17 -Wall -Wextra -I src -I tests tests/*.cpp \
    src/Geo.cpp src/NameMatch.cpp src/Session.cpp src/Text.cpp -o arkfind_tests && ./arkfind_tests
```

88 cases, currently all passing. They cover distance and bearing math, the compass/turn/vertical
hints, map-coordinate conversion, name normalisation, mod-tag and class-name parsing, fuzzy scoring,
result ranking and filtering, line formatting, arrival detection, `Messages.*` placeholder
substitution and `/findpick` argument parsing. Only the actor scanning, the timer loop, config
loading and chat plumbing need Windows.

## Limitations

* **Windows servers only.** See Building above.
* **Line-of-sight is not considered.** Directions are straight-line bearings, not pathfinding. If a
  cliff, a lake or the roof of a cave is in the way, the arrow still points through it — expect to
  route around obstacles yourself.
* **Vertical guidance is a hint, not a route.** `above you` / `below you` tells you the target is at
  a different height; it will not tell you where the cave entrance or the ramp is.
* **Snapshot positions.** The target's location is refreshed every `UpdateIntervalSeconds`, so a
  fast-moving flyer will always be slightly ahead of the last reported point.
* **Wild-only by default.** Tamed creatures are hidden unless `IncludeTamed` is `true`.
* **Dead creatures are never listed.** Corpses are filtered out, so this cannot be used to locate a
  kill you left behind.
* **Radius costs CPU.** A very large `SearchRadiusMeters` on a busy server makes each `/finddino`
  more expensive; keep it sane and lean on `MaxResults`.
* **GPS readout needs per-map calibration.** See Per-map GPS values above.
* **No cross-map or global search.** Only creatures loaded on the running server instance, within
  the configured radius of the requesting player, can be found.
* **Matching is one-directional.** Your query has to be contained in the name, in order: `rex`
  matches `Alpha Rex`, but `rex alpha` does not, and neither does `t-rex` (once punctuation is
  stripped it becomes `t rex`, which is longer than `rex` and is not contained in it). Type part of
  the name the game shows you, in the order it shows it.
* **No creature aliases or nicknames.** Matching only ever works against the strings the game itself
  provides — display name, class tag, blueprint path and mod tag. There is no curated synonym table.

## License

MIT — see [LICENSE](LICENSE).
