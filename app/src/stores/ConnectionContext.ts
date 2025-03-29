import { LucasConnection } from "@/lib/LucasConnection";
import { createContext } from "react";

const ConnectionContext = createContext<LucasConnection>(new LucasConnection());

export default ConnectionContext;
