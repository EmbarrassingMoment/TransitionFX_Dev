# Contributing to TransitionFX

Thank you for your interest in contributing to TransitionFX!

## Reporting Issues

- Use the [GitHub Issues](https://github.com/EmbarrassingMoment/TransitionFX/issues) page
- Include your Unreal Engine version, OS, and steps to reproduce
- Attach screenshots or videos when possible

## Pull Requests

1. Fork the repository and create a feature branch from `main`
2. Follow the existing code style (Unreal Engine coding standards)
3. Keep changes focused — one feature or fix per PR
4. Test your changes in the Unreal Editor before submitting
5. Update relevant documentation if your change affects the public API

## Code Style

- Follow the [Unreal Engine Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- Use `UE_LOG(LogTransitionFX, ...)` for logging
- Prefix boolean variables with `b` (e.g., `bIsActive`)
- Add Doxygen-style comments (`/** ... */`) for public functions

## Adding New Transition Effects

1. Create a new Material in `Content/Materials/` using the master material as a parent
2. Create a Material Instance in `Content/Materials/Instances/`
3. Create a Data Asset (`UTransitionPreset`) in `Content/Data/`
4. Set `EffectClass` to `UPostProcessTransitionEffect` (or your custom subclass)
5. Test with both Forward and Reverse modes

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
