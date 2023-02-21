{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  name = "linux-kernel-build";
  nativeBuildInputs = with pkgs; [
    autoconf automake libtool gettext pkg-config
    libuuid # codegen tool uses libuuid
  ];
  buildInputs = with pkgs; [ gnum4 m4 readline icu inih liburcu ];
  propagatedBuildInputs = with pkgs; [ libuuid ]; # Dev headers include <uuid/uuid.h>

  EDITOR = "nvim";
}

