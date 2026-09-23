# Getting Started with GaphopLang

## Install / Build the Compiler

```bash
cd GaphopLang/compiler
make
# produces ./ghlc  (copy to a PATH location if desired)
```

## Hello World

```bash
./ghlc new Hello
cd Hello
../compiler/ghlc build
./bin/Hello
```

Or single file:

```bash
echo 'fn main() -> int { println("Hi"); return 0; }' > hi.ghl
ghlc build hi.ghl -o hi
./hi
```

## Philosophy reminders

- Explicit types
- Strict errors (no silent failures)
- Native performance (no GC required)
