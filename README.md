# Top-Files

A CLI tool that recursively walks a directory, filters out anything smaller than your configured threshold, and lists the survivors sorted by size — smallest to largest — so you can see exactly which files on your system are eating your storage alive.

Think of it as `du -ah | sort -h | grep -v "^[0-9]*[KM]\b"`, except it doesn't require you to remember that incantation at 2am, and it shows you the total size across all matching files as a bonus diagnosis at the end.

---

## What It Does

```
$ topfiles /home/user

▌ Fetching files...
▋ Sorting...
▌ Finding total size...

/home/user/Downloads/ubuntu.iso                   : 1.23 GB
/home/user/Videos/lecture_recording.mkv           : 4.56 GB
/home/user/.local/share/Steam/steamapps/...       : 12.3 GB

Showing diagnosis for "/home/user"
---------------------------------
Found 3 files.
Total size: 18.09 GB
```

Files are listed left-padded so the size column lines up perfectly regardless of how deeply nested the paths get. The spinner animates through a smooth Unicode block progression while it works. All of this for a binary that is, comically, smaller than the wordlist in [Hangman](https://github.com/HassanIQ777/Hangman).

---

## Features

- **Recursive file scan** via `File::listfiles_recursive()`, respecting your exception list
- **Configurable minimum file size** — defaults to 10 MB; files below the threshold are silently erased from consideration (like they never existed)
- **Exception list** — a plain text file (`exception_list.txt`) with paths to skip entirely; populated by you, respected by the tool
- **Sorted output** — ascending by file size via `std::sort` + a lambda, so the big offenders float to the bottom where your eyes naturally land
- **Human-readable sizes** — `numutils::bytes()` converts raw byte counts to KB/MB/GB
- **Column-aligned output** — `strutils::pad_right()` pads every path to the width of the longest one so the size column is always flush
- **Animated spinner** — a 12-frame Unicode block spinner (`▏▎▍▌▋▊▉▊▋▌▍▎`) at 150ms/frame with live status messages: *Fetching files → Sorting → Finding total size*
- **Auto-bootstrapping config** — on first run, `config.ini` and `exception_list.txt` are created for you if they don't exist. Zero setup.
- **Diagnosis summary** — prints total file count and combined size of everything it found

---

## Configuration

On first run, two files are created in the **current working directory** (wherever you ran `topfiles` from):

### `config.ini`

```ini
min_file_size=10485760
```

Change `min_file_size` to any byte value. Some useful ones:

| Value | Meaning |
|---|---|
| `1048576` | 1 MB |
| `10485760` | 10 MB (default) |
| `104857600` | 100 MB |
| `1073741824` | 1 GB |

### `exception_list.txt`

One path per line. Any file whose full path matches an entry here will be skipped during the scan. Useful for things like `.git` directories, `/proc`, or that one massive file you already know about and are choosing to live with.

```
/home/user/.local/share/Steam
/home/user/.cache
```

> **Note:** Both config files are looked up relative to the working directory at runtime, not the binary's location. If you install `topfiles` system-wide and call it from `/home/user`, that's where it looks for `config.ini`.

---

## Building

### Dependencies

Only `g++` with C++20 support and `make`. The `libutils` library is bundled — no external packages needed.

```bash
git clone https://github.com/HassanIQ777/Top-Files
cd Top-Files
make
```

The Makefile is smart about `libutils`: if `libutils.a` is already built it links against it directly; if not, it builds `libutils` from source first; if the `libutils/` directory doesn't exist at all, it compiles without it. Exactly zero manual steps required.

```bash
# Release build (default) — -O2 -march=native -flto
make

# Debug build — ASan + UBSan, -Og
make debug

# Build and run immediately (pass ARGS= for arguments)
make run ARGS="$HOME/Downloads"

# Install to /usr/local/bin
make install

# Install to a custom prefix
make install PREFIX=~/.local

# Remove from system ("yeeted into the void", per the Makefile)
make uninstall

# Clean build artifacts ("Nothing but echoes remain...")
make clean
```

The binary is named `topfiles` and lands in the project root (or `$(PREFIX)/bin` after install).

---

## Usage

```bash
topfiles <directory>
```

Exactly one argument. Give it a directory. It does the rest.

```bash
# Scan your home directory
topfiles ~

# Scan a specific folder
topfiles /var/log

# Scan the root filesystem (add system paths to exception_list.txt first, or enjoy the ride)
topfiles /
```

**Exit behavior:**
- No argument → error: *"Expected 2 arguments, but only one was provided."*
- Too many arguments → error: *"Expected 2 arguments, but N were provided."*
- Path is not a directory → error: *"The provided path is not a directory."*
- No files found above the threshold → prints *"No files were found."* and the summary (0 files, 0 bytes)

---

## How It Works (Briefly)

1. `Config` bootstraps `config.ini` and `exception_list.txt` if missing, then loads both.
2. A `Loadingbar::Spinner` is started on a background thread — it keeps animating while the heavy work happens.
3. `File::listfiles_recursive()` walks the directory tree, skipping anything in `exception_list`.
4. A `std::remove_if` + `erase` pass filters out non-files and anything below `min_file_size`.
5. `std::sort` with a size-comparing lambda sorts ascending.
6. A final pass computes `total_size` and finds the longest path (for column alignment).
7. Spinner stops, `\r` clears the spinner line, and the results are printed.

The `std::remove_if` + `erase` combo is the classic erase-remove idiom — worth knowing if you haven't seen it before. It partitions the vector in-place (moving unwanted elements to the end), then `erase` chops the tail. No copies, no new allocations.

---

## Project Structure

```
Top-Files/
├── main.cpp          # ~90 lines — the entire program
├── Makefile          # smart enough to handle 3 different libutils states
├── libutils/         # bundled utility library
│   ├── src/
│   │   ├── CLIParser.hpp
│   │   ├── File.hpp
│   │   ├── LoadingBar.hpp
│   │   ├── Log.hpp
│   │   ├── funcs.hpp
│   │   ├── numutils.hpp
│   │   └── strutils.hpp
│   └── Makefile
└── LICENSE           # MIT
```

---

## MIT License
[![License: MIT](https://i0.wp.com/opensource.org/wp-content/uploads/2023/03/cropped-OSI-horizontal-large.png)](https://opensource.org/license/mit)


---

## **Made by [HassanIQ777](https://github.com/HassanIQ777)**

### Contributions are welcomed!