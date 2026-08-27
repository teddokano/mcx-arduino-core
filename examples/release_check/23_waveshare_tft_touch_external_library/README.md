# 0C: Waveshare_TFT_Touch (external library, real-world SPI-bus-sharing check)

No `.ino` here on purpose -- this isn't a self-contained sketch, it's a
pointer to run an external library's own example against this core.
This library was the real-world project that originally surfaced two
genuine SPI-related bugs in this core (the CS-pin-forced-to-hardware-
PCS bug that corrupted images when the SD card and the LCD shared one
SPI bus, and the per-byte SPI transfer overhead that made SD reads the
dominant cost) -- so re-running its example remains the most direct
regression check for both, closer to a real application than any
loopback-only sketch here can get.

## What to do

1. Install the `Waveshare_TFT_Touch` library (Tedd Okano's repo,
   `github.com/teddokano/Waveshare_TFT_Touch`) into your Arduino
   libraries folder, alongside the LCD + SD card hardware it expects
   (SPI bus shared between the two, separate CS pins).
2. Open and run the library's own `SDBitmapViewer` example against
   FRDM-MCXA153 and/or FRDM-MCXN947.
3. Check:
   - The bitmap renders correctly, with no diagonal streaking/noise
     (that was the CS-forced-to-PCS symptom -- the LCD's CS pin
     getting pulsed by the SD card's own SPI transfers).
   - Drawing finishes in a reasonable time, not dramatically slower
     than the same sketch on another Arduino core (the per-byte
     transfer overhead symptom).

No pass/fail print output to read here -- judge by the displayed
image and by feel for how long it takes to draw.
