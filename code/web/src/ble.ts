
// src/ble.ts

// These UUIDs must match the Arduino firmware.
export const SERVICE_UUID =
  "12345678-1234-5678-1234-56789abcdef0";

export const COMMAND_UUID =
  "12345678-1234-5678-1234-56789abcdef1";

export const TELEMETRY_UUID =
  "12345678-1234-5678-1234-56789abcdef2";

// Sensor data received from Arduino.
export interface Telemetry {
  distance_cm: number;
  average_cm: number;
  tilt_status: string;
  over40_count: number;
}

let device: BluetoothDevice | null = null;
let commandCharacteristic:
  | BluetoothRemoteGATTCharacteristic
  | null = null;

let telemetryCharacteristic:
  | BluetoothRemoteGATTCharacteristic
  | null = null;

const decoder = new TextDecoder();

// Check browser support.
export function isBluetoothSupported(): boolean {
  return (
    typeof navigator !== "undefined" &&
    "bluetooth" in navigator
  );
}

// Connect to the Arduino.
export async function connectToArduino(
  onTelemetry: (data: Telemetry) => void,
  onDisconnect: () => void
): Promise<string> {
  if (!isBluetoothSupported()) {
    throw new Error(
      "Web Bluetooth is not supported in this browser."
    );
  }

  // Open the browser's Bluetooth device chooser.
  device = await navigator.bluetooth.requestDevice({
    filters: [{ services: [SERVICE_UUID] }],
  });

  device.addEventListener(
    "gattserverdisconnected",
    onDisconnect
  );

  if (!device.gatt) {
    throw new Error("Bluetooth GATT is unavailable.");
  }

  // Connect and discover services.
  const server = await device.gatt.connect();

  const service = await server.getPrimaryService(
    SERVICE_UUID
  );

  commandCharacteristic =
    await service.getCharacteristic(COMMAND_UUID);

  telemetryCharacteristic =
    await service.getCharacteristic(TELEMETRY_UUID);

  // Subscribe to sensor notifications.
  await telemetryCharacteristic.startNotifications();

  telemetryCharacteristic.addEventListener(
    "characteristicvaluechanged",
    (event) => {
      const characteristic =
        event.target as BluetoothRemoteGATTCharacteristic;

      const value = characteristic.value;

      if (!value) return;

      try {
        const message = decoder.decode(value);
        const data = JSON.parse(message) as Telemetry;

        if (
          typeof data.distance_cm !== "number" ||
          typeof data.average_cm !== "number" ||
          typeof data.tilt_status !== "string" ||
          typeof data.over40_count !== "number"
        ) {
          throw new Error("Invalid telemetry format.");
        }

        onTelemetry(data);
      } catch (error) {
        console.error("Telemetry parsing error:", error);
      }
    }
  );

  return device.name || "Arduino";
}

// Send a command to Arduino.
export async function sendCommand(
  command: string
): Promise<void> {
  if (!commandCharacteristic) {
    throw new Error("Arduino is not connected.");
  }

  const encoded = new TextEncoder().encode(command);

  await commandCharacteristic.writeValueWithResponse(
    encoded
  );
}

// Disconnect from Arduino.
export function disconnectFromArduino(): void {
  if (device?.gatt?.connected) {
    device.gatt.disconnect();
  }

  commandCharacteristic = null;
  telemetryCharacteristic = null;
  device = null;
}
