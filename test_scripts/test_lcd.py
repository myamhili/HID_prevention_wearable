"""
Test LCD display commands with Arduino.
This script sends various commands to test the LCD display.
"""
import serial
import time

SERIAL_PORT = 'COM7' #changes
BAUD_RATE = 9600

print("=" * 60)
print("LCD Display Test Script")
print("=" * 60)

try:
    print(f"\n1. Opening serial port {SERIAL_PORT}")
    ser = serial.Serial(
        SERIAL_PORT,
        BAUD_RATE,
        timeout=1,
        write_timeout=2,
        dsrdtr=False,
        rtscts=False
    )
    print("   [OK] Port opened")

    print("\n2. Waiting for Arduino to initialize (3 seconds)")
    time.sleep(3)

    print("\n3. Clearing buffers")
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    print("   [OK] Buffers cleared")

    # Test sequence of commands
    test_commands = [
        ("Clear screen + Red color", "L:Test 1|Line 2\n", "RGB:255,0,0\n"),
        ("Green color", None, "RGB:0,255,0\n"),
        ("Simple text", "L:Hello Arduino!\n", None),
        ("Blue color", None, "RGB:0,0,255\n"),
        ("Activity: Sitting 50%", "L:S:sitting|[=====-----] 50%\n", None),
        ("Activity: Walking 80%", "L:W:walking|[========--] 80%\n", "RGB:0,255,0\n"),
        ("Activity: Full meter", "L:W:walking|[==========] 100%\n", None),
        ("Alert: Move now!", "L:!! MOVE NOW !!|[----------] 0%\n", "RGB:255,0,0\n"),
        ("Sleep mode", "L:Device Sleeping|Temp. Monitor\n", "RGB:0,0,50\n"),
    ]

    for i, test in enumerate(test_commands):
        description, lcd_cmd, rgb_cmd = test
        print(f"\n{i+1}. Testing: {description}")

        if lcd_cmd:
            print(f"   Sending LCD: {repr(lcd_cmd)}")
            try:
                bytes_written = ser.write(lcd_cmd.encode('ascii'))
                ser.flush()
                print(f"   -> Sent {bytes_written} bytes")
                time.sleep(0.3)  # Give Arduino time to process
            except Exception as e:
                print(f"   -> ERROR: {e}")

        if rgb_cmd:
            print(f"   Sending RGB: {repr(rgb_cmd)}")
            try:
                bytes_written = ser.write(rgb_cmd.encode('ascii'))
                ser.flush()
                print(f"   -> Sent {bytes_written} bytes")
                time.sleep(0.3)
            except Exception as e:
                print(f"   -> ERROR: {e}")

        print(f"   Waiting 2 seconds to observe LCD")
        time.sleep(2)

    print("\n\n" + "=" * 60)
    print("Test Complete")
    print("=" * 60)
    print("\nDid you see text on the LCD?")
    print("If LCD only showed blue backlight with no text:")
    print("  1. Check LCD is properly connected via I2C")
    print("  2. Check LCD I2C address (should be 0x3E or 0x62)")
    print("  3. Verify Arduino firmware has RGB LCD library installed")
    print("  4. Try adjusting LCD contrast (potentiometer on LCD)")
    print("  5. Check if lcd.begin() is called in Arduino setup()")

    print("\nClosing serial port...")
    ser.close()
    print("[OK] Port closed")

except serial.SerialException as e:
    print(f"\n[ERROR] Serial Error: {e}")
except Exception as e:
    print(f"\n[ERROR] Unexpected Error: {e}")
    import traceback
    traceback.print_exc()
