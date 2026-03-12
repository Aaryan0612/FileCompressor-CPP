# 🗜️ Real File Compression & Decompression System

> A C++ command-line application that compresses and decompresses real files using your system’s `gzip` utility, wrapped in a clean Object-Oriented design.

This project demonstrates practical systems programming: file handling, shell integration, robust exception handling, and byte-level integrity verification.

---

## ✨ Features

- 📦 Compress any file into `.gz` (text, PDF, image, binary)
- 📂 Decompress `.gz` files back to restored outputs
- ✅ Verify integrity using exact byte-for-byte comparison (`MATCH` / `MISMATCH`)
- 📊 Compression report with original size, compressed size, and percentage change
- 🧠 Smart output path resolution (blank/default, folder path, or full file path)
- 🛡️ Centralized error handling (`try` / `catch`) to keep the app stable

---

## 🏗️ Architecture

```text
┌─────────────────────────────────────────────┐
│  LAYER 1 — CLI / User Interaction           │
│  main() loop · menu · getInputLine()        │
├─────────────────────────────────────────────┤
│  LAYER 2 — Operation Handlers               │
│  compressFile() · decompressFile()          │
│  compareFiles()                             │
├─────────────────────────────────────────────┤
│  LAYER 3 — OOP Abstraction                  │
│  ICompressor (abstract interface)           │
│      └── GzipCompressor (implementation)    │
├─────────────────────────────────────────────┤
│  LAYER 4 — System Utility                   │
│  gzip (invoked via system command)          │
└─────────────────────────────────────────────┘
```

---

## 🧩 OOP Concepts Demonstrated

| Concept                | Usage in code                                   |
| ---------------------- | ----------------------------------------------- |
| Abstraction            | `ICompressor` interface                         |
| Pure virtual functions | `compress() = 0`, `decompress() = 0`            |
| Inheritance            | `GzipCompressor : public ICompressor`           |
| Runtime polymorphism   | `ICompressor*` pointing to `GzipCompressor`     |
| Encapsulation          | `private` members for paths and command helpers |
| Virtual destructor     | `virtual ~ICompressor() {}`                     |
| Exception handling     | `throw`, `try`, `catch` across operations       |
| File I/O utilities     | `ifstream` for existence/size/compare           |
| Static helper methods  | `quotePath()`, `runCommand()`                   |

---

## 📁 Repository Structure

```text
CPP_finalMP/
├── file_compressor.cpp
├── README.md
├── Documentation/
│   ├── FileCompression-AaryanKuchekar-2.pdf
│   └── Real-File-Compression-and-Decompression-System.pdf
├── TestingFolder/
│   ├── Testing.txt
│   ├── text1.txt
│   └── testing.jpg
├── OutputFolder/
└── restoreFolder/
```

---

## ⚙️ Requirements

- C++ compiler (`g++` or `clang++`) with C++11+ support
- `gzip` available in PATH
- macOS / Linux terminal (current implementation uses POSIX `stat`)

Check gzip:

```bash
gzip --version
```

---

## 🚀 Build and Run

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -o compressor file_compressor.cpp
./compressor
```

---

## 🖥️ Program Menu

```text
===========================================
   Real File Compression / Decompression
===========================================
1. Compress a file
2. Decompress a file
3. Compare original and restored files
4. Exit
```

---

## 🔁 Quick Demo (Using Current Repo Files)

Try this flow with existing samples:

1. Compress `TestingFolder/Testing.txt` and give output folder as `OutputFolder/`
2. Decompress generated `.gz` into `restoreFolder/`
3. Compare original and restored file paths using option 3

If output says `MATCH`, compression/decompression was lossless.

---

## 🧭 Output Path Rules

- Compression default (blank output):
  - `input.ext` → `input.ext.gz`
- Decompression default (blank output):
  - `input.ext.gz` → `input.ext_restored`
  - non-`.gz` input → `input_restored`
- If output is a directory, file name is auto-resolved inside that directory.

---

## 🔍 Internal Commands

Compression command pattern:

```bash
gzip -c 'input_file' > 'output_file'
```

Decompression command pattern:

```bash
gzip -dc 'compressed_file' > 'restored_file'
```

`quotePath()` wraps paths in single quotes to safely handle spaces/special characters.

---

## 🛡️ Error Handling

The program catches and reports common failures without crashing, such as:

- empty paths
- missing files
- invalid gzip input for decompression
- directory passed where a file is expected
- command execution failure

Example format:

```text
[ERROR] Input file not found: <path>
```

---

## 📚 Documentation

Project reports available in:

- [Documentation/FileCompression-AaryanKuchekar-2.pdf](Documentation/FileCompression-AaryanKuchekar-2.pdf)
- [Documentation/Real-File-Compression-and-Decompression-System.pdf](Documentation/Real-File-Compression-and-Decompression-System.pdf)

---

## 🔭 Future Scope

- [ ] Replace raw pointers with `std::unique_ptr`
- [ ] Add unit tests and CI pipeline
- [ ] Support additional algorithms (ZIP, Bzip2, LZ4)
- [ ] Add recursive folder compression
- [ ] Add cross-platform process layer (including Windows)

---

## 👨‍💻 Author

**Aaryan Kuchekar**

---

## 📄 License

This repository currently has no dedicated license file. Add an MIT (or similar) license before public open-source distribution.
