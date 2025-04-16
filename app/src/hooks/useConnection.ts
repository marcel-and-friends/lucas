import ConnectionContext from "@/stores/ConnectionContext";
import { useContext } from "react";

export default function useConnection() {
  const connection = useContext(ConnectionContext);
  if (!connection)
    throw new Error(
      "Programming error - using connection without context provider",
    );
  return connection;
}
