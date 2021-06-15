# Changelog

All notable changes by HENSOLDT Cyber GmbH to this 3rd party module included in
the TRENTOS-M SDK will be documented in this file.

For more details it is recommended to compare the module at hand with the
previous release or the baseline of the 3rd party module.

## [1.3]

### Fixed

- Fix `strncpy` truncating NUL terminating char.
- Fix uninitialized variables in the SPIFFS_vis() function.

## [1.0]

### Added

- Start integration of spiffs based on commit f7d3e9 of
  <https://github.com/pellepl/spiffs>.
