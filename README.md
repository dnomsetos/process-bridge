# process-bridge

Инструмент для снятия снепшота нативного процесса Linux в точке останова
(регистры + карта памяти) с последующим восстановлением этого состояния
внутри эмулятора Qiling. Состоит из двух частей:

- `native/` — C-утилита (`process-bridge-x64` / `process-bridge-x86`),
  которая форкает целевой процесс, ставит breakpoint по смещению от базы
  образа, дожидается его срабатывания и дампит снапшот;
- `loader/process_bridge/` — Python-обвязка на Qiling, которая читает
  дамп и продолжает исполнение уже внутри эмулятора.

## Требования

- CMake ≥ 3.20 и gcc с поддержкой `-m64` и `-m32`
- Python ≥ 3.10 и пакет `qiling` (для Python-части)
- Linux — нативная часть использует `ptrace(2)` и заголовки из `linux/`,
  под другие платформы не собирается (`platform.h` явно роняет сборку)

## Стандартная сборка

`CMakeLists.txt` собирает **сразу оба** таргета — 64-битный и 32-битный
(`add_arch_targets(x64 -m64)` и `add_arch_targets(x86 -m32)`), поэтому
даже обычная сборка требует, чтобы компилятор умел собирать 32-битный код.

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j"$(nproc)"
```

После сборки в `build/` появятся четыре артефакта:

- `process-bridge-x64`, `libprocess-bridge-lib-x64.a`
- `process-bridge-x86`, `libprocess-bridge-lib-x86.a`

### Пример сборки x86-таргета на x86_64-хосте

Раз хост x86_64, для `-m32` нужны 32-битные мультилиб-версии libc и
заголовков — без них сборка `process-bridge-x86` упадёт на этапе линковки.

```bash
# Debian/Ubuntu
sudo apt update
sudo apt install gcc-multilib g++-multilib

# затем обычная сборка соберёт оба таргета сразу
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j"$(nproc)"
```

## Запуск

```bash
./build/process-bridge-x64 <hex-offset> <path-to-target-binary> [target-args...]
```

- `hex-offset` — смещение breakpoint'а от базы загруженного образа (hex, без `0x`)
- `path-to-target-binary` — путь к трассируемому исполняемому файлу
- `target-args...` — опциональные аргументы, которые получит целевой процесс

Утилита форкает и запускает `target-binary`, ставит breakpoint по адресу
`image_base + hex-offset`, доводит процесс до него и пишет снепшот в
каталог `ql_snapshot` в текущей директории.

Для 32-битного трассируемого процесса используется `process-bridge-x86`
по тому же принципу.

## Python/Qiling-часть

```bash
pip install .
process-bridge-x64   # запустить собранный бинарник x64 через обёртку
process-bridge-x86   # то же для x86
```
