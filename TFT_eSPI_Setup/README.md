# TFT_eSPI User Setup

This folder contains the `User_Setup.h` configuration file for the ESP32-2432S028R (Cheap Yellow Display).

## Installation

1. Locate your TFT_eSPI library folder. Typical paths:
   - **Windows:** `C:\Users\<you>\Documents\Arduino\libraries\TFT_eSPI\`
   - **macOS:** `~/Documents/Arduino/libraries/TFT_eSPI/`
   - **Linux:** `~/Arduino/libraries/TFT_eSPI/`
2. Back up the existing `User_Setup.h` in that folder.
3. Copy the `User_Setup.h` from this folder into the TFT_eSPI library folder, replacing the original.
4. Recompile and upload your sketch.

## Notes

- This setup is configured for the ILI9341 driver with VSPI on the CYD's default pins.
- If you have a different CYD revision or variant, you may need to adjust pin definitions.
- After updating the TFT_eSPI library through Arduino Library Manager, you will need to copy this file again — library updates overwrite `User_Setup.h`.