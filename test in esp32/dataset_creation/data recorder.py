import serial
import csv
import time

# ==========================================
# CONFIGURATION
# ==========================================
SERIAL_PORT = '/dev/ttyUSB0' 
BAUD_RATE = 115200
OUTPUT_FILE = 'dataset.csv'
NUMBER_OF_INPUTS = 800

# MANUALLY HARDCODE YOUR CLASS HERE
# 0 = Front | 1 = Reverse | 2 = Noise
TARGET_CLASS = int(input("enter the class label"))
# ==========================================

def initialize_csv():
    # Create headers: feature_0 to feature_799, plus the label column
    headers = [f"feature_{i}" for i in range(NUMBER_OF_INPUTS)] + ["label"]
    try:
        with open(OUTPUT_FILE, 'x', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(headers)
    except FileExistsError:
        pass # File already exists, append mode will be used

def main():
    initialize_csv()
    
    try:
        # Opening the serial port forces an automatic reboot on the ESP32
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        print("Port open. Extracting ESP32 boot & calibration sequences...")
        
        # Actively read the buffer for 3 seconds to catch the setup() prints
        time_end = time.time() + 3.0
        while time.time() < time_end:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"ESP32 BOOT LOG: {line}")
                    
    except Exception as e:
        print(f"Failed to connect to {SERIAL_PORT}: {e}")
        return

    print("\n" + "="*50)
    print("BATCH RECORDING MODE ACTIVE")
    print(f"Target Class Hardcoded to: Class {TARGET_CLASS}")
    print("Press the physical ESP32 button to trigger recording.")
    print("Press Ctrl+C in this terminal to stop.")
    print("="*50 + "\n")

    recording_count = 0

    try:
        while True:
            # Read incoming data
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            
            # Skip empty lines
            if not line:
                continue
            
            # Condition 1: Trigger phrase
            if line == "Start speaking":
                print(f"[{recording_count + 1}] >>> START SPEAKING NOW <<<")
            
            # Condition 2: Data Array
            elif "," in line:
                data_points = line.split(",")
                
                if len(data_points) == NUMBER_OF_INPUTS:
                    # Append the hardcoded label
                    data_points.append(str(TARGET_CLASS))
                    
                    # Save to CSV
                    with open(OUTPUT_FILE, 'a', newline='') as f:
                        writer = csv.writer(f)
                        writer.writerow(data_points)
                    
                    recording_count += 1
                    print(f"Success: Saved {NUMBER_OF_INPUTS} samples for Class {TARGET_CLASS}. (Total session count: {recording_count})")
                    print("Waiting for next button press...\n")
                else:
                    print(f"Error: Received {len(data_points)} samples, expected {NUMBER_OF_INPUTS}.")
            
            # Condition 3: Catch-All Diagnostic (Prints anything else the ESP32 sends)
            else:
                print(f"ESP32: {line}")
                
    except KeyboardInterrupt:
        print(f"\nExiting logger. Total recordings saved this session: {recording_count}")

    ser.close()

if __name__ == "__main__":
    main()