import { createRoot } from "react-dom/client";
import ThemeProvider from "@/components/ThemeProvider";
import App from "./App";

const container = document.getElementById("root");
// biome-ignore lint/style/noNonNullAssertion: The root element is declared in `index.html`
const root = createRoot(container!);
root.render(
  <ThemeProvider>
    <App />
  </ThemeProvider>,
);
