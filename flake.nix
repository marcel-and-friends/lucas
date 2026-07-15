{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      inputs.flake-utils.follows = "flake-utils";
    };
  };

  outputs =
    inputs:
    inputs.flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import inputs.nixpkgs {
          inherit system;
          config = {
            allowUnfree = true;
            android_sdk.accept_license = true;
          };
        };

        esp-idf = inputs.nixpkgs-esp-dev.packages.${system}.esp-idf-full.override {
          toolsToInclude = [
            "esp-clang"
            "esp-rom-elfs"
            "xtensa-esp-elf"
          ];

          # nanopb requires some extra packages in the python environment
          extraPythonPackages = p: [
            p.grpcio-tools
            p.protobuf
          ];
        };

        buildToolsVersion = "36.0.0";

        android-sdk =
          (pkgs.androidenv.composeAndroidPackages {
            buildToolsVersions = [
              buildToolsVersion
              "35.0.0" # AGP 8.13's default build tools version
            ];
            platformVersions = [ "36" ];
          }).androidsdk;
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            # App
            android-sdk
            jdk
            nodejs_24

            # Firmware
            esp-idf

            # Shared
            buf
            protobuf
          ];

          env.ANDROID_SDK_ROOT = "${android-sdk}/libexec/android-sdk";

          shellHook = ''
            export PATH="$ANDROID_SDK_ROOT/build-tools/${buildToolsVersion}:$PATH"
          '';
        };
      }
    );
}
