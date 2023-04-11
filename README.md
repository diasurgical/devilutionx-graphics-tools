# DevilutionX Graphics Tools

## Build & install

```bash
cmake -S. -Bbuild-rel -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
cmake --build build-rel -j $(getconf _NPROCESSORS_ONLN)
sudo cmake --install build-rel --component Binaries
```

The above only install the binaries. If you want to install the libraries and headers as well, run:

```bash
sudo cmake --install build-rel
```

## cel2clx

Converts CEL files to CLX. Run `cel2clx --help` for more information.

## cl22clx

Converts CL2 files to CLX. Run `cl22clx --help` for more information.

## clx2pcx

Converts CLX files to PCX. Run `clx2pcx --help` for more information.

## pcx2clx

Converts PCX files to CLX. Run `pcx2clx --help` for more information.
