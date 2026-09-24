# Installing & Building GaphopLang on MSYS2 UCRT64

## 1. Install MSYS2

Download from: https://www.msys2.org/

After install, open the **UCRT64** terminal (not MSYS or MINGW64).

## 2. Install toolchain

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make make git
```

Verify:

```bash
gcc --version
# should show x86_64-w64-mingw32 / UCRT
```

## 3. Build the GHL compiler

```bash
cd GaphopLang/compiler
make
```

This produces:

```
ghlc.exe
```

Optional install into UCRT64 prefix:

```bash
make install
# installs to /ucrt64/bin/ghlc.exe
```

## 4. Create & build a GHL project

```bash
./ghlc.exe new Hello
cd Hello
../ghlc.exe build
./bin/Hello.exe
```

Or:

```bash
../ghlc.exe run
```

## 5. Notes

- On MSYS2 UCRT64 the compiler automatically targets **Windows x64** and produces `.exe`.
- The temporary backend still emits C and invokes `gcc` (the UCRT64 one).
- Paths work with both `/` and `\` under MSYS2.
- Use the **UCRT64** shell so that `gcc` is the UCRT runtime toolchain (recommended for modern Windows).

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `gcc: command not found` | Open **UCRT64** terminal, not MSYS |
| `make: command not found` | `pacman -S make` |
| Permission / antivirus | Allow `ghlc.exe` and generated `.exe` in Windows Defender |
| Old MINGW64 instead of UCRT64 | Prefer UCRT64 for better Windows 10/11 compatibility |
