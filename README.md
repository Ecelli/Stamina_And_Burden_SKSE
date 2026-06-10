# StaminaAndBurden

SKSE plugin adding stamina-based burden mechanics to Skyrim AE.

## Build

```sh
cmake --preset vs-windows-vcpkg-release
cmake --build --preset Release
```

Debug: `--preset vs-windows-vcpkg-test` + `--build --preset Test`.

## Prerequisites

- `VCPKG_ROOT` env var pointing to vcpkg root.
- `SKYRIM_MODS_FOLDER` env var (optional) — auto-deploys to MO2.

## License

Apache 2.0 — see [LICENSE](LICENSE).
