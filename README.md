# RP2040 FastAccelStepper Demo

PlatformIO sketch for Raspberry Pi Pico (RP2040) showcasing [FastAccelStepper](https://github.com/gin66/FastAccelStepper) with multiple motors. The code tries to attach eight steppers; on the stock Arduino core only the first four succeed, matching the driver limit noted in the [Pico section of the FastAccelStepper README](https://github.com/gin66/FastAccelStepper?tab=readme-ov-file#raspberry-pi-picopico-2-1). Serial logs tell you which motors were provisioned.

- Shared sleep pin `D16`, per-motor STEP/DIR arrays (`D15/D17/D21/D5/...` and `D14/D18/D20/D4/...`)
- Queue tuning via `engine.task_rate(1)` and `setForwardPlanningTimeInMs(60)` keeps the RP2040 PIO FIFOs busy and removes multi-motor jitter
- Non-blocking loop sweeps +1000/−1000 steps at 4 kHz with a 1 s dwell so you can watch motors alternate direction together

## Build & Upload

```bash
pio run -e rpipico2040
pio run -e rpipico2040 -t upload
```

Open the serial monitor at 115200 baud to confirm which steppers were allocated before energising your drivers.
