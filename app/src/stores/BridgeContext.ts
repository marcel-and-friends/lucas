import { Bridge } from "@/lib/Bridge";
import { createContext } from "react";

const BridgeContext = createContext<Bridge | undefined>(undefined);

export default BridgeContext;
