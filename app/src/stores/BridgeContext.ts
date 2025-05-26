import { createContext } from "react";
import type { Bridge } from "@/lib/Bridge";

const BridgeContext = createContext<Bridge | undefined>(undefined);

export default BridgeContext;
