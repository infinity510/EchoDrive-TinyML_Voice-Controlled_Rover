### **I. The Primary Objective & Hardware Shift**

The overarching goal was to port an acoustic Machine Learning model (a TensorFlow Lite Multi-Layer Perceptron trained to recognize voice commands via a KY-037 sensor) from an Arduino UNO to an ESP32.

The Arduino UNO (ATmega328P) possesses only 2 KB of SRAM. The model required approximately 16 KB of memory (`TENSOR_ARENA_SIZE`), which caused an immediate mathematical overflow on the UNO. The standalone ESP32 was selected as the replacement hardware because it provides 520 KB of SRAM, a 240 MHz dual-core 32-bit processor, and budget-friendly 3.3V logic compatibility.

---

### **II. Phase 1: The Library API Conflict (EloquentTinyML)**

**The Problem**
Upon pasting the initial C++ deployment code into the Arduino IDE, the compiler immediately threw a `fatal error: EloquentTinyML.h: No such file or directory`.

**The Cause**
The Arduino IDE Library Manager automatically downloaded the absolute newest version of the `EloquentTinyML` library (Version 3.0.1). The deployment code was written using the Version 2.x API. Version 3.x introduced massive breaking changes, including:

* Renaming the main header from capitalized `<EloquentTinyML.h>` to lowercase `<eloquent_tinyml.h>`.
* Flattening the `Eloquent::TinyML::TfLite` namespace.
* Removing automated mathematical operation mapping, requiring manual tensor configurations.

**The Action Taken**
We downgraded the `EloquentTinyML` library in the Library Manager from `3.0.1` to the stable, highly automated **`2.4.4`** release.

---

### **III. Phase 2: The Core Compiler Clash (GCC 12 vs. TensorFlow)**

**The Problem**
After downgrading the library, the compiler advanced further but abruptly crashed with a massive wall of red text. The primary errors were `error: deleted function 'virtual Eloquent::TinyML::TensorFlow::AllOpsResolver::~AllOpsResolver()'` and `operator delete(void*) is private`.

**The Cause**
This was the most severe conflict of the session. Espressif Systems recently updated the ESP32 Arduino Core to Version 3.x. This massive underlying update replaced the old C++ compiler (GCC 8.4.0) with a much newer, stricter compiler (GCC 12).

TensorFlow Lite for Microcontrollers utilizes a memory-saving macro called `TF_LITE_REMOVE_VIRTUAL_DELETE`. This macro hides the C++ `delete` operator to save SRAM, as neural networks run continuously and rarely need to free memory dynamically. GCC 12 strictly enforces modern C++ rules, which mandate that any class with a virtual destructor must have a public delete operator. The new ESP32 compiler saw the hidden delete operator in the TensorFlow backend, flagged it as an illegal C++ violation, and instantly terminated the build.

**The Action Taken**
We bypassed the strict C++ compiler entirely by downgrading the ESP32 Board Package itself. In the Boards Manager, the "esp32 by Espressif Systems" core was rolled back from `3.x.x` to **`2.0.17`**. This successfully restored the older, forgiving GCC 8.4.0 toolchain.

---

### **IV. Phase 3: The Namespace and Preprocessor Refinement**

**The Problem**
With both the library and the core compiler downgraded to stable V2 variants, the IDE threw a localized syntax error: `'TfLite' in namespace 'Eloquent::TinyML' does not name a template type`.

**The Cause**
In EloquentTinyML Version 2.4.4 specifically, the developer modularized the codebase to separate TensorFlow from other AI backends (like EdgeImpulse). The base `#include <EloquentTinyML.h>` no longer automatically loaded the TensorFlow math engine. Furthermore, the class name was moved into a highly specific, deeply nested namespace path.

**The Action Taken**
Two specific lines of code were injected into the C++ sketch to map directly to the 2.4.4 architecture:

1. A dedicated preprocessor directive was added: `#include <eloquent_tinyml/tensorflow.h>`.
2. The object declaration was updated to target the deep namespace: `Eloquent::TinyML::TensorFlow::TensorFlow<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;`.

---

### **V. Phase 4: The IDE Cache Trap**

**The Problem**
Despite the code, library, and compiler being perfectly aligned, the IDE continued to throw the exact same namespace errors.

**The Cause**
The terminal output revealed the phrase `Using cached sketch with function prototypes`. The Arduino IDE 2.x utilizes aggressive background caching to speed up build times. Instead of reading the newly corrected C++ text, it was fetching the corrupted, pre-compiled `.o` object files generated during the previous failed GCC 12 attempts.

**The Action Taken**
The build cache was manually invalidated. The target board was temporarily changed to "Arduino UNO" in the Tools menu, forcing the IDE to abandon the ESP32 cache. The board was then switched back to "ESP32 Dev Module," forcing the GCC 8.4.0 compiler to process the C++ source code entirely from scratch.

---

### **VI. The Final Working Configuration**

Upon flushing the cache, the GCC 8.4.0 compiler perfectly handshook with the EloquentTinyML 2.4.4 library, parsing the 16 KB tensor arena without throwing virtual destructor violations. The terminal outputted `Successfully created esp32 image`.

**The Definitive Working Environment:**

* **Hardware Profile:** ESP32 Dev Module (DOIT ESP32 DEVKIT V1)
* **ESP32 Board Core:** Version 2.0.17 (Restoring GCC 8.4.0)
* **Machine Learning Library:** EloquentTinyML Version 2.4.4
* **ADC Hardware Synchronization:** The command `analogReadResolution(10);` was implemented in the `setup()` function to force the ESP32's native 12-bit ADC (0-4095) down to a 10-bit scale (0-1023). This was mandatory to ensure mathematical parity with the original Python dataset generated by the Arduino UNO.
* **Memory Footprint:** The final compiled firmware utilized only 13% of the ESP32's dynamic memory (44,284 bytes), leaving 87% completely free, proving the hardware migration was a complete success.