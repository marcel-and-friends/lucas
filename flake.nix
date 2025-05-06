{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-esp-dev = {
      url = "github:iniw/nixpkgs-esp-dev";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    android-nixpkgs = {
      url = "github:tadfisher/android-nixpkgs/stable";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      nixpkgs-esp-dev,
      android-nixpkgs,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        overlays = [ (import "${nixpkgs-esp-dev}/overlay.nix") ];

        pkgs = import nixpkgs { inherit system overlays; };

        esp-idf = pkgs.esp-idf-full.override {
          extraPythonPackages = (
            # nanopb requires some extra packages in the python environment
            pythonPkgs: with pythonPkgs; [
              grpcio-tools
              protobuf
            ]
          );
          toolsToInclude = [
            "esp-clang"
            "xtensa-esp-elf"
            # Required until https://github.com/espressif/esp-idf/commit/b64ddb18939d06426607a04816e6de881524e87f lands in an ESP-IDF release
            # See https://github.com/espressif/esp-idf/issues/15035
            "esp-rom-elfs"
          ];
        };

        android-sdk = android-nixpkgs.sdk.${system} (
          sdkPkgs: with sdkPkgs; [
            build-tools-36-0-0 # Used for the newer version of zipalign that supports the "-P 16" flag
            build-tools-34-0-0
            cmdline-tools-latest
            platform-tools
            platforms-android-35
            platforms-android-34
          ]
        );
      in
      {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            # Firmware
            esp-idf
            # App
            android-sdk
            jdk
            nodejs_22
            prettierd
            vscode-langservers-extracted
            vtsls
            # Shared
            buf
            protobuf
          ];

          shellHook = ''
            export JAVA_HOME=${pkgs.jdk.home}
          '';
        };

      }
    );
}
