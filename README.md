<h1 align="center">Handheld Oscilloscope using ESP32</h1>

<p align="center">
  This project presents the design and fabrication of a portable, low-cost handheld oscilloscope based on the <strong>ESP32</strong> microcontroller. Designed for educational and practical engineering purposes, this device provides real-time waveform visualization and signal parameter analysis.
</p>

<p align="center">
  <img width="387" height="623" alt="image" src="https://github.com/user-attachments/assets/17bf38c7-9927-437c-acfe-6d05c52f481b" />
</p>

## 🚀 Key Features
*   **Dual-Core Processing:** Uses ESP32 dual-core architecture to separate high-frequency signal sampling (Core 1) from UI rendering and communication tasks (Core 0).
*   **Real-time Visualization:** Supports local TFT ILI9341 display and remote Web Interface monitoring via WebSocket.
*   **Analog Frontend:** Integrates LM358 Op-Amp for DC biasing and signal conditioning, allowing measurement of bipolar signals.
*   **Cost-Effective:** Low component cost (approx. $20), making it an ideal tool for students.
*   **Auto-Zero Calibration:** Software-based calibration to eliminate systematic DC offsets.

## 🛠 Hardware Architecture
The system comprises two main subsystems:
1.  **Analog Frontend (AFE):** Signal conditioning, 1X/10X attenuation, AC/DC coupling, and DC biasing using LM358.
2.  **Digital Block:** ESP32 MCU performing 12-bit ADC sampling and data processing.


<img width="382" height="672" alt="image" src="https://github.com/user-attachments/assets/c77f497c-f520-44b9-ab54-77bb84510ff7" />



## 💻 Tech Stack
*   **Microcontroller:** ESP32-WROOM-32
*   **Programming:** C++ (Arduino IDE)
*   **Graphics:** TFT ILI9341 Display, SPI Interface
*   **Communication:** WebSocket, Wi-Fi
*   **Simulation & Design:** MATLAB, EDA Tools for PCB Layout

## 📊 Performance Indicators
| Frequency (Hz) | Amplitude Deviation | SNR (dB) | THD (%) |
| :--- | :--- | :--- | :--- |
| 100 | 0.2% | 45.2 | 1.1% |
| 1,000 | 0.8% | 41.5 | 2.4% |
| 10,000 | 1.5% | 36.8 | 5.7% |

## 📂 Repository Structure
*   `/Firmware`: C++ source code for ESP32.
*   `/Schematic_PCB`: Design files (Schematic and PCB layout).
*   `/Docs`: Project reports and documentation.
*   `/WebInterface`: Source code for the real-time Web monitoring interface.

## 👥 Group Members (Group 6)
*   **Tran Tuan Minh** (SE203134) - Signal Processing
*   **Pham Dang Khanh Duy** (SE203985) - Firmware
*   **Nguyen Huy Hoang** (SE203347) - Hardware
*   **Ho Phan Anh Tuan** (SE203437) - System Design
*   **Nguyen Duy An** (SE203864) - Integration

## 🎓 Instructor
Dr. Le The Dung - Department of Semiconductor IC Design, FPT University.
