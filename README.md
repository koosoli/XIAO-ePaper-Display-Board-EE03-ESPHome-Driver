# Seeed 10.3-inch IT8951 ESPHome Drivers

Custom ESPHome external components and test configs for Seeed 10.3-inch IT8951-based ePaper hardware.

## Status


- `it8951/` targets the XIAO ePaper Display Board EE03 pinout. (untested)
- `it8951_reterminal_e1003/` targets the reTerminal E1003 pinout from Seeed Setup 522. (working)


## Quick Start

1. Copy `secrets.example.yaml` to `secrets.yaml`.
2. Replace the placeholder values in `secrets.yaml` with your own local credentials.
3. Put the component folder for your board next to the YAML you want to test.
4. For EE03, start with `xiao_test_smoke.yaml` or `xiao_test_ha.yaml`.
5. For reTerminal E1003, start with `reterminal_e1003_test_smoke.yaml` for minimal bring-up or `reterminal_e1003_test_ha.yaml` for the fuller Home Assistant config with buttons, buzzer, battery, and SHT4x mapping.
6. Compile and flash with ESPHome.

## Board Pin Summary

EE03:
- `TFT_SCLK=7`
- `TFT_MISO=8`
- `TFT_MOSI=9`
- `TFT_CS=44`
- `TFT_BUSY=4`
- `TFT_RST=38`
- `TFT_ENABLE=43`

reTerminal E1003:
- `TFT_SCLK=7`
- `TFT_MISO=8`
- `TFT_MOSI=9`
- `TFT_CS=10`
- `TFT_BUSY=13`
- `TFT_RST=12`
- `TFT_ENABLE=11`
- `ITE_ENABLE=21`

