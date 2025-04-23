{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    esp-dev = {
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
      esp-dev,
      android-nixpkgs,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        overlays = [ (import "${esp-dev}/overlay.nix") ];
        pkgs = import nixpkgs { inherit system overlays; };

        esp32-toolchain = pkgs.esp-idf-esp32.override {
          toolsToInclude = [
            "esp-clang"
            "xtensa-esp-elf"
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
            # firmware
            esp32-toolchain

            # app
            nodejs_22
            vtsls
            prettierd
            vscode-langservers-extracted
            android-sdk

            # shared
            protobuf
            buf
          ];
        };
      }
    );
}
