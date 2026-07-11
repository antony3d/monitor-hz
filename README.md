# Monitor Hz Change v0.2.5

A lightweight Windows command-line utility to programmatically change monitor refresh rates using modern Windows Display Config API.

## Features
- Detects real monitor friendly names
- **Single-switch mode**: Changes refresh rate instantly and exits
- **Dual-switch mode**: Changes refresh rate, waits for a delay, and changes to other.

## Usage
```cmd
Usage: monitor_hz.exe [options]
Options:
  -m <index>   Monitor index (0 for primary, 1 for secondary, etc.)
  -f1 <hz>     First refresh rate frequency in Hz (Default: 50)
  -f2 <hz>     Second refresh rate frequency in Hz (Default: 60)
  -d <seconds> Delay between switches in seconds (Default: 2.0)  
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

##Examples

Change monitor at index 1 to 50 Hz permanently:

```
monitor_hz.exe -m 1 -f1 50
```
Switch to 50 Hz, wait 3 seconds, and revert to 60 Hz:
```
monitor_hz.exe -m 1 -f1 50 -f2 60 -d 3
```