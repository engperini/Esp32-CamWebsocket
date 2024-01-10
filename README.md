# Esp32-CamWebsocket
A WebSocket ESP32-CAM Streaming Server with BME Sensor Reading and Commands

This repository contains code for controlling an ESP32-CAM using websockets. It utilizes various sensors and functionalities to enable remote control and sensor data monitoring.

## Table of Contents
- [Overview](#overview)
- [Dependencies](#dependencies)
- [Functionality](#functionality)
- [Setup](#setup)
- [Web Interface](#web-interface)
- [Camera Configuration](#camera-configuration)
- [WebSocket Events](#websocket-events)
- [Sensor Data Handling](#sensor-data-handling)
- [Control Functions](#control-functions)

## Overview
The code controls an ESP32-CAM and integrates various libraries for different functionalities:
- Camera control and streaming
- WebSockets for real-time communication
- Sensor readings for BME280 and VL53L1X

![image](https://github.com/engperini/Esp32-CamWebsocket/assets/117356668/e64fbe96-a4df-4661-a802-344a1b649a57)


## Dependencies
The code utilizes several libraries:
- Adafruit BME280 for environmental sensing
- Adafruit VL53L1X for distance measurements
- AsyncTCP and ESPAsyncWebServer for asynchronous handling
- ArduinoJson for JSON handling
- Adafruit SSD1306 and Adafruit GFX for OLED display

## Functionality
The code provides functionalities for:
- Websocket server setup
- Live camera streaming
- Reading sensor data (temperature, pressure, altitude, humidity, distance)
- Controlling the ESP32-CAM remotely via WebSocket commands

## Setup
1. Configure the ESP32-CAM board with specified GPIO pins
2. Connect to Wi-Fi network
3. Run the server on port 80
4. Initialize sensors and camera configuration
5. Handle WebSocket events and incoming client commands

## Web Interface
The code serves a simple web interface for:
- Displaying sensor readings in real-time
- Providing controls for the ESP32-CAM (light, photo capture, reset)

## Camera Configuration
The `configCamera()` function initializes the camera settings required for capturing frames.

## WebSocket Events
- `webSocketEvent()` handles WebSocket events like client connection, disconnection, and incoming text data.
- Processes client commands received via WebSocket, performs actions based on the received commands.

## Sensor Data Handling
- Reads sensor data at specified intervals and sends it via WebSocket when available.
- Shows sensor data on an OLED display.

## Control Functions
- Manages functionalities triggered by client commands (e.g., light control, photo capture, automatic photo mode, reset).

Please refer https://github.com/abish7643/ESP32Cam-I2CSensors to lib errors using esp32cam with adafruit lib

This README provides an overview of the code functionalities and setup process for controlling ESP32-CAM via websockets. Please refer to the code comments and specific functions for detailed implementation.

