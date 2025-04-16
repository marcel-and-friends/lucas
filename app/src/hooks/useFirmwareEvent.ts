import { FirmwareEvent } from "#/proto/firmware/FirmwareEvent";
import useConnection from "@/hooks/useConnection";
import { useEffect } from "react";

export default function useFirmwareEvent<C extends keyof FirmwareEvent>(
  caseStr: C,
  callback: (data: NonNullable<FirmwareEvent[C]>) => void,
) {
  const connection = useConnection();
  useEffect(() => {
    const id = connection.subscribe(caseStr, callback);
    return () => {
      connection.unsubscribe(id);
    };
  }, [caseStr, callback, connection]);
}
