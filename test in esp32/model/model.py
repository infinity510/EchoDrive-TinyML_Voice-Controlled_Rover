import numpy as np
import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split
from tensorflow.keras import layers, models
import os

# ==========================================
# 1. DATA INGESTION & PREPROCESSING
# ==========================================
print("Loading dataset.csv...")
df = pd.read_csv('dataset.csv')

# Separate features (800 columns) and labels (last column)
X = df.iloc[:, :-1].values.astype('float32')
y = df.iloc[:, -1].values.astype('int32')

# Normalize the 12-bit ESP32 data (0 to 4095) to float values between 0.0 and 1.0
X = X / 4095.0

# Split into 80% training and 20% test sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# ==========================================
# 2. NEURAL NETWORK ARCHITECTURE
# ==========================================
model = models.Sequential([
    layers.InputLayer(input_shape=(800,)),
    
    # Hidden layers designed to compress the large 800-element array
    layers.Dense(128, activation='relu'),
    layers.Dropout(0.3), 
    layers.Dense(64, activation='relu'),
    layers.Dropout(0.2),
    layers.Dense(32, activation='relu'),
    
    # Output layer: 3 classes (Front, Reverse, Noise) using Softmax for probabilities
    layers.Dense(3, activation='softmax')
])

model.compile(optimizer='adam',
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

print("Starting model training...")
# The test set is passed as validation_data to track metrics during epochs
history = model.fit(X_train, y_train, epochs=100, batch_size=16, validation_data=(X_test, y_test))

# ==========================================
# 3. TEST DATASET EVALUATION 
# ==========================================
print("\nEvaluating model against the reserved unseen test dataset...")
test_loss, test_accuracy = model.evaluate(X_test, y_test, verbose=0)

print("\n" + "="*50)
print(f"FINAL TEST ACCURACY: {test_accuracy * 100:.2f}%")
print(f"FINAL TEST LOSS:     {test_loss:.4f}")
print("="*50 + "\n")

# ==========================================
# 4. TENSORFLOW LITE MICRO CONVERSION
# ==========================================
print("Converting model to TensorFlow Lite format...")
converter = tf.lite.TFLiteConverter.from_keras_model(model)
#converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

# Save the raw .tflite binary
with open("model.tflite", "wb") as f:
    f.write(tflite_model)

# ==========================================
# 5. C++ HEADER FILE GENERATION
# ==========================================
print("Generating model_data.h for ESP32...")

def hex_to_c_array(hex_data, var_name):
    c_str = ''
    c_str += f'#ifndef {var_name.upper()}_H\n'
    c_str += f'#define {var_name.upper()}_H\n\n'
    c_str += f'#ifdef __has_attribute\n'
    c_str += f'#define HAVE_ATTRIBUTE(x) __has_attribute(x)\n'
    c_str += f'#else\n'
    c_str += f'#define HAVE_ATTRIBUTE(x) 0\n'
    c_str += f'#endif\n\n'
    c_str += f'#if HAVE_ATTRIBUTE(aligned)\n'
    c_str += f'#define ALIGN_ATTRIBUTE __attribute__((aligned(4)))\n'
    c_str += f'#else\n'
    c_str += f'#define ALIGN_ATTRIBUTE\n'
    c_str += f'#endif\n\n'
    c_str += f'const unsigned char {var_name}[] ALIGN_ATTRIBUTE = {{\n'
    
    # Format hex string into 12 columns per row
    hex_array = [hex(b) for b in hex_data]
    for i in range(0, len(hex_array), 12):
        c_str += '  ' + ', '.join(hex_array[i:i+12]) + ',\n'
        
    c_str += '};\n\n'
    c_str += f'const unsigned int {var_name}_len = {len(hex_data)};\n\n'
    c_str += f'#endif // {var_name.upper()}_H\n'
    return c_str

# Write the C++ header file
with open("model_data.h", "w") as f:
    f.write(hex_to_c_array(tflite_model, "acoustic_nn_model"))

print("Complete. 'model_data.h' has been generated in the current directory.")