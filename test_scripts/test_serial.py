"""
Simple serial communication test script.
Use this to diagnose Arduino communication issues.
"""
import serial
import time

SERIAL_PORT = 'COM7'
BAUD_RATE = 9600

print("=" * 50)
print("Serial Communication Test")
print("=" * 50)

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
    print("   [OK] Port opened successfully")

    print("\n2. Waiting for Arduino to initialize (3 seconds)")
    time.sleep(3)

    print("\n3. Clearing buffers")
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    print("   [OK] Buffers cleared")

    print("\n4. Reading incoming data from Arduino (10 seconds)")
    start_time = time.time()
    packet_count = 0

    while time.time() - start_time < 10:
        if ser.in_waiting > 0:
            line = ser.readline().decode('ascii', errors='ignore').strip()
            if line:
                packet_count += 1
                print(f"   [{packet_count}] {line}")

    print(f"\n   [OK] Received {packet_count} data packets")

    if packet_count == 0:
        print("\n   [WARNING] No data received from Arduino!")
        print("   Check that:")
        print("   - Arduino firmware is uploaded")
        print("   - Arduino is powered on")
        print("   - Serial Monitor is closed in Arduino IDE")

    print("\n5. Testing write to Arduino...")
    test_commands = [
        "L:Test Message|Line 2\n",
        "RGB:255,0,0\n"
    ]

    for cmd in test_commands:
        print(f"\n   Sending: {cmd.strip()}")
        try:
            bytes_written = ser.write(cmd.encode('ascii'))
            print(f"   -> Wrote {bytes_written} bytes")
            ser.flush()
            print(f"   -> Flushed successfully")
            time.sleep(0.5)
        except serial.SerialTimeoutException:
            print(f"   -> ERROR: Write timeout")
            print("   This means Arduino is not consuming data from serial buffer")
            break
        except Exception as e:
            print(f"   -> ERROR: {e}")
            break

    print("\n6. Closing serial port")
    ser.close()
    print("   [OK] Port closed")

    print("\n" + "=" * 50)
    print("Test completed successfully!")
    print("=" * 50)

except serial.SerialException as e:
    print(f"\n[ERROR] Serial Error: {e}")
    print("\nPossible causes:")
    print("- Wrong COM port (check Device Manager)")
    print("- Arduino IDE Serial Monitor is open")
    print("- Another program is using the port")
    print("- Arduino not connected")

except Exception as e:
    print(f"\n[ERROR] Unexpected Error: {e}")
    import traceback
    traceback.print_exc()
