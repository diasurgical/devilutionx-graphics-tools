# DevilutionX Graphics Tools

## Build & install

```bash
cmake -S. -Bbuild-rel -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j $(getconf _NPROCESSORS_ONLN)
sudo cmake --install build-rel
```

## cel2clx

Converts CEL files to CLX. Run `cel2clx --help` for more information.

## cl22clx

Converts CL2 files to CLX. Run `cl22clx --help` for more information.

## pcx2clx

Converts PCX files to CLX. Run `pcx2clx --help` for more information.
