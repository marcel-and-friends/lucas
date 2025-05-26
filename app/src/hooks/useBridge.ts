import { useContext } from "react";
import BridgeContext from "@/stores/BridgeContext";

export default function useBridge() {
  const bridge = useContext(BridgeContext);
  if (!bridge)
    throw new Error(
      "Programming error - using bridge without context provider",
    );
  return bridge;
}
