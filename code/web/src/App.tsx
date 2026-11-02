
// src/App.tsx

import { useState } from "react";
import {
  connectToArduino,
  disconnectFromArduino,
  sendCommand,
  isBluetoothSupported,
  type Telemetry,
} from "./ble";

import "./App.css";

function App() {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);

  const [deviceName, setDeviceName] = useState("");

  const [status, setStatus] = useState("Disconnected");

  const [telemetry, setTelemetry] =
    useState<Telemetry | null>(null);

  const [lastUpdated, setLastUpdated] =
    useState<string>("--");

  const [commandBusy, setCommandBusy] = useState(false);

  // Connect to Arduino.
  const handleConnect = async () => {
    try {
      setConnecting(true);
      setStatus("Connecting...");

      const name = await connectToArduino(
        (data) => {
          setTelemetry(data);
          setLastUpdated(
            new Date().toLocaleTimeString()
          );
        },
        () => {
          setConnected(false);
          setDeviceName("");
          setTelemetry(null);
          setStatus("Disconnected");
        }
      );

      setDeviceName(name);
      setConnected(true);
      setStatus("Connected");
    } catch (error) {
      console.error(error);

      setStatus(
        error instanceof Error
          ? error.message
          : "Connection failed"
      );
    } finally {
      setConnecting(false);
    }
  };

  // Disconnect from Arduino.
  const handleDisconnect = () => {
    disconnectFromArduino();

    setConnected(false);
    setDeviceName("");
    setTelemetry(null);
    setStatus("Disconnected");
  };

  // Send LED commands.
  const handleCommand = async (command: string) => {
    try {
      setCommandBusy(true);

      await sendCommand(command);

      setStatus(`Command sent: ${command}`);
    } catch (error) {
      console.error(error);

      setStatus(
        error instanceof Error
          ? error.message
          : "Command failed"
      );
    } finally {
      setCommandBusy(false);
    }
  };

  const distance = telemetry?.distance_cm;

  return (
    <div className="app">
      <header className="header">
        <div>
          <h1>STRIDE SECURE</h1>
          <p>Device Monitoring & Control</p>
        </div>

        <div
          className={`connection-indicator ${
            connected ? "online" : "offline"
          }`}
        >
          <span className="status-dot" />
          {connected ? "Connected" : "Disconnected"}
        </div>
      </header>

      <main className="dashboard">
        {/* Device Connection */}
        <section className="card connection-card">
          <div className="section-heading">
            <h2>Device Connection</h2>
            <span className="device-icon">BLE</span>
          </div>

          <p className="description">
            Connect to your Arduino using Bluetooth Low Energy.
          </p>

          <div className="device-info">
            <span className="label">Device</span>
            <strong>
              {connected ? deviceName : "No device connected"}
            </strong>
          </div>

          <div className="device-info">
            <span className="label">Status</span>
            <strong>{status}</strong>
          </div>

          {!connected ? (
            <button
              className="primary-button"
              onClick={handleConnect}
              disabled={
                connecting || !isBluetoothSupported()
              }
            >
              {connecting
                ? "Connecting..."
                : "Find & Connect Device"}
            </button>
          ) : (
            <button
              className="disconnect-button"
              onClick={handleDisconnect}
            >
              Disconnect
            </button>
          )}

          {!isBluetoothSupported() && (
            <p className="warning">
              Web Bluetooth is unavailable in this browser.
              Try Chrome on desktop or a compatible iOS browser.
            </p>
          )}
        </section>

        {/* Device Controls */}
        <section className="card">
          <div className="section-heading">
            <h2>Device Controls</h2>
          </div>

          <p className="description">
            Control the Arduino onboard LED.
          </p>

          <div className="controls">
            <button
              className="primary-button"
              disabled={!connected || commandBusy}
              onClick={() => handleCommand("LED_ON")}
            >
              Turn LED ON
            </button>

            <button
              className="secondary-button"
              disabled={!connected || commandBusy}
              onClick={() => handleCommand("LED_OFF")}
            >
              Turn LED OFF
            </button>
          </div>
        </section>

        {/* Sensor Dashboard */}
        <section className="card sensor-section">
          <div className="section-heading">
            <h2>Sensor Dashboard</h2>
            <span className="live-label">
              {connected ? "LIVE" : "OFFLINE"}
            </span>
          </div>

          <div className="sensor-grid">
            <div className="sensor-card">
              <span className="sensor-label">
                Distance
              </span>

              <div className="sensor-value">
                {distance === undefined
                  ? "--"
                  : distance < 0
                  ? "N/A"
                  : distance.toFixed(1)}
              </div>

              <span className="sensor-unit">cm</span>
            </div>

            <div className="sensor-card">
              <span className="sensor-label">
                Tilt Status
              </span>

              <div className="sensor-value">
                {telemetry?.tilt_status ?? "--"}
              </div>
            </div>
          </div>

          <div className="updated">
            Last updated: {lastUpdated}
          </div>
        </section>
      </main>

      <footer>
        <p>Stride Secure | BLE Device Interface</p>
      </footer>
    </div>
  );
}

export default App;
