{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      inputs.nixpkgs.follows = "nixpkgs";
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

        android-sdk =
          (pkgs.androidenv.composeAndroidPackages {
            buildToolsVersions = [
              "36.0.0" # Used for the newer version of zipalign that supports the "-P 16" flag
              "34.0.0"
            ];
            platformVersions = [
              "35"
              "34"
            ];
          }).androidsdk;
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            # App
            android-sdk
            jdk
            nodejs_22

            # Firmware
            esp-idf

            # Shared
            buf
            protobuf
          ];

          ANDROID_SDK_ROOT = "${android-sdk}/libexec/android-sdk";
        };
      }
    );
}
