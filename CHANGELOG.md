# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
## [1.0.2] - 2026-01-25
### Added
- Added Sobel filter
- Added some dev effects, for debugging (not in the release files)

### Changed
- Burn transition: added smoothing effect

### Fixed
- All the built-in transitions are now fully alpha-aware
- Color parameter: Fixed inaccurate color due to sRGB (issue #16)
- Transition: fixed incorrect alpha blending with partially transparent scenes  
- Transition: fixed first frame glitching when a transition auto reloads in dev mode

## [1.0.1] - 2026-01-11
### Fixed
- Video parameter: Fixed an incorrect brightness due to sRGB conversion not done in filters

## [1.0.0] - 2026-01-10
Happy New Year !
This version brings a lot of improvements and rewrites!  
The metadata file structure has been changed for better adaptability of the project.  
The face tracking is now much faster, especially with multiple face tracking effects stacked.  
The whole render pipeline has been fully rewritten for the best performance possible.  
Also: We have MacOS builds now! I don't have a Mac to test it extensively 
but the alpha testers say everything works perfectly.

But the biggest change, which opens many possibilities, is **the `prev_frame` parameter**!
It allows an effect to keep track of what happened before, meaning fun stuff like particle simulation
or recursion effects are now possible!  
Right now, I have only a few releasable effects to show, but many, **many** more are coming soon... 


### Added
- `prev_frame` parameter (#ST-2)
- Mac OS support (#ST-5)
- Multiple effects path support (#ST-37)
- Added `metafileVersion` to the meta file in order to anticipate future changes
- Added json-schema and d.ts files for reference and IDE support
- Added `frame_index` parameter, counting the number of frames since the transition started or the effect is visible (#ST-36)
- `is_studio_mode` parameter, mostly for development purpose (#ST-38)
- Show compile errors in the UI (#ST-39)
- [Filter] New built-in effect "Color invert"
- [Filter] New built-in effect "Ghost"
- [Filter] New built-in effect "Recursion"
- Added a meta.json definition file `meta.d.ts`

### Fixed
- Fixed `obs_get_source_properties("shadertastic_transition")` and `obs_get_source_properties("shadertastic_filter")` crashing OBS
- Fixed "Min Cutoff" step settings being zero
- Reset `prev_time` on visibility toggle
- Updated plugin name for OBS 32
- Fixed "Crashes when reading OBS properties by source ID" (issue #11)
- Alpha compensation (#ST-41)

### Changed
- Replaced CMake build with newest template from OBS 30.2: https://github.com/obsproject/obs-plugintemplate/commit/7c017427c9ee2accc74be525e3cbf2ace5549b6c (#ST-5)
- Migrated `input_type` option as parameter `time` (#ST-34)
- Migrated `input_facedetection` option as parameter `facetracking` (#ST-35)
- Moved export buttons above the about box
- Renamed default filter name to "Shadertastic" instead of "Shadertastic filter"
- Listing effects by label instead of name
- Face tracking: Slightly improved the crop192 filter
- Face tracking: slight performance improvement & do not execute the face mesh if a face is not detected
- Face tracking: Updated ONNX Runtime from `1.17.1` to `1.23.2`
- Performance: shared placeholder textures for all filters and transitions
- [Filter] Gaussian Blur: better alpha management
- [Filter] Rainbow effect: added random mode
- [Filter] Laser eyes: performance improvement by using static data

### Removed
- Face tracking: removed eye points parameters (`fd_leye_1` and similar) (#ST-35)

## [0.1.5] - 2025-06-06
### Fixed
- Transition Settings : copy/paste config showing only in developer mode

## [0.1.4] - 2025-05-28
### Added
- Transition Settings: copy/paste config buttons

## [0.1.3] - 2025-04-30
### Changed
- Settings: Rewritten texts for better clarity

## [0.1.2] - 2025-03-15
### Added
- Vignette in bundled filters
- Zoom to face in bundled filters
- Face tracking: add one-euro filter for smoother points detection
- Added `devmode` meta-attribute for all parameters

### Changed
- Face tracking is now considered stable. The warning message has been removed from the params 
- Face tracking: Massivley improved performance (~2.5x faster)
- Face tracking: Avoid multiple loading of the MediaPipe models
- Face tracking: Added eye points in the prev_texture sent to the shaders, changed 468 → 478 in built-in shaders 
- Face Detection Reference: add "Show prev_texture" option
- Face Detection reference: add toggles for finer checks
- Parameter type `image`: added the possibility to use bundled images in the effect
- Various internal improvements and rewrites
- Rewritten GLSL fract() function to match frac() in HLSL and reversed

### Fixed
- Fixed crash due to incorrect char[] size with face detection filters

## [0.1.0] - 2024-08-23
### Added
- Experimental: Face Tracking
- Rainbow cycle filter in bundled filters
- Face Detection: Reference filter in bundled filters
- Face Detection: Distort filter in bundled filters
- Face Detection: Laser Eyes filter in bundled filters
- Effect selection: added "(Choose an effect)" by default

### Changed
- Set default speed of 0.1 on `input_time` parameters
- Improved Burn transition
- Float parameter: changed the default step_ from 0.1 to 0.01
- Development mode: Reworked the fallback effect to show "ERR", for clarify

### Removed
- Removed template effects from the built-in effects

### Fixed
- Fixed audio level not going to -100 when the audio source is disabled
- Fixed Inkdrop transition on Windows
- Fixed memory leak with .shadertastic files loading
- Development mode: fixed reloading of the effects spamming the logs

## [0.0.8] - 2024-01-15
### Changed
- "Displacement Map" filter : Added a `displace_mode` "Single" or "Displace + Overlay"

### Added
- Added parameter's description (rendered as a tooltip in the UI)
- Added Parameter type `text`

## [0.0.7] - 2023-12-06
### Fixed
- Fixed crash and/or inability to switch the Scene Collection when there is a circular reference of audiolevels

## [0.0.6] - 2023-12-03
### Fixed
- Fixed crash and/or inability to switch the Scene Collection when there is a circular reference of sources
- Fixed parameter source with transparency not rendering correctly

## [0.0.5] - 2023-10-26
### Added
- Added Parameter type `color`
- Added support for `.shadertastic` bundled effects
- Ink drop transition
- Displacement transition
- Displacement Map filter

### Changed
- Dev mode setting, disabling the "Reload" and "Auto Reload" options by default

### Fixed
- Fixed transition image parameter not being reloaded in auto reload mode
- Fixed param type `source` messing up the colors

## [0.0.4] - 2023-09-29
### Fixed
- Fixed built-in transition effects not loading when multiple transitions are created

## [0.0.3] - 2023-08-30
### Added
- Added Parameter type `image`
- Added Parameter type `source`
- Added Parameter type `audiolevel`
- Added "Additionnal effects" folder config, found in Tools > Shadertastic Settings 
- Added "Reset time on toggle" parameter on time-dependant filters
- Filters : "Pixelate" effect
- Filters : "Shake on sound" effect

### Changed
- Using RGBA with 16bit channels for intermediate textures
- Parameters are now ordered as they appear in the meta file
- Performance improvement : shaders now load only once, instead of creating new shaders for each filter

### Fixed
- Fixed crash when no effect is chosen in a transition (issue #2)
- Fixed multistep transitions handling of color in RGBA mode
- Fixed time increased multiple times when a source is duplicated
- Fixed incorrect copyright name in comments 

## [0.0.2] - 2023-06-24
### Added 
- Added the metadata field `input_time` defaulting to `false`. This is a breaking change for time-dependent filters
- Added boolean parameters in effects

### Changed
- Slightly improved performance of the filters rendering
- Gaussian blur and Pixelate transition : changed `breaking_point` 0->100 to `break_point` 0->1
- Gaussian blur filter : default levels to 10 (to avoid lags on small configs)
- Filters : reset rand_seed on reload

### Fixed
- Fixed multi-step_ rendering of filter and transitions when the source is not full-screen
- Rewritten `shadertastic_filter_get_color_space` to match a filter, not a transition
- Fixed transparent prev_texture not created correctly
- Filter reload should update the filter UI without the need to switch filters

### Removed
- Removed dead code used for debugging purposes
