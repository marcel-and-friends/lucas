/// <reference types="vitest" />

import path from "node:path";
import tailwindcss from "@tailwindcss/vite";
import legacy from "@vitejs/plugin-legacy";
import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";
import viteProtoGenPlugin from "./plugins/vite-proto-gen";

// https://vitejs.dev/config/
export default defineConfig({
  server: {
    watch: {
      followSymlinks: true,
    },
  },
  plugins: [
    react(),
    legacy(),
    tailwindcss(),
    viteProtoGenPlugin({
      protoDir: path.resolve("../shared"),
      outDir: path.resolve("generated"),
    }),
  ],
  resolve: {
    alias: {
      "@": path.resolve("src"),
      "#": path.resolve("generated"),
    },
  },
  test: {
    globals: true,
    environment: "jsdom",
    setupFiles: "./src/setupTests.ts",
  },
});
