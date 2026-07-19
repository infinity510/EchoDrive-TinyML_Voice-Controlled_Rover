
### **Project Description**

**Core Objective:**
To engineer a custom voice-controlled robotics platform by bridging low-resolution analog hardware with robust supervised machine learning algorithms. The system processes raw acoustic energy to classify distinct verbal commands and executes physical driving maneuvers autonomously.

**Hardware Architecture:**
The physical platform utilizes an Arduino UNO R4 WiFi tethered to a 4-wheel drive RC chassis. An L293D dual H-bridge motor driver handles the high-current demands of the DC motors. Acoustic input is captured via a standard analog sound sensor (KY-037), which relies on a dynamic environmental calibration loop to establish an active noise floor threshold.

**The Engineering Challenge:**
Standard speech recognition relies on Mel-Frequency Cepstral Coefficients (MFCCs) to analyze the frequency domain of audio. The KY-037 analog sensor strips away all frequency fidelity, outputting only a raw amplitude envelope. Traditional voice classification is impossible under these hardware constraints.

**The Machine Learning Solution:**
The project pivots the machine learning architecture from speech recognition to **Temporal Geometric Classification**.

* **Data Pipeline:** The microcontroller is programmed to capture a 1-second window of acoustic data immediately following a threshold trigger, sampling 200 distinct amplitude measurements at 5-millisecond intervals.
* **Feature Engineering:** This data is routed via serial communication to a Python logging script, which formats the readings into a 200-dimensional feature vector ($X$) and appends a target label ($y$).
* **Supervised Learning:** By intentionally articulating command words with physically distinct rhythms (e.g., a sharp explosive syllable vs. a sustained drawn-out vowel), the resulting data matrices exhibit unique mathematical shapes. The dataset is ingested into Scikit-Learn and TensorFlow, where Multiclass Logistic Regression and Neural Networks (Multilayer Perceptrons) are trained to calculate decision boundaries separating the temporal energy profiles of different commands.

**Final Deployment:**
The trained mathematical model acts as the brain of the rover. It evaluates live 200-point time-series matrices captured by the Arduino, predicts the intended command with high statistical confidence, and triggers the corresponding logic gates on the L293D to drive or halt the physical vehicle.
