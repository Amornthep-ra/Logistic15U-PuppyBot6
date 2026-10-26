# wait_SW1 menu with RGB calibration, 0.1.13-dev

The menu order is Start, ADC A0-A15, Mux 8 Front/Back, IMU + Encoder 1/2, Calibrate RGB. Start is selected initially. BOOT release advances; SW1 release selects. Diagnostic pages retain BOOT cycling and SW1 Exit to the main menu. Only selecting Start returns from wait_SW1.

Calibrate opens the existing RGB capture engine inside the blocking wait loop. Its sensor selector contains RGB0, RGB1, Save all, Exit. The color selector contains six colors, Save all, Sensor menu, Exit. Save remains explicit and persists both channels; Exit returns to the main menu and preserves completed unsaved RAM records. Re-entering does not reload old flash over those records. Exit displays an unsaved-RAM notice when appropriate. No calibration thresholds, capture statistics or storage format were changed.

Only the active screen consumes button input and draws. Input state is reset on transitions to prevent one release selecting two pages. Calibration is serviced inside wait_SW1, without relying on loop(). Leaving calibration disables its mode updates and stops its feedback; normal menu feedback continues. Existing standalone calibration defaults retain their original navigation without an orphaned Exit.

## Display

- Five colored menu rows, centered headings and labels, bright selection fill on a dark background.
- Landscape 160x128 and portrait 128x160 are supported. Portrait abbreviates the Mux label to Mux 8 F/B instead of truncating the name.
- Fixed-size GFX font, no wrapping, field-width bounds. Calibration title and storage status occupy separate rows; action/status text is centered within its allotted field.
- Changed values and menu highlights repaint only their fields. Full redraw occurs when switching screens or changing orientation.
- Simulated previews are under .arduino-test/wait-sw1-tests/. They use test values and the actual GFX font, not hardware measurements.

## Validation

- `python tests/wait_sw1/run.py`: PASS. Covers Start-first ordering and wrapping, diagnostic Exit, actual ADC/Mux implementations, legacy 16-channel routing, signed encoder limits without resets, every main-menu selection, every calibration sensor/color action, partial-redraw equivalence and screen bounds in both orientations.
- The same test drives the real blocking wait_SW1 with timed BOOT/SW1 events and simulated core-1 cached RGB frames: capture RGB0, Save, capture RGB1, Exit without Save, re-enter, Save both, Exit, Start. Exactly two explicit flash commits occur. Both 20-frame records survive and decode correctly. No I2C access is made by the menu. Exit also works with absent sensor samples.
- `python tests/calibration/run.py`: PASS capture validity, 6-color flow, storage preservation/failure, buttons, buzzer and rendering regressions.
- `python tests/rgb_mode/run.py`: PASS empty/saved startup, explicit Save, Show isolation and preserving RAM across mode switches.
- `python tests/encoders/run.py`: PASS all eight independent encoder cases.
- Actual user sketch compiled against installed 0.1.13-dev using Arduino CLI and FQBN `logistic15u:rp2040:logistic15u`: 119708 bytes flash, 13352 bytes global RAM. Log: .arduino-test/compile-user-wait-rgb-menu.log.
- All source headers match the installed Arduino library. The IMU, encoder, RGB reader/settings/classifier/capture/storage and puppybot.h are byte-identical to the before-change backup. User setup1/loop1 text is unchanged.
- Visual review: all menu choices are rendered and bounds-checked; landscape main/calibration Exit and portrait main/calibration Exit were inspected. Existing calibration screen regressions were also rendered.

## Limits and unchanged behavior

The board has not been uploaded or physically tested for this change. BOOTSEL briefly pauses the other core; EEPROM commit also uses the Arduino-Pico flash mechanism. No claim of zero IMU timing interruption is made. IMU initialization and offset remain on core 1. ADC/Mux routing is inherited from the existing library rather than inferred from physical wiring. The menu does not command motors/servos or reset encoders. SW1 remains INPUT_PULLUP, and wait_SW1_one/wait_SW1_ALL are unchanged.

Backup: .arduino-test/before-wait-rgb-menu/. Source and installed library were updated locally. No GitHub push, release ZIP rebuild or firmware upload.
