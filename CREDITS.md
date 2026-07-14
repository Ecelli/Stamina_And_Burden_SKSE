# Credits

StaminaAndBurden is an original work, but several of its hook points and
developer workflows were informed by existing open-source SKSE mods.  These
attributions acknowledge the value of having reference code to understand
engine call sites and expected function signatures.

## Reference mods (hook points)

| Hook                 | Source mod                                                 | Author            | License            |
|----------------------+------------------------------------------------------------+-------------------+--------------------|
| Attack cost          | https://www.nexusmods.com/skyrimspecialedition/mods/181654 | Styyxus           | GPL-3.0            |
| Bow fire cost + deny | https://www.nexusmods.com/skyrimspecialedition/mods/181654 | Styyxus           | GPL-3.0            |
| Jump cost + height   | https://www.nexusmods.com/skyrimspecialedition/mods/181654 | Styyxus           | GPL-3.0            |
| Staff cast / channel | https://www.nexusmods.com/skyrimspecialedition/mods/181654 | Styyxus           | GPL-3.0            |
| Regeneration rate    | https://www.nexusmods.com/skyrimspecialedition/mods/168926 | Zzyxzz            | None specified     |
| Regen delay bypass   | https://github.com/clayne/StaminaNPC (Fenix Stamina)       | Fenix31415 (copy) | MIT                |
| Attack stamina deny  | https://www.nexusmods.com/skyrimspecialedition/mods/43532  | Magicockerel      | —                  |
| Block stamina cost   | https://www.nexusmods.com/skyrimspecialedition/mods/64741  | dTry              | BSD-3 / Apache-2.0 |
| Block stamina cost   | https://www.nexusmods.com/skyrimspecialedition/mods/91626  | dTry /Borgut1337  | BSD-3 / Apache-2.0 |
| Movement speed       | https://www.nexusmods.com/skyrimspecialedition/mods/151353 | Zzyxzz            | MIT                |
| Sprint drain         | https://www.nexusmods.com/skyrimspecialedition/mods/128208 | Yahim0            | MIT                |
| Damage scaling       | https://www.nexusmods.com/skyrimspecialedition/mods/91626  | Borgut1337        | Apache-2.0         |

## Dependencies (used at build time or runtime)

| Dependency                       | Author       | License | Repository                                         |
|----------------------------------+--------------+---------+----------------------------------------------------|
| CommonLibSSE                     | powerofthree | MIT     | https://github.com/powerof3/CommonLibSSE           |
| Perk Entry Point Extender (PEPE) | NoahBoddie   | MIT     | https://github.com/powerof3/PerkEntryPointExtender |
| Address Library for SKSE         | meh321       | MIT     | https://github.com/meh321/AddressLibraryForSKSE    |

## Tools

| Tool                   | Repository                                              |
|------------------------+---------------------------------------------------------|
| SKSE CMake Modules     | https://github.com/Nightfallstorm/SKSE-CMakeModules     |
| Bethesda CMake Modules | https://github.com/Nightfallstorm/Bethesda-CMakeModules |
| vcpkg                  | https://github.com/microsoft/vcpkg                      |

## Note

None of the reference code was copied verbatim.  Every implementation in this
project is independently written; the references above served only to identify
engine hook locations, understand expected function signatures, and validate
approaches.
