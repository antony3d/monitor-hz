# Monitor Hz Change v0.2.7

A lightweight Windows command-line utility to programmatically change monitor refresh rates using modern Windows Display Config API.

## Features
- Detects real monitor friendly names
- **Single-switch mode**: Changes refresh rate instantly and exits
- **Dual-switch mode**: Changes refresh rate, waits for a delay, then switches again on the same monitor
- **Multi-monitor mode**: Switches two monitors to different frequencies simultaneously (no delay)

## Usage
```cmd
Usage: monitor_hz.exe [options]
Options:
  -m <index> [index2]  Monitor index(es) (Default: 1)
  -f <hz1> [hz2]       Refresh rate(s) in Hz (Default: 50 60)
  -d <seconds>          Delay between switches in seconds (Default: 2.0)
```

Run without parameters to see available monitors:
```
--------------------------------------------------
Monitor Index: 0
  System Name:  \\.\DISPLAY1
  GPU Model:    Radeon (TM) RX 470 Graphics
  Monitor Name: SONY TV
  Current Mode: 1920x1080 @ 50 Hz
--------------------------------------------------
Monitor Index: 1
  System Name:  \\.\DISPLAY2
  GPU Model:    Radeon (TM) RX 470 Graphics
  Monitor Name: W2243
  Current Mode: 1920x1080 @ 60 Hz
--------------------------------------------------
```

## Examples

### Single monitor, single frequency

Change monitor at index 0 to 50 Hz and exit:

```
monitor_hz.exe -m 0 -f 50
```

### Single monitor, dual-switch with delay

Switch monitor 1 to 50 Hz, wait 3 seconds, then revert to 60 Hz:

```
monitor_hz.exe -m 1 -f 50 60 -d 3
```

### Two monitors, different frequencies (no delay)

Switch monitor 0 to 50 Hz and monitor 1 to 60 Hz simultaneously:

```
monitor_hz.exe -m 0 1 -f 50 60
```

### Two monitors, same frequency (no delay)

Switch both monitor 0 and monitor 1 to 60 Hz:

```
monitor_hz.exe -m 0 1 -f 60
```

