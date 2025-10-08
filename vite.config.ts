import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import path from "path";

const clientRoot = path.resolve(__dirname, "src/client");

export default defineConfig({
  root: clientRoot,
  plugins: [react()],
  build: {
    outDir: path.resolve(__dirname, "public"),
    emptyOutDir: true,
    sourcemap: true
  },
  server: {
    port: 5173,
    proxy: {
      "/api": {
        target: "http://localhost:3000",
        changeOrigin: true
      },
      "/ws": {
        target: "ws://localhost:3000",
        ws: true
      }
    }
  }
});
