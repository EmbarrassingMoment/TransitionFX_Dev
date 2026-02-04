# Performance Optimization Rationale

## Issue
The functions `UTransitionBlueprintLibrary::QuickFadeToBlack` and `QuickFadeFromBlack` utilized a synchronous `LoadObject` call within their internal helper function `QuickFadeInternal`. This function is typically called during gameplay.

`LoadObject` performs a synchronous load from disk if the asset is not in memory, and performs a string hash lookup and map traversal even if it is in memory. Calling this synchronously during gameplay can cause frame hitches (spikes in frame time), especially on the first call when the asset needs to be loaded from disk.

## Optimization
The optimization involves:
1.  **Preloading:** Moving the asset loading to `UTransitionManagerSubsystem::Initialize`. This ensures the assets (`DA_FadeToBlack` and `M_Transition_Master`) are loaded when the GameInstance starts, which is part of the loading phase, not gameplay.
2.  **Caching:** Storing the loaded assets in `UPROPERTY(Transient)` pointers within the Subsystem. This prevents them from being garbage collected.
3.  **Fast Access:** Replacing the `LoadObject` call with a simple pointer access (O(1)) via the Subsystem.

## Measurement
Direct runtime profiling with Unreal Insights is not possible in the current headless environment. However, the performance benefit is theoretically guaranteed by the removal of blocking I/O and hash lookups from the hot path (gameplay) to the cold path (initialization).

### Complexity Analysis
*   **Before:** O(Disk_IO) + O(Hash_Lookup) on first run; O(Hash_Lookup) on subsequent runs.
*   **After:** O(1) Pointer Access on all runs (Disk_IO moved to Initialization).
