import { useEffect } from "react";
import type { FirmwareEvent } from "#/proto/firmware/FirmwareEvent";
import useBridge from "@/hooks/useBridge";

export default function useFirmwareEvent<C extends keyof FirmwareEvent>(
  caseStr: C,
  callback: (data: NonNullable<FirmwareEvent[C]>) => void,
) {
  const bridge = useBridge();
  useEffect(() => {
    bridge.subscribe(caseStr, callback);
    return () => {
      bridge.unsubscribe(caseStr);
    };
  }, [caseStr, callback, bridge]);
}
