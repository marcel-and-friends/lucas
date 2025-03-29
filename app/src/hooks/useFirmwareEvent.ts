import { CaseMap } from "@/lib/LucasConnection";
import { FirmwareEvent } from "#/proto/firmware/FirmwareEvent";
import ConnectionContext from "@/stores/ConnectionContext";
import { useContext, useEffect } from "react";

export default function useFirmwareEvent<
  C extends keyof CaseMap<FirmwareEvent>,
>(caseStr: C, callback: (data: CaseMap<FirmwareEvent>[C]) => void) {
  const connection = useContext(ConnectionContext);
  useEffect(() => {
    const id = connection.subscribe(caseStr, callback);
    return () => {
      connection.unsubscribe(id);
    };
  }, [caseStr, callback, connection]);
}
