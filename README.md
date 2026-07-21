<h1 align="center">Handheld Oscilloscope using ESP32</h1>

<p align="center">
  This project presents the design and fabrication of a portable, low-cost handheld oscilloscope based on the <strong>ESP32</strong> microcontroller. Designed for educational and practical engineering purposes, this device provides real-time waveform visualization and signal parameter analysis.
</p>

---

## 🏗 System Architecture & Workflow

### 1. Hardware System Block Diagram
<p align="center">
  <img src="image/diagram/Hardware_Block_cropped.pdf" alt="Hardware System Block Diagram" width="850">
</p>

### 2. Firmware Execution Flowchart
<p align="center">
  <img src="image/diagram/Flowchart_2_cropped (1).pdf" alt="Firmware Flowchart" width="850">
</p>

---

## 🚀 Key Features
*   **Dual-Core Processing:** Uses ESP32 dual-core architecture to separate high-frequency signal sampling (Core 1) from UI rendering and communication tasks (Core 0).
*   **Real-time Visualization:** Supports local TFT ILI9341 display and remote Web Interface monitoring via WebSocket.
*   **Analog Frontend:** Integrates LM358 Op-Amp for DC biasing and signal conditioning, allowing measurement of bipolar signals.
*   **Cost-Effective:** Low component cost (approx. $20), making it an ideal tool for students.
*   **Auto-Zero Calibration:** Software-based calibration to eliminate systematic DC offsets.

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
