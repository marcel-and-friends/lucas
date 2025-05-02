import { FirmwareEvent } from "#/proto/firmware/FirmwareEvent";
import useBridge from "@/hooks/useBridge";
import { useEffect } from "react";

export default function useFirmwareEvent<C extends keyof FirmwareEvent>(
  caseStr: C,
  callback: (data: NonNullable<FirmwareEvent[C]>) => void,
) {
  const bridge = useBridge();
  useEffect(() => {
    const id = bridge.subscribe(caseStr, callback);
    return () => {
      bridge.unsubscribe(id);
    };
  }, [caseStr, callback, bridge]);
}
