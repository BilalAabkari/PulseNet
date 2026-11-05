# OpenSSL (Windows x64, static)

Prebuilt OpenSSL binaries used by PulseNet's networking module.

## Build info

| Item | Value |
|------|-------|
| OpenSSL version | 4.1.0 |
| Source | https://github.com/openssl/openssl/releases/download/openssl-4.1.0/openssl-4.1.0.tar.gz |
| Platform / target | Windows x64, `VC-WIN64A` |
| Toolchain | `Visual Studio 2022 (MSVC 14.x), Strawberry Perl, NASM` |
| Linkage | Static (`no-shared`) |
| C runtime | Dynamic CRT (`/MD`), OpenSSL's default |
| Build configuration | Release |
| Build date | `05/11/2026` |

## Configure command

```
perl Configure VC-WIN64A no-shared no-legacy no-docs --prefix="<install path>" --openssldir="<install path>/ssl"
nmake
nmake install_dev
```

Options used:

- `no-shared`: static libraries only, no DLLs.
- `no-legacy`: the legacy provider (MD4, RC4, DES variants, etc.) is not built.
- `no-docs`: HTML and man pages are not generated.

The `default` and `base` providers are compiled into `libcrypto`. The FIPS provider is not built.

## Contents

```
windows-x64/
  include/openssl/   Public headers
  lib/
    libcrypto.lib    Static libcrypto
    libssl.lib       Static libssl
  README.md          This file
```

Not committed on purpose: `bin/` (`openssl.exe`, `.pdb`), `ssl/` (config, certs, demo scripts such as `CA.pl`), and `lib/ossl-modules/`. None of these are needed to build or link against OpenSSL.

## Linking

Link against `libssl.lib` and `libcrypto.lib`, plus these Windows system libraries (required by static OpenSSL):

```
ws2_32.lib  crypt32.lib  advapi32.lib  user32.lib
```

Your project must use the dynamic CRT (`/MD` for Release, `/MDd` for Debug) to match this build. Mixing runtimes causes linker or runtime errors. Debug builds of this project should be linked against a Release OpenSSL only if the CRT settings are compatible; otherwise rebuild OpenSSL with `--debug`.

## Runtime notes

- `OPENSSLDIR` is compiled in as the path used at build time, which does not exist on other machines. This is harmless unless you rely on a default `openssl.cnf`. If you need one, load it explicitly (`OPENSSL_conf` env var or `OPENSSL_INIT_set_config_filename`).
- No CA bundle is shipped. For TLS certificate verification, supply trust roots yourself (Windows certificate store, or a `cacert.pem` loaded with `SSL_CTX_load_verify_locations`).
- Legacy algorithms are unavailable. Avoid old formats such as RC2/3DES-encrypted PKCS#12, or rebuild without `no-legacy` as a shared build.


## Updating

OpenSSL receives regular security fixes, and a static build means they only reach PulseNet when these binaries are rebuilt.

1. Download the new release source.
2. Rebuild with the configure command above, using a clean build directory.
3. Replace `include/` and `lib/`, then update the version, date, and checksums in this file.

Security advisories: https://openssl-library.org/news/vulnerabilities/

## License

OpenSSL 4.x is licensed under the Apache License 2.0. Keep the license text with redistributed binaries: https://github.com/openssl/openssl/blob/master/LICENSE.txt