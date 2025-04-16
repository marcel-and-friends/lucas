import { LucasConnection } from "@/lib/LucasConnection";
import { createContext } from "react";

const ConnectionContext = createContext<LucasConnection | undefined>(undefined);

export default ConnectionContext;
