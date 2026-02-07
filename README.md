# Smart Dashboard System

This project presents the design and implementation of a **Smart Car Dashboard System** based on an embedded real-time architecture using an **STM32F407VGT6** microcontroller running **FreeRTOS**.
The system monitors critical vehicle parameters and transmits data over a **Controller Area Network (CAN)** bus to an **ESP32 gateway** for cloud integration and visualization.

---

## 🚀 System Overview

The system emulates a real-world automotive dashboard by integrating real-time data acquisition, robust in-vehicle communication, and cloud connectivity.

* **Vehicle ECU**: STM32F407VGT6 manages sensors and CAN communication using FreeRTOS tasks.
* **Gateway ECU**: ESP32 receives CAN data via an MCP2515 module and forwards it to a DWIN LCD and AWS IoT.
* **Cloud Integration**: Uses **MQTT protocol** over Wi-Fi to publish data to Amazon Web Services (AWS) IoT for remote monitoring.
* **HMI**: A **DWIN LCD** provides an interactive graphical interface for real-time visualization.

---

## 🛠️ Hardware Architecture

### System Block Diagram
![Block Diagram](block%20diagram.png)

### Key Components
| Component | Purpose | Interface |
| :--- | :--- | :--- |
| **STM32F407VGT6** | Main Vehicle ECU (Data Processing) | CAN, ADC, GPIO |
| **ESP32** | Gateway Controller (IoT & Display) | SPI, UART, Wi-Fi |
| **SN65HVD230** | CAN Transceiver (Physical Layer) | CAN  |
| **MCP2515** | Standalone CAN Controller for ESP32 | SPI |
| **DWIN LCD** | Dashboard Visualization (7-inch TFT) | UART (DGUS) |

### Sensors Used
* **ACS712**: Measures battery charging and discharging current.
* **Voltage Sensor**: Monitors real-time battery voltage.
* **LM35**: Monitors battery and motor temperature for safety.
* **LM393**: Detects wheel/motor rotation for speed calculation.
* **Digital Switches**: Monitor door status and seatbelt status.

---

## 💻 Software Implementation

### Real-Time Operating System (FreeRTOS)
The STM32 utilizes FreeRTOS to manage concurrent tasks with deterministic timing:
1. **Task_SensorRead (High Priority)**: Acquires data from all sensors every 50ms.
2. **Task_CANTransmit (Medium Priority)**: Formats data into CAN frames and transmits every 100ms.
3. **Task_Monitor (Low Priority)**: Oversees system health and CAN bus-off conditions every 1000ms.

### Communication Protocol (CAN)
* **Standard**: ISO 11898.
* **Baud Rate**: 500 kbps.
* **Mechanism**: Uses bit-wise arbitration based on message priority.

---

## 📈 Project Results
![Dashboard UI](01.JPG)
*The DWIN Display showing real-time Car Speed and Battery status.*

* **Data Accuracy**: Successfully acquired RPM, speed, temperature, and battery State of Charge (SOC).
* [**Reliability**: Established robust CAN communication between STM32 and ESP32.
* **Cloud Connectivity**: Continuous data logging to AWS IoT via MQTT over Wi-Fi.

---

## 📜 Authors (C-DAC, Pune)
* **Akash Dubey** 
* **Allu Taraka Rama Sai Aravind**
* **Bhavesh Chandrashekhar Satkar** 
* **Nadarge Shivam Santosh**
* **Sambaladeevi Gowtham**

**Guided by:** Mr. Shreepad Deshpande
