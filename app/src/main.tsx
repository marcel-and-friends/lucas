import ThemeProvider from "@/components/ThemeProvider";
import { createRoot } from "react-dom/client";
import App from "./App";

const container = document.getElementById("root");
const root = createRoot(container!);
root.render(
  <ThemeProvider>
    <App />
  </ThemeProvider>,
);
