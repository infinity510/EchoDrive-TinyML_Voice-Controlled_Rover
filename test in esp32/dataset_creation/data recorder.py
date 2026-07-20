import serial
import csv
import time
import json
import sys

# ==========================================
# HARDWARE CONFIGURATION
# ==========================================
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 115200

# Updated to expect all 200 samples captured by the Arduino
NUM_SAMPLES = 200 

# TARGET LABEL (0 = Front, 1 = Back, 2 = Noise)
CURRENT_LABEL = 2
CSV_FILENAME = 'acoustic_dataset_2D.csv'

def initialize_serial():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        return ser
    except serial.SerialException as e:
        print(f"Hardware Error: Port {SERIAL_PORT} unavailable. Close the Arduino Serial Monitor.")
        sys.exit()

def main():
    ser = initialize_serial()
    
    time.sleep(2) 
    print(f"\n[SYSTEM ACTIVE]")
    print(f"Target Label Set To: {CURRENT_LABEL}")
    print(f"Waiting for hardware calibration to complete...\n")

    with open(CSV_FILENAME, mode='a', newline='') as file:
        writer = csv.writer(file)
        
        if file.tell() == 0:
            writer.writerow(["2D_Array_Data", "Label", "Threshold", "Average"])
        
        while True:
            try:
                line = ser.readline().decode('utf-8').strip()
                if not line:
                    continue
                
                # 1. CATCH CALIBRATION
                if "calliberation done" in line:
                    print(f"[HARDWARE READY] {line}")
                    print("Press the physical button on your breadboard to begin recording.")

                # 2. CATCH SPEAK PROMPT
                elif line == "speakup":
                    print("\n>> SPEAK NOW! <<")

                # 3. CATCH DATA TRANSMISSION
                elif line.startswith("threshold ="):
                    threshold_val = int(line.split("=")[1].strip())
                    
                    avg_line = ser.readline().decode('utf-8').strip()
                    avg_val = int(avg_line.split("=")[1].strip())
                    
                    two_dimensional_array = []
                    x_index = 1
                    
                    while True:
                        data_line = ser.readline().decode('utf-8').strip()
                        
                        if "readings successfully taken" in data_line:
                            print("[CAPTURE COMPLETE] Ready for next button press.")
                            break
                        
                        try:
                            y_val = int(data_line)
                            two_dimensional_array.append([x_index, y_val])
                            x_index += 1
                        except ValueError:
                            pass 
                    
                    if len(two_dimensional_array) == NUM_SAMPLES:
                        array_string = json.dumps(two_dimensional_array)
                        
                        writer.writerow([array_string, CURRENT_LABEL, threshold_val, avg_val])
                        file.flush() 
                        print(f"-> Successfully saved {NUM_SAMPLES} features to CSV. Label: {CURRENT_LABEL}\n")
                    else:
                        print(f"-> Error: Expected {NUM_SAMPLES} readings, got {len(two_dimensional_array)}. Matrix discarded.\n")

            except KeyboardInterrupt:
                print("\n[TERMINATED] Data pipeline closed by user.")
                ser.close()
                break
            except Exception as e:
                pass 

if __name__ == '__main__':
    main()