# STM32 Firmware

This firmware targets the STM32 Nucleo-F401RE and implements the same serial contract as the Arduino firmware.

## Runtime Behavior

- Samples 4 ADC channels with DMA:
  - `PA0`: thermistor
  - `PA1`: accelerometer X
  - `PA4`: accelerometer Y
  - `PB0`: accelerometer Z
- Sends one sensor packet every 100 ms over USART2 at 9600 baud:

```text
T:25.1,X:2048,Y:2050,Z:2046
```

- Accepts backend display commands:

```text
L:line1|line2
RGB:r,g,b
```

- Drives a 16x2 RGB LCD on I2C1:
  - `PB8`: SCL
  - `PB9`: SDA
  - LCD address: `0x3E`
  - RGB controller address: `0x62`

## Build

Open `Delirium-Preventable-Wearable.ioc` in STM32CubeIDE and build the Debug configuration.

If the STM32 GNU toolchain is available on PATH, the checked-in Debug makefile can also be used:

```bash
cd firmware/stm32_firmware/Debug
make
```

The Debug makefile links against `../STM32F401RETX_FLASH.ld`, so it is not tied to a local STM32CubeIDE workspace path.

## Validation Checklist

After flashing the board:

1. Confirm the LCD shows `System Online!` and `Waiting for PC...`.
2. Open the board serial port at 9600 baud and verify packets arrive at about 10 Hz.
3. Send `L:Hello|STM32` and confirm both LCD lines update.
4. Send `RGB:255,0,0`, `RGB:0,255,0`, and `RGB:0,0,255` and confirm the backlight changes.
5. Run the backend with `SERIAL_PORT` set to the STM32 virtual COM port.
6. Verify active mode updates activity data and sleep mode updates temperature/sleep duration.

## Notes

The firmware is backend-compatible and build-ready from the repository, but final production acceptance should be done on the assembled hardware. Temperature calibration and accelerometer orientation can vary with the exact thermistor divider, ADXL335 wiring, supply voltage, and enclosure.
