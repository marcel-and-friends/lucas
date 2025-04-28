import { AppCommand } from "#/proto/app/AppCommand";
import { FirmwareEvent } from "#/proto/firmware/FirmwareEvent";
import {
  BleClient,
  BleDevice,
  numberToUUID,
} from "@capacitor-community/bluetooth-le";

export const SERVICE_UUID = numberToUUID(0xabf0);
export const SPP_UUID = "5019aa43-e7f5-95be-9a44-8dd43473c349";
export const STATUS_UUID = "44642abc-6057-f595-1246-fe23279bcf52";

export class LucasConnection {
  private subscriptions: {
    [C in keyof FirmwareEvent]?: Array<
      (event: NonNullable<FirmwareEvent[C]>) => void
    >;
  } = {};

  private readonly device: BleDevice;

  constructor(device: BleDevice) {
    this.device = device;

    void BleClient.startNotifications(
      this.device.deviceId,
      SERVICE_UUID,
      SPP_UUID,
      (data) => {
        const message = FirmwareEvent.decode(new Uint8Array(data.buffer));
        for (const [field, value] of Object.entries(message)) {
          const typed_field = field as keyof FirmwareEvent;
          const typed_value = value as FirmwareEvent[typeof typed_field];
          if (typed_value !== undefined) {
            this.subscriptions[typed_field]?.forEach((cb) =>
              // @ts-expect-error: I don't think there's a way to make tsc happy about this but it's sound.
              cb(typed_value),
            );
            break;
          }
        }
      },
    );
  }

  disconnect() {
    return BleClient.disconnect(this.device.deviceId);
  }

  subscribe<C extends keyof FirmwareEvent>(
    caseStr: C,
    callback: (event: NonNullable<FirmwareEvent[C]>) => void,
  ): SubscriptionId {
    if (!this.subscriptions[caseStr]) this.subscriptions[caseStr] = [];
    this.subscriptions[caseStr].push(callback);
    return { caseStr, index: this.subscriptions[caseStr].length - 1 };
  }

  unsubscribe(id: SubscriptionId) {
    this.subscriptions[id.caseStr]?.splice(id.index, 1);
  }

  sendCommand(command: AppCommand) {
    const bytes = AppCommand.encode(command).finish();
    return BleClient.write(
      this.device.deviceId,
      SERVICE_UUID,
      SPP_UUID,
      new DataView(bytes.buffer),
    );
  }
}

interface SubscriptionId {
  caseStr: keyof FirmwareEvent;
  index: number;
}
