# PartVault

PartVault is a Qt-based inventory manager for electronic components.

It stores parts in an SQLite database, organizes them by category and storage location, and tracks related files such as datasheets, CAD sources, and 3D models.

## Features

- Hierarchical categories
- Hierarchical storage locations
- Parts table with quantity, category, location, and last-change timestamp
- File attachments stored as relative paths under the application data directory
- Open file and open containing directory actions from the UI
- Cleanup actions for unused categories, storage locations, and files
- Ukrainian translation support
- Dummy data seeding for testing and demo use

## Tech Stack

- C++17
- Qt 6 Widgets
- SQLite
- CMake 3.19+
- Qt Linguist tools
- Qt Installer Framework for Windows installer packaging

## Project Layout

- `src/` — application sources, UI, SQL resources, translations
- `tests/` — unit tests
- `cmake/` — packaging helpers, including Qt IFW support
- `installer/` — Qt Installer Framework metadata

## Data Storage

At runtime the application stores its data under:

- `~/.partvault`

Inside that directory PartVault creates:

- `parts.db`
- `cad/`
- `datasheets/`
- `models/`

The `files.path` values stored in the database are relative to this directory.

## Command-Line Options

PartVault supports these startup options:

- `--help`
- `--version`
- `--log-level <debug|info|warning|critical|fatal>`
- `--dummy` — populate the database with sample data
- `--reset` — recreate the database from scratch

Example:

```powershell
PartVault.exe --reset --dummy --log-level debug
```

## Building

### Requirements

You need a Qt 6 installation with these modules available to CMake:

- Core
- Widgets
- Sql
- LinguistTools
- Test

### Configure

If Qt is not already configured in your environment, pass `CMAKE_PREFIX_PATH`.

Windows example:

```powershell
cmake -S . -B build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/llvm-mingw_64
```

Linux example:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

### Build

Windows:

```powershell
cmake --build build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug --config Debug
```

Linux:

```bash
cmake --build build
```

## Running

From a build directory, run the generated executable.

Windows example:

```powershell
.\build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug\PartVault.exe --dummy
```

## Tests

The repository currently contains one Qt Test target for SQL comment stripping.

Build and run tests with CTest:

```powershell
cmake --build build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug --config Debug
ctest --test-dir build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug --output-on-failure -C Debug
```

## Dummy Data

Running with `--dummy` loads sample parts, categories, storage locations, and file records from `src/sql/dummy.sql`.

It also creates placeholder files on disk for the seeded file records, including:

- PDF datasheets in `datasheets/`
- STEP models in `models/`
- CAD placeholder files in `cad/`

Some dummy file records are intentionally left unused so the `Remove unused files` action can be tested.

## TODO / Planned Features

- Add create, edit, and remove workflows for file attachments
- Add import and export support for the database
- Add part editing and richer search/filter tools
- Add physical file management helpers, including copy/move into the data directory
- Add more unit tests for database and model behavior
- Add Release build packaging and CI artifacts for Windows and Linux

## Windows Installer

This project includes Qt Installer Framework packaging support in `cmake/QtIfwInstaller.cmake`.

Prerequisites:

- Qt Installer Framework installed on Windows
- `binarycreator.exe` available, or `PARTVAULT_BINARYCREATOR_EXECUTABLE` set explicitly

Build the installer with:

```powershell
cmake --build build\Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug --target ifw_installer --config Debug
```

The installer output is written to the build directory as:

- `PartVault-Installer-<version>.exe`

## License

PartVault is licensed under the GNU General Public License v3.0 or later.

See `LICENSE` for the full license text.
