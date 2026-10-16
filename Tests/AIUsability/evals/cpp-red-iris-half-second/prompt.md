---
description: C++ integration. Tests exact subsystem access, StartTransition argument order, duration-to-PlaySpeed conversion, FTransitionParameters/FadeColor override, and the Build.cs dependency.
tags: [transitionfx, cpp]
plugins: ["../.."]
max_turns: 20
allowed_tools: [Read, Glob, Grep, Skill]
---

Write the C++ for a UUserWidget method `void UMyMenu::PlayExit()` that uses the TransitionFX plugin to play the preset held in my `UTransitionPreset* IrisPreset` member (it's DA_Iris, already loaded) as a fade-out that takes exactly 0.5 seconds, with the transition colour overridden to red, without waiting for it to complete. Also tell me what to add to my module's Build.cs. Just the code and one or two sentences.
