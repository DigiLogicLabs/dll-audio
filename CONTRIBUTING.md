# Contributing to DLL Audio

Thanks for helping! Digi Logic Labs maintains this plugin and uses it in our own games, so changes are
reviewed against real production use.

- **Issues:** include engine version, platform, net mode (standalone / listen / dedicated) and a minimal repro.
- **Pull requests:** one change per PR; follow [docs/DLL_PLUGIN_STANDARD.md](docs/DLL_PLUGIN_STANDARD.md);
  add or update automation tests under `DLLAudio.*`; add a CHANGELOG entry under *Unreleased*.
- **Scope:** the core stays engine-only. New engine-plugin or third-party dependencies go in a separate
  optional module.
- **Content:** only content you own and can license under Apache-2.0. No marketplace assets.
- By submitting a contribution you agree it is licensed under the Apache License 2.0 (section 5 of the LICENSE).

## Getting started (fork and pull request)

This plugin is built alongside our own games: `main` is what we ship with, and it should always build. Anyone can
use it, fork it and send changes back.

1. **Fork** [dll-audio](https://github.com/DigiLogicLabs/dll-audio) on GitHub and clone your fork.
2. Put it in an Unreal Engine 5.7 C++ project as `Plugins/dll-audio/` (any project; a blank C++ project is enough),
   enable **DLL Audio** in *Edit → Plugins*, and add `"DLLAudioRuntime"` to your module's `Build.cs` if you call it from C++.
3. Create a branch for one change (`fix/...`, `feature/...`).
4. Build the plugin on its own, which proves it has no hidden dependency on your project:
   `RunUAT.bat BuildPlugin "-Plugin=<path>\DLLAudio.uplugin" "-Package=<short temp path>" -TargetPlatforms=Win64`
   must end with `BUILD SUCCESSFUL`.
5. Run the automation tests: *Tools → Session Frontend → Automation*, filter `DLLAudio.*`, or headless
   `UnrealEditor-Cmd.exe <Project>.uproject -ExecCmds="Automation RunTests DLLAudio; Quit" -unattended -nullrhi`.
6. Update the docs the change touches and add a line under *Unreleased* in [CHANGELOG.md](CHANGELOG.md).
7. Open a pull request against `main` and fill in the template. Small, focused PRs get reviewed fastest.

Please read the [Code of Conduct](CODE_OF_CONDUCT.md). Security problems go through [SECURITY.md](SECURITY.md),
not public issues.
