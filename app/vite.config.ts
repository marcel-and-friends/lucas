/// <reference types="vitest" />

import legacy from "@vitejs/plugin-legacy";
import react from "@vitejs/plugin-react";
import path from "path";
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
