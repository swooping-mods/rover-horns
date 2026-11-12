# Rover Horns: behavior and implementation

Describes the retained payload **0.1.1**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.1.1** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.1.1** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

H triggers a driver horn, with configurable key, WAV and gain. The loader Input SDK exposes rover-horns.honk so Custom Keybinds can override the chord. WAV loading accepts bounded mono PCM16 RIFF (8–96 kHz, at most three seconds and 2 MB), walks at most 32 chunks and reads at most 8,192 samples per Update. Sound Framework follows the current local driver’s car with 3–45 m attenuation and no Doppler.

## State and lifecycle

Configured paths/gain persist in the INI; active clips/emitter state are temporary. Input cooldown is 0.5 seconds and an active voice is not restarted/overlapped. Focus, UI, driver or vehicle changes stop sound and release generation-checked runtime clip resources. Missing/invalid audio reports through native notification.

## Integration and configuration

Runtime dependencies: [sound-framework](https://github.com/swooping-mods/sound-framework).

The [module metadata](../source/mods/rover-horns/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `sound-framework.sound` | 1 | at least 48 bytes |
| Requires `sound-framework.sound-assets` | 1 | at least 32 bytes |

Packaged defaults in [settings.ini](../source/mods/rover-horns/settings.ini); supported bounds and interpretation remain in the linked parser.

| Section | Setting | Default |
|---|---|---|
| `[Horn]` | `Wav` | `..\settings\rover-horns.wav` |
| `[Horn]` | `Key` | `72` |
| `[Horn]` | `VolumePercent` | `65` |

## Implementation sources

- [mod.c](../source/mods/rover-horns/mod.c)
- [wav.h](../source/mods/rover-horns/wav.h)
- [input.h](../source/mods/rover-horns/input.h)
- [base.h](../source/mods/rover-horns/base.h)

## Verification boundary

Requires a supplied compatible WAV; the mod does not generate or download one. No remote horn replication. Actual playback, loudness and device/game-mixer behavior require listening/game checks.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
