import type { CapacitorConfig } from "@capacitor/cli";

const config: CapacitorConfig = {
  appId: "com.marcelandfriends.lucas_app",
  appName: "Lucas Coffe Machine",
  webDir: "dist",
  android: {
    buildOptions: {
      keystorePath: "keystore",
      keystorePassword: "lucas!5547",
      keystoreAlias: "lucas-key",
      keystoreAliasPassword: "lucas!5547",
      releaseType: "APK",
      signingType: "apksigner",
    },
    adjustMarginsForEdgeToEdge: "auto",
  },
  plugins: {
    Keyboard: {
      resizeOnFullScreen: true,
    },
  },
};

export default config;
