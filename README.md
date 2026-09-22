# VoidFX

VoidFX is an OBS Studio plugin that adds visual effects to any source through a single filter. Add the **VoidFX** filter to a source, pick an effect from the **Effect** dropdown, and the settings below it switch to match that effect.

## Effects

### Chromatic Aberration

Splits the red and blue channels apart for an old-school VHS or lens look.

| Setting | Description |
|---------|-------------|
| Mode | **Linear (VHS)** shifts channels in one direction. **Radial (Lens)** pushes them outward from a center point, like a cheap lens. |
| Strength | How far apart the channels are, in pixels. |
| Angle | Direction of the split (Linear only). |
| Center X / Y, Falloff | Center of the lens and how quickly the split grows towards the edges (Radial only). |
| Jitter, Jitter Speed | Randomly wobbles the strength over time. |
| Scanlines, Scanline Spacing | Darkened horizontal lines, like a CRT. |
| Noise | Animated grain. |
| Tracking Distortion | A rolling band of horizontal tearing, like a worn tape. |

### Shattered Glass

Cuts the source into shards, then offsets, rotates and shades each one and draws cracks between them.

| Setting | Description |
|---------|-------------|
| Pattern | **Impact** radiates cracks out from an impact point. **Random Shards** breaks the image into evenly sized pieces. |
| Shard Size | Average shard size in pixels (Random Shards only). |
| Radial Cracks, Rings | How many cracks radiate from the impact and how many rings cross them (Impact only). |
| Impact X / Y | Where the glass was hit. Also used by Scatter From Impact. |
| Irregularity, Seed | How uneven the shards are, and which random layout is used. |
| Shatter Amount | Scales displacement and rotation. 0% leaves the glass cracked but intact, so you can animate this to "break" the glass. |
| Displacement, Rotation | How far each shard moves and turns at 100% shatter. |
| Scatter From Impact | 0% moves shards in random directions, 100% throws them away from the impact point. |
| Crack Width, Crack Color | Appearance of the crack lines. The color's alpha controls their opacity. |
| Gap Width | Transparent space between shards. |
| Edge Highlight, Shard Shading | Glints on shard edges and brightness variation between shards. |

## Building

VoidFX is based on the [OBS plugin template](https://github.com/obsproject/obs-plugintemplate) and builds the same way:

```sh
cmake --preset <macos|windows-x64|ubuntu-x86_64>
cmake --build --preset <macos|windows-x64|ubuntu-x86_64>
```

See the [plugin template wiki](https://github.com/obsproject/obs-plugintemplate/wiki) for platform requirements.

## Adding an effect

Each effect is a `struct voidfx_effect_info` (see `src/voidfx-effect.h`) with its own shader in `data/effects/`. To add one:

1. Write the shader in `data/effects/<name>.effect` with a `Draw` technique.
2. Implement the effect in `src/effects/<name>.c`. Prefix every setting name with a short, unique prefix.
3. Declare it in `src/voidfx-effect.h`, add it to the `voidfx_effects` list in `src/voidfx-filter.c`, add the source file to `CMakeLists.txt` and add its strings to `data/locale/en-US.ini`.
