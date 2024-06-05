{ stdenv, fetchFromGitHub, pkg-config, meson, ninja, perl }:

stdenv.mkDerivation rec {
  pname = "libvfn";
  version = "4.0.1";

  src = fetchFromGitHub {
    owner = "SamsungDS";
    repo = "libvfn";
    rev = "v${version}";
    hash = "sha256-/XOSD4S/Jr8ohltahMTiFF18njt341Uq2ZZjgV+y9Pg=";
  };

  patches = [ ./trace_pl_pathfix.patch ];

  mesonFlags = [
    "-Ddocs=disabled"
    "-Dlibnvme=disabled"
    "-Dprofiling=false"
  ];

  nativeBuildInputs = [ meson ninja pkg-config perl ];
}
