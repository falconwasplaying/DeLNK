# DeArrow

A simple lightweight portable Windows utility designed to toggle shortcut overlay arrows on and off, made to be as optimised as can be and is completely native to Windows, containing _zero_ external dependencies, thus only taking 12 KB of storage space, also able to run with a very low memory footprint, using 1.2 MB of ram, your numbers may vary.

## System Requirements

* **Operating System**: Windows 7, Windows 8.1, Windows 10, or Windows 11.
* **Architecture**: 64-bit (x86-64) and 32-bit (x86-32).
* **Permissions**: Administrator privileges are _required_ for modifying system-wide icon configurations which requires writing to `HKEY_LOCAL_MACHINE`.

## How to Use

* **Two Formats Available**:
  * **`dearrow-x64.exe / dearrow-x32.exe`**: A simple graphical interface.
  * **`dearrow-cli-x64.exe / dearrow-cli-x32.exe`**: An interactive command-line tool.

### Graphical User Interface (`dearrow-x64.exe / dearrow-x32.exe`)

1. Run `dearrow-x64.exe` or `dearrow-x32.exe`.
2. Accept the User Account Control (UAC) prompt to run as Administrator.
3. Click **Remove Arrows** to hide them, or **Restore Default** to bring them back.
4. Click **Yes** when prompted to restart Windows Explorer to apply changes immediately, or **No** to exit.

### Command-Line Interface (`dearrow-cli-x64.exe / dearrow-cli-x32.exe`)

1. Run `dearrow-cli-x64.exe` or `dearrow-cli-x32.exe`.
2. Accept the UAC prompt to run as Administrator.
3. Type `remove` (or `rm`) or `restore` (or `rs`) (case-insensitive) and press **Enter**.
4. Type `y` to restart Windows Explorer now, or `n` to exit.

### Command-Line Flags

Both the GUI and CLI versions support command-line arguments for quick execution or scripted automation (run an elevated Command Prompt or PowerShell terminal):

```bash
# Hide arrows and automatically restart Explorer immediately
dearrow-x64.exe -rm -y
dearrow-cli-x64.exe -rm -y

# Restore default arrows and automatically restart Explorer immediately
dearrow-x64.exe -rs -y
dearrow-cli-x64.exe -rs -y

# Apply changes without restarting Explorer
dearrow-x64.exe -rm -n
dearrow-cli-x64.exe -rs -n
```

| Flag | Description |
| :--- | :--- |
| `-rm`, `--remove`, `/rm` | Hide shortcut overlay arrows |
| `-rs`, `--restore`, `/rs` | Restore default shortcut overlay arrows |
| `-y`, `--yes`, `/y` | Automatically restart Windows Explorer without prompting |
| `-n`, `--no`, `/n` | Apply changes without restarting Windows Explorer |
| `-h`, `--help`, `/?` | Display usage and available flags |

> [!Note]
> If you choose not to restart Windows Explorer immediately, changes will apply on next boot or whenever Windows Explorer is restarted.

---

## How to Build the GUI App Yourself

You'll need MinGW-w64/UCRT64 and MinGW-w32 installed (via [MSYS2](https://www.msys2.org/) is easiest).

### 64-bit gui build

Open the **MSYS2 UCRT64** (or **MinGW-w64**) terminal, and go to your project directory:

```bash
cd /c/users/username/documents/dearrow
```

> replace "/c/users/username/documents/dearrow" with your project directory

Now run:

```bash
mkdir -p bin
windres -i res/resources-x64.rc -I res -O coff -o bin/rsc-x64.o
g++ src/main.cpp bin/rsc-x64.o -o bin/dearrow-x64.exe -mwindows -lgdi32 -ladvapi32 -lshell32 -nostartfiles -e WinMainCRTStartup -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections "-Wl,--gc-sections"
```

### 32-bit gui build

Open the **MinGW-w32** terminal, and go to your project directory:

```bash
cd /c/users/username/documents/dearrow
```

> replace "/c/users/username/documents/dearrow" with your project directory

Now run:

```bash
mkdir -p bin
windres -i res/resources-x32.rc -I res -O coff -o bin/rsc-x32.o
g++ src/main.cpp bin/rsc-x32.o -o bin/dearrow-x32.exe -mwindows -lgdi32 -ladvapi32 -lshell32 -nostartfiles -e _WinMainCRTStartup@0 -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -static-libgcc -Wl,--gc-sections
```

> [!Tip]
> Each MSYS2 shell (UCRT64/MinGW-w64/MinGW-w32) automatically points `g++` and `windres` at the matching architecture — you don't need to type prefixed binary names or full paths, just make sure you're in the right shell.

## How to build the CLI app yourself

### 64-bit cli build

Open the **MSYS2 UCRT64** (or **MinGW-w64**) terminal, and go to your project directory:

```bash
cd /c/users/username/documents/dearrow
```

> [!Important]
> replace "/c/users/username/documents/dearrow" with your project directory

Now run:

```bash
mkdir -p bin
windres -i res/resources-cli-x64.rc -I res -O coff -o bin/rsc-cli-x64.o
g++ src/cli.cpp bin/rsc-cli-x64.o -o bin/dearrow-cli-x64.exe -mconsole -ladvapi32 -lshell32 -nostartfiles -e mainCRTStartup -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -Wl,--gc-sections
```

### 32-bit cli build

Open the **MinGW-w32** terminal, and go to your project directory:

```bash
cd /c/users/username/documents/dearrow
```

> [!Important]
> replace "/c/users/username/documents/dearrow" with your project directory

Now run:

```bash
mkdir -p bin
windres -i res/resources-cli-x32.rc -I res -O coff -o bin/rsc-cli-x32.o
g++ src/cli.cpp bin/rsc-cli-x32.o -o bin/dearrow-cli-x32.exe -mconsole -ladvapi32 -lshell32 -nostartfiles -e _mainCRTStartup@0 -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -static-libgcc -Wl,--gc-sections
```

### If not using MSYS2

If you installed MinGW-w64 another way (e.g. WinLibs) and have both
architectures available, use the prefixed binaries explicitly instead:

```bash
mkdir -p bin

# 64-bit
x86_64-w64-mingw32-windres -i res/resources-x64.rc -I res -O coff -o bin/rsc-x64.o
x86_64-w64-mingw32-g++ src/main.cpp bin/rsc-x64.o -o bin/dearrow-x64.exe -mwindows -lgdi32 -ladvapi32 -lshell32 -nostartfiles -e WinMainCRTStartup -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections "-Wl,--gc-sections"

# 32-bit
i686-w64-mingw32-windres -i res/resources-x32.rc -I res -O coff -o bin/rsc-x32.o
i686-w64-mingw32-g++ src/main.cpp bin/rsc-x32.o -o bin/dearrow-x32.exe -mwindows -lgdi32 -ladvapi32 -lshell32 -nostartfiles -e _WinMainCRTStartup@0 -Os -s -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections "-Wl,--gc-sections"
```
