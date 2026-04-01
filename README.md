# XIAO ePaper Display Board EE03 ESPHome Driver

Custom ESPHome external component and test configs for the Seeed XIAO ePaper Display Board EE03 using the IT8951 controller.

## Status

This project is currently **not functional** on the tested hardware.

- The custom component builds and flashes successfully.
- OTA updates and normal ESPHome boot work.
- The display still does not return valid IT8951 device info over SPI, so no image is shown yet.

## What Is Included

- `it8951/`
  Custom ESPHome display component under active development.
- `xiao_test_smoke.yaml`
  Minimal smoke-test config for display bring-up.
- `xiao_test_ha.yaml`
  Home Assistant / ESPHome test config with safe secret placeholders.
- `xiao_test.yaml`
  Larger designer-derived example config with safe secret placeholders.
- `secrets.example.yaml`
  Template for local credentials. Copy this to `secrets.yaml` and fill in your own values before compiling.
- `arduino_ee03/`
  Reference Arduino demo shared by Seeed engineers.
- `seeed_gfx/`
  Reference graphics library from Seeed.
- `seeed_lcd/`
  Additional reference library files.
- `XIAO ePaper Display Board EE03_V1.0_SCH & PCB_251217/`
  Hardware design files used for pin and board-path investigation.

## Quick Start

1. Copy `secrets.example.yaml` to `secrets.yaml`.
2. Replace the placeholder values in `secrets.yaml` with your own local credentials.
3. Put the `it8951/` folder next to the YAML you want to test.
4. Start with `xiao_test_smoke.yaml` or `xiao_test_ha.yaml`.
5. Compile and flash with ESPHome.

## Current Known Problem

The main blocker is still SPI communication with the IT8951 on the EE03 board. The current driver already tries:

- hardware reset
- power cycling through `PWR_EN`
- low-speed SPI probing
- `SYS_RUN` wake attempts
- multiple VCOM write variants

Even with those probes, the tested board still reports zeroed device info and does not refresh the panel.
