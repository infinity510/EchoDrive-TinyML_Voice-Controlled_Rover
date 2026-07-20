import pandas as pd
import numpy as np
import json
import tensorflow as tf
from tensorflow.keras import layers, models
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

# ==========================================
# 1. DATA INGESTION & JUMBLING
# ==========================================
df = pd.read_csv('acoustic_dataset_2D.csv')

def extract_amplitudes(json_str):
    data = json.loads(json_str)
    return [pair[1] for pair in data]

# Extract features and labels
X = np.array(df['2D_Array_Data'].apply(extract_amplitudes).tolist())
y = df['Label'].values

# Shuffle and split the dataset (80% Training, 20% Testing)
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, shuffle=True, stratify=y)

# Feature Scaling (Crucial for Neural Network gradient descent convergence)
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# ==========================================
# 2. NEURAL NETWORK ARCHITECTURE
# ==========================================
model = models.Sequential([
    # Input layer expects a 1D array of 200 features
    layers.Input(shape=(200,)),
    
    # Hidden Layer 1
    layers.Dense(100, activation='relu'),
    layers.Dropout(0.2), # Prevents overfitting
    
    # Hidden Layer 2
    layers.Dense(64, activation='relu'),
    
    # Output Layer (3 neurons for 3 classes, Softmax for probabilities)
    layers.Dense(3, activation='softmax')
])

model.compile(optimizer='adam',
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

# Train the model
print("Commencing Model Training...")
history = model.fit(X_train_scaled, y_train, epochs=3000, batch_size=16, validation_data=(X_test_scaled, y_test), verbose=1)

# Evaluate final accuracy
test_loss, test_acc = model.evaluate(X_test_scaled, y_test, verbose=0)
print(f"\nFinal Test Accuracy: {test_acc * 100:.2f}%")

# ==========================================
# 3. TENSORFLOW LITE MICRO CONVERSION
# ==========================================
# Convert the Keras model to a lightweight TFLite model optimized for ARM Cortex-M4
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT] # Applies 8-bit weight quantization
tflite_model = converter.convert()

# Save the binary .tflite file
with open('acoustic_model.tflite', 'wb') as f:
    f.write(tflite_model)

# ==========================================
# 4. C++ BYTE ARRAY GENERATOR
# ==========================================
# Microcontrollers cannot read .tflite files from a hard drive. 
# The model must be converted into a C++ hex array to be compiled directly into flash memory.
def hex_to_c_array(tflite_bytes, var_name):
    c_str = f"const unsigned char {var_name}[] __attribute__((aligned(4))) = {{\n"
    hex_array = [hex(b) for b in tflite_bytes]
    for i in range(0, len(hex_array), 12):
        c_str += "    " + ", ".join(hex_array[i:i+12]) + ",\n"
    c_str += "};\n"
    c_str += f"const int {var_name}_len = {len(tflite_bytes)};\n"
    return c_str

c_header_content = hex_to_c_array(tflite_model, "acoustic_nn_model")

with open('model_data.h', 'w') as f:
    f.write(c_header_content)

print("\nModel successfully compiled and exported to 'model_data.h'")