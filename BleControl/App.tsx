
import React, { useEffect, useRef, useState } from "react";

import {
  Alert,
  Button,
  PermissionsAndroid,
  Platform,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from "react-native";

import {
  BleManager,
  Device,
  Subscription,
} from "react-native-ble-plx";

import { Buffer } from "buffer";

// ----------------------------
// BLE UUIDs
// Must match Arduino firmware
// ----------------------------
const SERVICE_UUID =
  "12345678-1234-5678-1234-56789abcdef0";

const COMMAND_UUID =
  "12345678-1234-5678-1234-56789abcdef1";

const TELEMETRY_UUID =
  "12345678-1234-5678-1234-56789abcdef2";

type Telemetry = {
  distance_cm: number;
  dist_raw: number;
  tilt_raw: number;
};

// ----------------------------
// Android BLE permissions
// ----------------------------
async function requestBlePermissions() {
  if (Platform.OS !== "android") {
    return true;
  }

  if (Number(Platform.Version) >= 31) {
    const results = await PermissionsAndroid.requestMultiple([
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
    ]);

    return (
      results[PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN] ===
        PermissionsAndroid.RESULTS.GRANTED &&
      results[PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT] ===
        PermissionsAndroid.RESULTS.GRANTED
    );
  }

  const result = await PermissionsAndroid.request(
    PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION
  );

  return result === PermissionsAndroid.RESULTS.GRANTED;
}

// ----------------------------
// Main application
// ----------------------------
export default function App() {
  const managerRef = useRef<BleManager | null>(null);

  if (!managerRef.current) {
    managerRef.current = new BleManager();
  }

  const manager = managerRef.current;

  const [devices, setDevices] = useState<Device[]>([]);
  const [connectedDevice, setConnectedDevice] =
    useState<Device | null>(null);

  const [isScanning, setIsScanning] = useState(false);
  const [isConnecting, setIsConnecting] = useState(false);

  const [telemetry, setTelemetry] =
    useState<Telemetry | null>(null);

  const [message, setMessage] = useState(
    "Disconnected"
  );

  const subscriptionRef =
    useRef<Subscription | null>(null);

  // Clean up Bluetooth resources
  useEffect(() => {
    return () => {
      subscriptionRef.current?.remove();
      manager.destroy();
    };
  }, [manager]);

  // ----------------------------
  // Scan for Arduino devices
  // ----------------------------
  const scanForDevices = async () => {
    const permissionGranted =
      await requestBlePermissions();

    if (!permissionGranted) {
      Alert.alert(
        "Permission required",
        "Bluetooth permissions are required to scan for devices."
      );
      return;
    }

    setDevices([]);
    setIsScanning(true);
    setMessage("Scanning for devices...");

    manager.startDeviceScan(
      [SERVICE_UUID],
      null,
      (error, device) => {
        if (error) {
          setIsScanning(false);
          setMessage(error.message);
          return;
        }

        if (device) {
          setDevices((previous) => {
            const alreadyFound = previous.some(
              (item) => item.id === device.id
            );

            if (alreadyFound) {
              return previous;
            }

            return [...previous, device];
          });
        }
      }
    );

    // Stop scanning after 10 seconds
    setTimeout(() => {
      manager.stopDeviceScan();
      setIsScanning(false);
      setMessage("Scan complete");
    }, 10000);
  };

  // ----------------------------
  // Connect to Arduino
  // ----------------------------
  const connectToDevice = async (device: Device) => {
    try {
      manager.stopDeviceScan();
      setIsScanning(false);
      setIsConnecting(true);
      setMessage("Connecting...");

      const connected = await manager.connectToDevice(
        device.id
      );

      await connected.discoverAllServicesAndCharacteristics();

      setConnectedDevice(connected);
      setMessage("Connected to " + (device.name || "Arduino"));

      // Subscribe to sensor notifications
      subscriptionRef.current =
        manager.monitorCharacteristicForDevice(
          connected.id,
          SERVICE_UUID,
          TELEMETRY_UUID,
          (error, characteristic) => {
            if (error) {
              setMessage("Telemetry error: " + error.message);
              return;
            }

            if (characteristic?.value) {
              try {
                const decoded = Buffer.from(
                  characteristic.value,
                  "base64"
                ).toString("utf8");

                const data: Telemetry = JSON.parse(decoded);

                setTelemetry(data);
              } catch (error) {
                console.error(
                  "Could not parse telemetry:",
                  error
                );
              }
            }
          }
        );
    } catch (error) {
      setMessage("Connection failed");

      Alert.alert(
        "Bluetooth connection error",
        String(error)
      );
    } finally {
      setIsConnecting(false);
    }
  };

  // ----------------------------
  // Send command to Arduino
  // ----------------------------
  const sendCommand = async (command: string) => {
    if (!connectedDevice) {
      Alert.alert("Not connected", "Connect to your Arduino first.");
      return;
    }

    try {
      const encoded = Buffer.from(
        command,
        "utf8"
      ).toString("base64");

      await manager.writeCharacteristicWithResponseForDevice(
        connectedDevice.id,
        SERVICE_UUID,
        COMMAND_UUID,
        encoded
      );

      setMessage("Command sent: " + command);
    } catch (error) {
      setMessage("Command failed");

      Alert.alert(
        "Command error",
        String(error)
      );
    }
  };

  // ----------------------------
  // Disconnect
  // ----------------------------
  const disconnect = async () => {
    if (!connectedDevice) {
      return;
    }

    try {
      subscriptionRef.current?.remove();
      subscriptionRef.current = null;

      await manager.cancelDeviceConnection(
        connectedDevice.id
      );

      setConnectedDevice(null);
      setTelemetry(null);
      setMessage("Disconnected");
    } catch (error) {
      setMessage("Disconnect error");
      console.error(error);
    }
  };

  // ----------------------------
  // User interface
  // ----------------------------
  return (
    <ScrollView contentContainerStyle={styles.container}>
      <Text style={styles.title}>Stride Secure</Text>

      <Text style={styles.subtitle}>
        Arduino BLE Control
      </Text>

      <View style={styles.card}>
        <Text style={styles.sectionTitle}>
          Device Connection
        </Text>

        <Text style={styles.status}>
          Status: {message}
        </Text>

        {!connectedDevice ? (
          <>
            <Button
              title={
                isScanning
                  ? "Scanning..."
                  : "Scan for Devices"
              }
              onPress={scanForDevices}
              disabled={isScanning || isConnecting}
            />

            {devices.map((device) => (
              <View key={device.id} style={styles.device}>
                <Text>
                  {device.name || "Unknown device"}
                </Text>

                <Text style={styles.deviceId}>
                  {device.id}
                </Text>

                <Button
                  title="Connect"
                  onPress={() => connectToDevice(device)}
                  disabled={isConnecting}
                />
              </View>
            ))}
          </>
        ) : (
          <>
            <Text style={styles.connected}>
              Connected: {connectedDevice.name || "Arduino"}
            </Text>

            <Button
              title="Disconnect"
              color="red"
              onPress={disconnect}
            />
          </>
        )}
      </View>

      {connectedDevice && (
        <>
          <View style={styles.card}>
            <Text style={styles.sectionTitle}>
              Device Controls
            </Text>

            <Button
              title="Turn LED ON"
              onPress={() => sendCommand("LED_ON")}
            />

            <View style={styles.spacing} />

            <Button
              title="Turn LED OFF"
              onPress={() => sendCommand("LED_OFF")}
            />
          </View>

          <View style={styles.card}>
            <Text style={styles.sectionTitle}>
              Sensor Data
            </Text>

            <Text style={styles.sensor}>
              Distance:{" "}
              {telemetry?.distance_cm === undefined
                ? "--"
                : telemetry.distance_cm < 0
                ? "No object detected"
                : telemetry.distance_cm.toFixed(1) + " cm"}
            </Text>

            <Text style={styles.sensor}>
              Distance sensor raw:{" "}
              {telemetry?.dist_raw ?? "--"}
            </Text>

            <Text style={styles.sensor}>
              Tilt sensor raw:{" "}
              {telemetry?.tilt_raw ?? "--"}
            </Text>
          </View>
        </>
      )}
    </ScrollView>
  );
}

// ----------------------------
// Styling
// ----------------------------
const styles = StyleSheet.create({
  container: {
    flexGrow: 1,
    padding: 24,
    paddingTop: 60,
    backgroundColor: "#f4f6f8",
  },

  title: {
    fontSize: 30,
    fontWeight: "bold",
    textAlign: "center",
  },

  subtitle: {
    fontSize: 16,
    color: "#666",
    textAlign: "center",
    marginBottom: 25,
  },

  card: {
    backgroundColor: "white",
    padding: 20,
    borderRadius: 12,
    marginBottom: 20,
    gap: 12,
  },

  sectionTitle: {
    fontSize: 20,
    fontWeight: "bold",
  },

  status: {
    fontSize: 15,
    color: "#444",
  },

  connected: {
    color: "green",
    fontWeight: "bold",
  },

  device: {
    padding: 12,
    backgroundColor: "#f1f3f5",
    borderRadius: 8,
    gap: 8,
  },

  deviceId: {
    fontSize: 11,
    color: "#777",
  },

  sensor: {
    fontSize: 17,
    paddingVertical: 5,
  },

  spacing: {
    height: 8,
  },
});
