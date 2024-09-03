{ pkgs ? (let lock = builtins.fromJSON (builtins.readFile ./flake.lock);
in import (builtins.fetchTarball {
  url =
    "https://github.com/NixOS/nixpkgs/archive/${lock.nodes.nixpkgs.locked.rev}.tar.gz";
  sha256 = lock.nodes.nixpkgs.locked.narHash;
}) { }) }:

let
  dependencies = with pkgs; [
    gcc
    mesa
    glfw
    xorg.libX11
    xorg.libXrandr
    xorg.libXi
    raylib
    glibc
  ];

in pkgs.mkShell {
  name = "c devshell";
  buildInputs = with pkgs; [
    gnumake42
    gdb
    valgrind
  ];

  nativeBuildInputs = with pkgs; [
  ];

  packages = dependencies;
}