# Lab Notebook

## Date - RFA

## Date - Project Proposal

## Date - Component Selection

## date - Team Contract

## Date - Design Document

## Date - Design Review

## Date - Github Setup

## Oct 14th - Breadboard Demo

## Oct 19th 
Objectives: The goal is to clean up the breadboard demo code in VScode.

Work Done: Migrated code from Arduino IDE to function on VScode with the arduino extension. There were some configuration issues that I was not initally familiar with. I needed to install an the Arduino Community Edition and the Arduino CLI. I also needed add the path of the CLI to the arduino.json file to enable verifying and uploading code to the Arduino DUE.

## Nov 1st 
Objective: Read data from IMU and Proximity sensor, and transmit data to mobile app.

Work Done: Soldered Arduino Nano 33 IoT dev board and started reading data from the IMU and proximity sensor. To set the threshold for tilt angle, the threshold is decided in degrees as sin(30 deg) = 0.5 to get the acceleration threshold 