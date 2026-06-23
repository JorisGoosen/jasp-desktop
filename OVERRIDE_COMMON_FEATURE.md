# OverrideCommon Feature

## Overview

The `OverrideCommon` feature allows you to customize which modules appear as "Common" (visible on the ribbon) and which appear as "Extra" (in the "Add analyses" menu).

## How It Works

When `OverrideCommon` is specified, the system:
1. Combines the Common and Extra module lists from `modules-settings.json`
2. Moves modules listed in `OverrideCommon` to the Common group (in the specified order)
3. Moves all other modules (including originally Common modules NOT in OverrideCommon) to the Extra group

## Configuration Sources

`OverrideCommon` can be specified in two places:

### 1. TOML Configuration File (config.toml)
```toml
OverrideCommon = [
    "jaspDescriptives",
    "jaspTTests",
    "jaspRegression"
]
```

### 2. modules-settings.json
```json
{
    "common": [...],
    "extra": [...],
    "OverrideCommon": ["module1", "module2"]
}
```

Both sources can be used together - modules from both will be included in the Common list.

## Examples

### Example 1: Subset of Original Common Modules
```toml
OverrideCommon = [
    "jaspDescriptives",
    "jaspTTests",
    "jaspRegression"
]
```
**Result:**
- **Common**: jaspDescriptives, jaspTTests, jaspRegression (in that order)
- **Extra**: All other modules (including jaspAnova, jaspFrequencies, jaspFactor, etc.)

### Example 2: Reordering Common Modules
```toml
OverrideCommon = [
    "jaspFrequencies",
    "jaspFactor",
    "jaspDescriptives"
]
```
**Result:**
- **Common**: jaspFrequencies, jaspFactor, jaspDescriptives (custom order)
- **Extra**: All other modules

### Example 3: Combining with EnabledModules
```toml
EnabledModules = [
    "jaspDescriptives",
    "jaspTTests",
    "jaspRegression"
]
OverrideCommon = [
    "jaspDescriptives",
    "jaspRegression"
]
```
**Result:**
- **Common**: jaspDescriptives, jaspRegression (on ribbon, enabled)
- **Extra**: jaspTTests (enabled but not on ribbon), all other modules (disabled)

## Implementation Details

### Modified Files

1. **Desktop/gui/jaspConfiguration/jaspconfiguration.h/cpp**
   - Added `_overrideCommon` member variable
   - Added `getOverrideCommon()` and `setOverrideCommon()` methods
   - Updated `clear()` to reset `_overrideCommon`

2. **Desktop/gui/jaspConfiguration/jaspconfigurationtomlparser.cpp**
   - Added parsing for `OverrideCommon` array from TOML config

3. **Desktop/modules/installedmodules.cpp/h**
   - Modified `getModules()` to read and apply `OverrideCommon` from both config sources
   - Added logic to reorganize modules into Common/Extra based on OverrideCommon

4. **Modules/modules-settings.json**
   - Added `OverrideCommon` field (empty by default)

## Usage

To use this feature:

1. Edit your `config.toml` file (usually in JASP's configuration directory)
2. Add or uncomment the `OverrideCommon` array
3. List the modules you want to be Common, in the order they should appear
4. Restart JASP to see the changes

## Default Behavior

If `OverrideCommon` is not specified (empty array or missing), JASP uses the default behavior:
- Common modules are read from `modules-settings.json` "common" field
- Extra modules are read from `modules-settings.json` "extra" field
