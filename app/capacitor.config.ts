import type { CapacitorConfig } from "@capacitor/cli";

const config: CapacitorConfig = {
  appId: "com.marcelandfriends.lucas_app",
  appName: "Lucas Coffe Machine",
  webDir: "dist",
  android: {
    buildOptions: {
      keystorePath: "keystore",
      keystorePassword: "lucas-cafe",
      keystoreAlias: "lucas-key",
      keystoreAliasPassword: "lucas-cafe",
      releaseType: "APK",
    },
  },
};

export default config;
