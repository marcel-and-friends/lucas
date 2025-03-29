import { FirmwareEvent } from "#/proto/firmware/FirmwareEvent";
import {
  BleClient,
  BleDevice,
  numberToUUID,
} from "@capacitor-community/bluetooth-le";

export const SERVICE_UUID = numberToUUID(0xabf0);
export const SPP_UUID = "5019aa43-e7f5-95be-9a44-8dd43473c349";
export const STATUS_UUID = "44642abc-6057-f595-1246-fe23279bcf52";

type EventOf<T extends { event?: unknown }> = NonNullable<T["event"]>;
type CaseOf<T extends { event?: { $case: string } }> = EventOf<T>["$case"];
type CaseTypeOf<T extends { event?: { $case: string } }, C extends CaseOf<T>> =
  Extract<EventOf<T>, { $case: C }> extends { [K in C]: infer U } ? U : never;
export type CaseMap<T extends { event?: { $case: string } }> = {
  [C in CaseOf<T>]: CaseTypeOf<T, C>;
};

interface SubscriptionId<T extends { event?: { $case: string } }> {
  caseStr: keyof CaseMap<T>;
  index: number;
}

export class LucasConnection {
  private subscriptions: {
    [C in keyof CaseMap<FirmwareEvent>]?: Array<
      (event: CaseMap<FirmwareEvent>[C]) => void
    >;
  } = {};

  readonly device?: BleDevice;

  constructor(device?: BleDevice) {
    this.device = device;
    if (!this.device) return;

    void BleClient.startNotifications(
      this.device.deviceId,
      SERVICE_UUID,
      SPP_UUID,
      (data) => {
        const message = FirmwareEvent.decode(new Uint8Array(data.buffer));
        const event = message.event;
        if (!event) {
          console.error("No event field in message");
          return;
        }

        const caseStr = event.$case;
        const eventData = event[caseStr];

        this.subscriptions[caseStr]?.forEach((cb) => cb(eventData));
      },
    );
  }

  subscribe<C extends keyof CaseMap<FirmwareEvent>>(
    caseStr: C,
    callback: (event: CaseMap<FirmwareEvent>[C]) => void,
  ): SubscriptionId<FirmwareEvent> {
    if (!this.subscriptions[caseStr]) this.subscriptions[caseStr] = [];
    this.subscriptions[caseStr].push(callback);
    return { caseStr, index: this.subscriptions[caseStr].length - 1 };
  }

  unsubscribe(id: SubscriptionId<FirmwareEvent>) {
    this.subscriptions[id.caseStr]?.splice(id.index, 1);
  }
}
