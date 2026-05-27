"""
Shared configuration and utility functions for the Delirium Prevention project.
"""
# --- Hardware Configuration ---
SERIAL_PORT = 'COM7' # <-- CHECK THIS PORT
BAUD_RATE = 9600

# --- ML Model Configuration ---
WINDOW_SIZE = 20
STEP_SIZE = 10
ACTIVITIES = ['still', 'active']  # Simplified to 2 classes
NUM_CLASSES = len(ACTIVITIES)

# --- Shared Utility Functions ---
def parse_full_packet(line):
    """
    Parses a full data packet line and returns a dictionary.
    Format: "T:25.1,X:2048,Y:2050,Z:2046"

    This parser is robust and will parse all valid key:value
    pairs it finds, even if some are missing.
    """
    data = {}
    try:
        parts = line.strip().split(',')
        for part in parts:
            key_value = part.split(':')
            if len(key_value) == 2:
                data[key_value[0]] = float(key_value[1])
    except Exception as e:
        print(f"Error parsing packet part: {e}")
        pass

    return data