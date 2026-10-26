# RGB and TFT tests

Run `python tests/rgb/run.py` with host g++, Python/Pillow and the installed
Adafruit GFX font. The standard font is read from the external dependency,
not copied into this repository.

The tests execute the real reader and display headers using sensor, Wire,
mutex and TFT doubles. Checks cover channel/bus selection, lazy/idempotent
initialization, dark readings, disconnect/reconnect, invalid channels, legacy
cache compatibility and coherent RGB tuples with concurrent readers.

The TFT recorder/raster checks verify zero draw operations for unchanged data,
updates confined to the affected card, absence of stale digits after a value
shrinks, disconnect transitions, and valid drawing bounds at 160x128/128x160.
Incrementally updated pixels are compared with a freshly rendered reference.
Mode changes rebuild the layout once. Screenshots are saved under
`.arduino-test/rgb-tests/` at native resolution and 4x nearest-neighbor scale.

Clear tests check independent raw values (0 and 65535), reuse of the same sensor
read, a Clear-only dirty field, and removal of stale digits when 12345 becomes 9.

The raster uses the real GFX 5x7 font and approximates rounded corners. It is a
host preview, not evidence of physical TFT wiring, SPI transfer timing, RP2350
mutex behavior or electrical I2C reads. Target compilation and on-board testing
remain distinct checks. No firmware is uploaded by this script.
