import serial
import csv
import time

# ==========================================
# CONFIGURATION
# ==========================================
SERIAL_PORT = '/dev/ttyUSB0' # Update if using Windows (e.g., 'COM3')
BAUD_RATE = 115200
OUTPUT_FILE = 'dataset.csv'
NUMBER_OF_INPUTS = 800

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
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        time.sleep(2) # Allow ESP32 to reset after serial connection
    except Exception as e:
        print(f"Failed to connect to {SERIAL_PORT}: {e}")
        return

    while True:
        try:
            print("\n" + "="*40)
            print("CLASSES: 0 = Front | 1 = Reverse | 2 = Noise")
            label_input = input("Enter class label for next recording (or 'q' to quit): ")
            
            if label_input.lower() == 'q':
                break
                
            label = int(label_input)
            
            print("Waiting for ESP32 button press...")
            
            while True:
                line = ser.readline().decode('utf-8').strip()
                
                if line == "Start speaking":
                    print(">>> START SPEAKING NOW <<<")
                
                # Check if the line is the data array (contains commas)
                elif "," in line:
                    data_points = line.split(",")
                    
                    if len(data_points) == NUMBER_OF_INPUTS:
                        # Append the label to the end of the data array
                        data_points.append(str(label))
                        
                        # Save to CSV
                        with open(OUTPUT_FILE, 'a', newline='') as f:
                            writer = csv.writer(f)
                            writer.writerow(data_points)
                        
                        print(f"Success: Saved 800 samples for Class {label}")
                        break
                    else:
                        print(f"Error: Received {len(data_points)} samples, expected {NUMBER_OF_INPUTS}.")
                        break
                        
        except ValueError:
            print("Invalid input. Please enter 0, 1, or 2.")
        except KeyboardInterrupt:
            print("\nExiting logger.")
            break

    ser.close()

if __name__ == "__main__":
    main()