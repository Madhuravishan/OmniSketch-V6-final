<div align="center">

# 🤖 OmniSketch (Project Scrubby)
### *Autonomous ESP32-Powered Wall-Plotter & Whiteboard Drawing Robot*

[![Faculty of IT, University of Moratuwa](https://img.shields.io/badge/UoM-Faculty%20of%20IT-blue.svg)](https://it.mrt.ac.lk/)
[![Grade: A](https://img.shields.io/badge/Evaluation%20Grade-A-success.svg)]()
[![Status: Completed](https://img.shields.io/badge/Status-Completed-brightgreen.svg)]()

*An open-source, precision V-plotter built from scratch over 15 months of hardware engineering, custom firmware development, and a bespoke web-based control application.*

</div>

---

## ✨ Overview
**OmniSketch** (also known as *Scrubby*) is a wall-hanging drawing robot designed and developed as our first-year hardware project at the Faculty of Information Technology, University of Moratuwa. By integrating custom kinematics, dual NEMA 17 stepper motors, an ESP32 microcontroller, and a custom web application hosted directly on the device, the machine is capable of rendering intricate, continuous-line art and vector graphics directly onto vertical surfaces.

---

## 🛠️ Tech Stack & Hardware Architecture

| Component / Subsystem | Technology / Hardware Used | Role / Description |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 DevKit V1 | Core processing unit running custom firmware and serving the web app |
| **Software Stack** | Custom Web Application & LittleFS | Local web-based control panel, file handling, and motion execution |
| **Motor Drivers** | DRV8825 (U3 & U4) | Precise microstepping for left and right NEMA 17 motors |
| **Actuators** | NEMA 17 Steppers | Drive the V-plotter suspension belts |
| **Pen Lift Mechanism**| Dual MG90S Servos (U9, U11) | Precise up/down pen actuation with 180° transform mapping |
| **Laser Module** | Laser diode via IRLZ44N MOSFET | Low-side switching via GPIO 16 (RX2) with gate protection |
| **Display & Feedback**| SSD1306 OLED (I2C) & Buzzer | System status display and audio alerts on GPIO 4 |

---

## 📌 Pinout & Signal Mapping

| Signal Name | ESP32 GPIO | Hardware Connection / Notes |
| :--- | :--- | :--- |
| **STEP_L / DIR_L / EN_L** | GPIO18 / GPIO14 / GPIO13 | Left DRV8825 — STEP / DIR / ENABLE (Active LOW) |
| **STEP_R / DIR_R / EN_R** | GPIO27 / GPIO26 / GPIO25 | Right DRV8825 — STEP / DIR / ENABLE (Active LOW) |
| **SERVO_PEN1** | GPIO33 | MG90S Pen 1 (U11 connector) — Servo control |
| **SERVO_PEN2** | GPIO32 | MG90S Pen 2 (U9 connector) — Mirrored pen (180° transform) |
| **SCL / SDA (I2C)** | GPIO22 / GPIO21 | SSD1306 OLED Display (400 kHz, pull-ups to 3.3V) |
| **BUZZER** | GPIO4 | Active buzzer via U10 header (LEDC channel 15) |
| **LASER (Gate)** | GPIO16 | IRLZ44N MOSFET gate (Q1) — Low-side switch with gate resistor & pulldown |

---

## 🚀 Key Features
* **V-Plotter Kinematics:** Custom mathematical string-length calculations for precise coordinate plotting on vertical planes.
* **Custom Web Control Interface:** Fully responsive, locally hosted web application built from scratch to interface directly with the ESP32.
* **Robust Circuit Protection:** Designed with low-side MOSFET switching, pull-down resistors, and strict memory management on the LittleFS partition.

---

## 👥 The Team
Developed with passion and dedication by Batch 24 IT Undergraduates, University of Moratuwa:

* **Kavindu Kalhara**
* **Madhura Ravishan** 
* **Manuri Pabara**
* **Dilki Nimeshika**
* **Senan Senujaya**

### 🎓 Acknowledgements
Our deepest gratitude to our project supervisors for their invaluable guidance, constant support, and technical insights:
* **Mr. B.H. Sudantha**
* **Mr. Hemantha Wanniarachchi**
* **Ms. Lakdini Manchanayaka**

---

<div align="center">
  <p><i>Faculty of Information Technology • University of Moratuwa • 2026</i></p>
</div>
