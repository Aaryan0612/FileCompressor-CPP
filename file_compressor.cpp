// ============================================================
//  REAL FILE COMPRESSION & DECOMPRESSION SYSTEM
//  Language  : C++
//  Algorithm : gzip (DEFLATE — LZ77 + Huffman coding)
//  Author    : Aaryan kuchekar
// ============================================================
//
//  HOW THE PROGRAM WORKS AT A HIGH LEVEL:
//  ─────────────────────────────────────
//  1. main() shows a menu and loops until the user exits.
//  2. The user picks Compress, Decompress, Compare, or Exit.
//  3. compressFile() / decompressFile() collect file paths,
//     create a GzipCompressor object, and call compress() or
//     decompress() through an ICompressor BASE CLASS POINTER.
//  4. GzipCompressor builds a shell command (e.g. "gzip -c
//     'input' > 'output'") and runs it via system().
//  5. The OS runs gzip and the result is saved to disk.
//  6. Any error anywhere is caught in main() — the program
//     NEVER crashes; it just shows the error and loops back.
//
//  OOP CONCEPTS USED:
//  ──────────────────
//  • Abstract class          → ICompressor
//  • Pure virtual functions  → compress() = 0, decompress() = 0
//  • Inheritance             → GzipCompressor : public ICompressor
//  • Runtime Polymorphism    → ICompressor* ptr = new GzipCompressor(...)
//  • Encapsulation           → private members in GzipCompressor
//  • Virtual destructor      → ~ICompressor()
//  • Exception Handling      → try / catch / throw
//  • File I/O                → ifstream for checks, size, comparison
// ============================================================

#include <cstdlib>     // system() — runs a terminal/shell command from C++
#include <fstream>     // ifstream (read files), ofstream (write files)
#include <iostream>    // cin, cout — user input and screen output
#include <stdexcept>   // runtime_error — the exception class we throw
#include <string>      // string class — stores file paths, commands, messages
#include <sys/stat.h>  // stat(), struct stat — checks if a path is a folder
#include <vector>      // vector<char> — dynamic array used as a file-read buffer

using namespace std;

// ============================================================
//  ABSTRACT BASE CLASS — ICompressor
// ============================================================
//
//  WHY THIS CLASS EXISTS:
//  An abstract class is a BLUEPRINT / CONTRACT.
//  It says: "Any compressor class MUST have compress() and
//  decompress(). I don't care HOW you do it — just that you do."
//
//  You CANNOT create an ICompressor object directly:
//      ICompressor obj;  ← COMPILE ERROR
//  You CAN create a GzipCompressor and point to it:
//      ICompressor* ptr = new GzipCompressor(...);  ← VALID
//  This is the foundation of Runtime Polymorphism.
// ============================================================
class ICompressor {
public:
    // pure virtual function — the "= 0" means NO body here.
    // Any class that inherits ICompressor MUST override this.
    virtual void compress()   = 0;
    virtual void decompress() = 0;

    // Virtual destructor — CRITICAL for memory safety.
    // When we write:  delete compressor;
    // ...and compressor is an ICompressor* pointer pointing to
    // a GzipCompressor object, the 'virtual' keyword ensures
    // GzipCompressor's destructor runs FIRST before ICompressor's.
    // Without 'virtual' here, GzipCompressor would never be
    // properly cleaned up → MEMORY LEAK.
    virtual ~ICompressor() {}
};

// ============================================================
//  DERIVED CLASS — GzipCompressor
// ============================================================
//
//  This is where the REAL work happens.
//  It inherits from ICompressor and fulfils the contract by
//  providing actual implementations of compress() and decompress().
//
//  ": public ICompressor" means:
//      GzipCompressor IS-A ICompressor.
//      It inherits the interface and MUST implement both functions.
// ============================================================
class GzipCompressor : public ICompressor {
private:
    // ── ENCAPSULATION ──────────────────────────────────────
    // These two strings are PRIVATE — they can only be accessed
    // by code INSIDE this class. External code cannot touch them
    // directly. They are set once via the constructor and used
    // inside compress() and decompress().
    string inputFile;   // full path of the file to compress/decompress
    string outputFile;  // full path where the result will be written

    // ── UTILITY: quotePath() ────────────────────────────────
    //
    //  PURPOSE: Wrap a file path in single quotes so it is
    //  safe to use inside a shell command.
    //
    //  PROBLEM IT SOLVES:
    //  If inputFile = "/home/user/my report.txt"
    //  then the raw command would be:
    //      gzip -c /home/user/my report.txt
    //  The SPACE breaks it — the shell reads "my" and
    //  "report.txt" as two separate arguments.
    //
    //  SOLUTION — wrap in single quotes:
    //      gzip -c '/home/user/my report.txt'  ← shell safe
    //
    //  EDGE CASE: What if the path itself contains a single
    //  quote? e.g. "user's folder/file.txt"
    //  A literal ' inside a single-quoted string breaks the shell.
    //  The standard shell escape for this is:  '\''
    //  So "user's" becomes: 'user'\''s'
    //
    //  'static' means this function belongs to the CLASS itself,
    //  not to any specific GzipCompressor object. It cannot
    //  access 'inputFile' or 'outputFile' because those are
    //  per-object (instance) variables.
    static string quotePath(const string& path) {
        string quoted = "'";                        // open single quote
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] == '\'') {
                quoted += "'\\''";                  // escape internal single quote
            } else {
                quoted += path[i];                  // normal character — add as-is
            }
        }
        quoted += "'";                              // close single quote
        return quoted;
    }

    // ── UTILITY: runCommand() ───────────────────────────────
    //
    //  PURPOSE: Run a terminal command from inside C++ and
    //  convert any failure into a C++ exception.
    //
    //  HOW system() WORKS:
    //  system("gzip -c 'file.txt' > 'file.gz'")
    //  This is EXACTLY like typing that command in your terminal.
    //  The OS runs it and returns:
    //      0         → command succeeded
    //      non-zero  → something went wrong
    //
    //  command.c_str() converts C++ string to a C-style char*
    //  because system() is a C function that expects char*, not
    //  a C++ string object.
    //
    //  If the command fails, we throw runtime_error so the error
    //  bubbles up to main() which catches and displays it.
    //  The program DOES NOT crash.
    static void runCommand(const string& command, const string& errorMessage) {
        int result = system(command.c_str());
        if (result != 0) {
            throw runtime_error(errorMessage);
        }
    }

public:
    // ── CONSTRUCTOR with INITIALIZER LIST ──────────────────
    //
    //  The ": inputFile(inputPath), outputFile(outputPath)" part
    //  is called a CONSTRUCTOR INITIALIZER LIST.
    //  It initializes the private member variables DIRECTLY at
    //  object creation — more efficient than assigning inside
    //  the constructor body.
    //
    //  'const string&' parameters:
    //    → '&'     = pass by REFERENCE (no copy of the string is made)
    //    → 'const' = we PROMISE not to modify the original string
    //  This is the preferred way to pass strings in C++.
    GzipCompressor(const string& inputPath, const string& outputPath)
        : inputFile(inputPath), outputFile(outputPath) {}

    // ── THE MOST IMPORTANT FUNCTION: compress() ────────────
    //
    //  This is where compression actually happens.
    //  We build a shell command and run it via runCommand().
    //
    //  WHAT THE COMMAND LOOKS LIKE:
    //      gzip -c '/path/to/input.txt' > '/path/to/output.gz'
    //
    //  BREAKING DOWN THE COMMAND:
    //  • gzip        = the compression tool
    //  • -c          = write compressed bytes to STDOUT (screen)
    //                  instead of creating a file automatically
    //  • > 'output'  = shell REDIRECT: instead of printing to
    //                  screen, write it into the output file
    //
    //  'override' confirms this is replacing the pure virtual
    //  function from ICompressor. If the base function signature
    //  ever changes and we forget to update, compiler gives error.
    void compress() override {
        string command = "gzip -c " + quotePath(inputFile) + " > " + quotePath(outputFile);
        runCommand(command, "Compression failed. Check the input file path and gzip availability.");
    }

    // ── THE MOST IMPORTANT FUNCTION: decompress() ──────────
    //
    //  Same idea as compress() but with -d flag to DEcompress.
    //
    //  WHAT THE COMMAND LOOKS LIKE:
    //      gzip -dc '/path/to/file.gz' > '/path/to/restored'
    //
    //  • -d = decompress mode
    //  • -c = still output to STDOUT so we can redirect with >
    void decompress() override {
        string command = "gzip -dc " + quotePath(inputFile) + " > " + quotePath(outputFile);
        runCommand(command, "Decompression failed. Ensure the file is a valid .gz file.");
    }
};

// ============================================================
//  UTILITY FUNCTIONS
//  These are standalone helper functions — not inside any class.
//  They handle specific small tasks to keep the main logic clean.
// ============================================================

// ── UTILITY: trimTrailingWhitespace() ──────────────────────
//
//  PURPOSE: Remove invisible characters from the END of user input.
//
//  PROBLEM IT SOLVES:
//  When the user types a file path and presses Enter, the input
//  string contains an invisible '\n' (newline) at the very end.
//  If that gets included in the filename, the OS cannot find the
//  file — even though it LOOKS correct on screen.
//
//  This function removes trailing: spaces (' '), tabs ('\t'),
//  carriage returns ('\r'), and newlines ('\n').
//
//  ABOUT THE PARAMETER: const string& text
//  ─────────────────────────────────────────────────────────
//  The '&' here means PASS BY REFERENCE.
//  Without '&': the string would be COPIED into a new variable.
//               Copying a long string wastes memory and time.
//  With    '&': no copy is made. The function works directly
//               on the original string's memory location.
//  The 'const' means: we CAN read the string but CANNOT modify it.
//  This pattern (const string&) is used in almost every function
//  in this program for the same reason.
string trimTrailingWhitespace(const string& text) {
    // find_last_not_of searches from the END of the string.
    // It returns the INDEX of the last character that is NOT
    // a space, tab, CR, or newline — i.e. the last real character.
    size_t end = text.find_last_not_of(" \t\r\n");

    // string::npos means "not found" — the entire string was whitespace.
    if (end == string::npos) {
        return "";
    }

    // substr(0, end + 1) returns from position 0 up to and
    // including 'end' — cutting off all trailing whitespace.
    return text.substr(0, end + 1);
}

// ── UTILITY: getInputLine() ────────────────────────────────
//
//  Reusable function for reading one line of user input.
//  Used everywhere instead of repeating cout + getline.
//
//  getline(cin, value) reads the ENTIRE line including spaces.
//  Using cin >> value would only read up to the first space —
//  which would break file paths like "my documents/file.txt".
string getInputLine(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    return trimTrailingWhitespace(value);  // clean invisible chars before returning
}

// ── UTILITY: fileExists() ──────────────────────────────────
//
//  PURPOSE: Check if a file actually exists before we try to
//  compress or decompress it. Prevents confusing errors later.
//
//  METHOD: Try to open the file. If it opens → exists.
//  If it fails to open → does not exist (or no permission).
//  is_open() returns true or false accordingly.
bool fileExists(const string& path) {
    ifstream file(path.c_str(), ios::binary);
    return file.is_open();
}

// ── UTILITY: isDirectory() ─────────────────────────────────
//
//  PURPOSE: Check if the given path is a FOLDER, not a file.
//
//  WHY WE NEED THIS:
//  The user can enter either a full output FILE path
//  OR just a FOLDER name. If they enter a folder, the program
//  should automatically generate the filename inside it.
//  This function lets us tell the difference.
//
//  HOW IT WORKS:
//  stat() from <sys/stat.h> fills a 'struct stat' with
//  information about the path — what type it is, its size, etc.
//  pathInfo.st_mode is a BITMASK — a number where each bit
//  means something different (file type, permissions, etc.)
//  S_IFDIR is the specific bit that means "is a directory".
//  The '&' (bitwise AND) checks if that bit is set.
bool isDirectory(const string& path) {
    struct stat pathInfo;
    if (stat(path.c_str(), &pathInfo) != 0) {
        return false;   // stat() failed — path doesn't exist
    }
    return (pathInfo.st_mode & S_IFDIR) != 0;
}

// ── UTILITY: getFileName() ─────────────────────────────────
//
//  PURPOSE: Extract just the filename from a full path.
//  Example: "TestingFolder/data.txt"  →  "data.txt"
//           "C:\\Users\\docs\\a.txt"  →  "a.txt"
//           "justfilename.txt"        →  "justfilename.txt"
//
//  find_last_of("/\\") finds the LAST slash in the path.
//  The "\\" is an escaped backslash — handles Windows paths too.
//  Everything AFTER that slash is the filename.
//  If no slash is found (npos), the path itself is the filename.
string getFileName(const string& path) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == string::npos) {
        return path;
    }
    return path.substr(slashPos + 1);
}

// ── UTILITY: joinPath() ────────────────────────────────────
//
//  PURPOSE: Safely combine a directory path and a filename.
//
//  Example: "MyFolder" + "file.gz"   →  "MyFolder/file.gz"
//           "MyFolder/" + "file.gz"  →  "MyFolder/file.gz"  (not "MyFolder//file.gz")
//
//  We check if the directory already ends with a slash.
//  If YES → just append the filename.
//  If NO  → add "/" first, then the filename.
//  This prevents double slashes in the path.
string joinPath(const string& directory, const string& fileName) {
    if (directory.empty()) {
        return fileName;
    }
    char lastChar = directory[directory.size() - 1];
    if (lastChar == '/' || lastChar == '\\') {
        return directory + fileName;
    }
    return directory + "/" + fileName;
}

// ── UTILITY: getFileSize() ─────────────────────────────────
//
//  PURPOSE: Find the size of a file in bytes.
//
//  WHY long long?
//  An 'int' can hold up to ~2.1 billion.
//  A 'long long' can hold up to ~9.2 QUINTILLION.
//  Modern files can be gigabytes — easily over 2 billion bytes.
//  Using 'int' for file size would OVERFLOW (wrap around to
//  negative) on large files. 'long long' is safe for any
//  realistic file size.
//
//  THE ios::ate TRICK:
//  'ios::ate' means "At The End" — when the file opens, the
//  read pointer is placed IMMEDIATELY at the end of the file.
//  Since the position of the end = number of bytes in the file,
//  tellg() gives us the file size WITHOUT reading through it.
//  This is fast even for very large files.
long long getFileSize(const string& path) {
    ifstream file(path.c_str(), ios::binary | ios::ate);
    if (!file.is_open()) {
        throw runtime_error("Cannot open file: " + path);
    }

    streampos position = file.tellg();   // position at end = file size
    if (position < 0) {
        throw runtime_error("Unable to determine file size: " + path);
    }

    return static_cast<long long>(position);
}

// ── UTILITY: filesAreEqual() ───────────────────────────────
//
//  PURPOSE: Prove that the restored file is IDENTICAL to the
//  original — byte for byte — confirming lossless compression.
//
//  WHY READ IN CHUNKS OF 4096 BYTES?
//  If the file is 1 GB, loading it all into memory at once
//  would crash the program. Reading 4096 bytes at a time
//  (one memory page) keeps RAM usage tiny and CONSTANT
//  no matter how large the files are.
//
//  WHAT IS vector<char>?
//  A vector is a DYNAMIC ARRAY — its size is set at runtime,
//  not at compile time. Here we create two vectors of 4096
//  characters each — one for each file.
//  vector<char> firstBuffer(4096) means:
//    → Create a dynamic array that holds 4096 characters
//    → All slots start as '\0' (null character)
//  We use TWO separate buffers so we can read both files
//  simultaneously and compare them chunk by chunk.
//
//  WHAT IS buf.data()?
//  data() returns a raw char* pointer to the vector's internal
//  array. The read() function needs a raw char* pointer — it
//  cannot accept a vector object directly.
//  Think of it as: "give me the address of the first element."
bool filesAreEqual(const string& firstPath, const string& secondPath) {
    ifstream first(firstPath.c_str(), ios::binary);
    ifstream second(secondPath.c_str(), ios::binary);

    if (!first.is_open()) {
        throw runtime_error("Cannot open file: " + firstPath);
    }
    if (!second.is_open()) {
        throw runtime_error("Cannot open file: " + secondPath);
    }

    const size_t bufferSize = 4096;         // read 4096 bytes at a time

    // Two separate buffers — one for each file.
    // vector<char> buf(4096) creates a dynamic char array of size 4096.
    vector<char> firstBuffer(bufferSize);
    vector<char> secondBuffer(bufferSize);

    while (true) {
        // read(pointer_to_array, number_of_bytes)
        // .data() gives us the raw char* pointer the read() function needs.
        first.read(firstBuffer.data(), static_cast<streamsize>(bufferSize));
        second.read(secondBuffer.data(), static_cast<streamsize>(bufferSize));

        // gcount() = how many bytes were ACTUALLY read in the last read().
        // The last chunk of a file may be smaller than 4096 — we need the
        // real count, not the requested count.
        streamsize firstRead  = first.gcount();
        streamsize secondRead = second.gcount();

        if (firstRead != secondRead) {
            return false;   // different chunk sizes = files have different sizes = not equal
        }

        if (firstRead == 0) {
            return true;    // both ended at the same time AND all previous chunks matched
                            // → files are IDENTICAL
        }

        // Compare every byte in this chunk.
        // One mismatch = files are not identical.
        for (streamsize i = 0; i < firstRead; ++i) {
            if (firstBuffer[static_cast<size_t>(i)] != secondBuffer[static_cast<size_t>(i)]) {
                return false;
            }
        }
        // if we reach here, this chunk matched — loop to read next chunk
    }
}

// ── UTILITY: defaultCompressedName() ──────────────────────
//
//  If the user presses Enter (gives no output path) during
//  compression, we just add ".gz" to the input filename.
//  Example: "report.pdf" → "report.pdf.gz"
string defaultCompressedName(const string& inputPath) {
    return inputPath + ".gz";
}

// ── UTILITY: defaultRestoredName() ─────────────────────────
//
//  If the user presses Enter during decompression, we strip
//  the ".gz" extension and add "_restored" as a safe default.
//  Example: "report.pdf.gz" → "report.pdf_restored"
//  Why "_restored"? To avoid accidentally overwriting the
//  original file if it's still in the same folder.
string defaultRestoredName(const string& compressedPath) {
    const string extension = ".gz";
    if (compressedPath.size() >= extension.size() &&
        compressedPath.substr(compressedPath.size() - extension.size()) == extension) {
        return compressedPath.substr(0, compressedPath.size() - extension.size()) + "_restored";
    }
    return compressedPath + "_restored";
}

// ── UTILITY: resolveCompressedOutputPath() ─────────────────
//
//  PURPOSE: Decide the FINAL output path for the compressed file.
//
//  ABOUT THE PARAMETERS:
//  ─────────────────────────────────────────────────────────────
//  string resolveCompressedOutputPath(const string& inputPath,
//                                     const string& userOutputPath)
//
//  The 'const string& inputPath' means:
//    → string      = the type (a string)
//    → &           = pass by REFERENCE — no copy is made.
//                    The function works directly on the caller's
//                    variable in memory, not a copy of it.
//                    This is EFFICIENT for large strings.
//    → const       = the function PROMISES not to modify this
//                    string. If you try to change it inside,
//                    the compiler gives an error.
//  In short: "give me direct access to the original string
//  but I won't change it."
//
//  THREE CASES THIS FUNCTION HANDLES:
//  ─────────────────────────────────────────────────────────────
//  Case 1: User pressed Enter (empty string)
//          → auto-generate name beside the input file
//            "report.pdf" → "report.pdf.gz"
//
//  Case 2: User entered an existing folder name
//          → put the compressed file inside that folder
//            "report.pdf" + "BackupFolder/" → "BackupFolder/report.pdf.gz"
//
//  Case 3: User entered a full file path
//          → use it exactly as typed
//            "output/compressed.gz" → "output/compressed.gz"
string resolveCompressedOutputPath(const string& inputPath, const string& userOutputPath) {
    if (userOutputPath.empty()) {
        return defaultCompressedName(inputPath);    // Case 1: pressed Enter
    }

    if (isDirectory(userOutputPath)) {
        // Case 2: user gave a folder → put file inside it
        return joinPath(userOutputPath, getFileName(inputPath) + ".gz");
    }

    return userOutputPath;    // Case 3: user gave a full path — use it directly
}

// ── UTILITY: resolveRestoredOutputPath() ───────────────────
//
//  Same three-case logic as resolveCompressedOutputPath()
//  but for DECOMPRESSION output.
//
//  Case 1: pressed Enter → "report.pdf.gz" becomes "report.pdf_restored"
//  Case 2: entered a folder → restored file placed inside that folder
//  Case 3: entered a full path → use it directly
string resolveRestoredOutputPath(const string& inputPath, const string& userOutputPath) {
    if (userOutputPath.empty()) {
        return defaultRestoredName(inputPath);
    }

    if (isDirectory(userOutputPath)) {
        return joinPath(userOutputPath, defaultRestoredName(getFileName(inputPath)));
    }

    return userOutputPath;
}

// ── UTILITY: printCompressionReport() ─────────────────────
//
//  Shows a clean summary after compression:
//  original size, compressed size, and the percentage change.
//
//  WHY long long for sizes?
//  Same reason as getFileSize() — files can be gigabytes.
//  'int' would overflow on files larger than ~2 GB.
//  'long long' safely handles files up to exabytes.
//
//  cout.setf(ios::fixed) + cout.precision(2) makes the
//  percentage print with exactly 2 decimal places (e.g. 47.35%).
void printCompressionReport(const string& originalFile, const string& compressedFile) {
    long long originalSize   = getFileSize(originalFile);
    long long compressedSize = getFileSize(compressedFile);

    cout << "\nCompression Report\n";
    cout << "Original file size   : " << originalSize   << " bytes\n";
    cout << "Compressed file size : " << compressedSize << " bytes\n";

    if (originalSize == 0) {
        cout << "Result               : Empty file handled successfully\n";
        return;
    }

    if (compressedSize < originalSize) {
        double saved = static_cast<double>(originalSize - compressedSize) * 100.0 /
                       static_cast<double>(originalSize);
        cout.setf(ios::fixed);
        cout.precision(2);
        cout << "Result               : File size reduced by " << saved << "%\n";
        cout.unsetf(ios::floatfield);
    } else if (compressedSize > originalSize) {
        // This happens on already-compressed files (JPEG, MP4, ZIP).
        // gzip adds its own header overhead but cannot compress further.
        double increase = static_cast<double>(compressedSize - originalSize) * 100.0 /
                          static_cast<double>(originalSize);
        cout.setf(ios::fixed);
        cout.precision(2);
        cout << "Result               : File size increased by " << increase << "%\n";
        cout.unsetf(ios::floatfield);
    } else {
        cout << "Result               : File size remained the same\n";
    }
}

// ============================================================
//  OPERATION HANDLER — compressFile()
//  ★ ONE OF THE MOST IMPORTANT FUNCTIONS ★
//
//  This orchestrates the ENTIRE compression flow:
//  1. Ask user for input file path → validate it exists
//  2. Ask user for output path → resolve to final path
//  3. Create GzipCompressor using a BASE CLASS POINTER
//     (this is Runtime Polymorphism in action)
//  4. Call compress() through the base pointer
//  5. Safely delete the object (even if an error occurs)
//  6. Print the compression report
// ============================================================
void compressFile() {
    // Step 1: Get and validate the input file path
    string inputPath = getInputLine("Enter input file path              : ");
    if (inputPath.empty()) {
        throw runtime_error("Input file path cannot be empty.");
    }
    if (!fileExists(inputPath)) {
        throw runtime_error("Input file not found: " + inputPath);
    }

    // Step 2: Get output path and resolve it to the final destination
    string outputPath = getInputLine("Enter output compressed file path or folder: ");
    outputPath = resolveCompressedOutputPath(inputPath, outputPath);

    // Step 3 & 4: ★ RUNTIME POLYMORPHISM ★
    // We create a GzipCompressor object but store it in an
    // ICompressor BASE CLASS POINTER.
    // When compress() is called through this pointer, C++ checks
    // at RUNTIME what object is actually stored there (GzipCompressor)
    // and calls GzipCompressor's version — NOT ICompressor's.
    // This is called Late Binding or Runtime Polymorphism.
    ICompressor* compressor = new GzipCompressor(inputPath, outputPath);

    // Step 5: ★ MEMORY SAFETY PATTERN ★
    // Why try-catch around just this part?
    // If compress() throws an exception, the 'delete compressor'
    // on the next line would be SKIPPED — causing a memory leak.
    // The catch(...) catches ANY exception, deletes the object
    // first (safe cleanup), then re-throws with bare 'throw;'
    // so main() can catch it and show the error message.
    try {
        compressor->compress();
        delete compressor;  // free memory on success
    } catch (...) {
        delete compressor;  // free memory EVEN on failure — no leak
        throw;              // re-throw the same exception to main()
    }

    cout << "\nCompression completed successfully.\n";
    cout << "Compressed file saved as: " << outputPath << "\n";

    // Step 6: Print the size comparison report
    printCompressionReport(inputPath, outputPath);
}

// ============================================================
//  OPERATION HANDLER — decompressFile()
//  ★ ONE OF THE MOST IMPORTANT FUNCTIONS ★
//
//  Mirror of compressFile() — same pattern, different operation.
//  Creates a GzipCompressor via base pointer, calls decompress().
// ============================================================
void decompressFile() {
    // Step 1: Get and validate the compressed input file
    string inputPath = getInputLine("Enter compressed file path         : ");
    if (inputPath.empty()) {
        throw runtime_error("Compressed file path cannot be empty.");
    }
    if (!fileExists(inputPath)) {
        throw runtime_error("Compressed file not found: " + inputPath);
    }

    // Step 2: Get output path and resolve it
    // resolveRestoredOutputPath handles: Enter / folder / full path
    string outputPath = getInputLine("Enter restored output file path or folder   : ");
    outputPath = resolveRestoredOutputPath(inputPath, outputPath);

    // Step 3 & 4: Runtime Polymorphism — base pointer, derived object
    ICompressor* decompressor = new GzipCompressor(inputPath, outputPath);

    // Step 5: Same memory-safe try-catch-delete pattern
    try {
        decompressor->decompress();
        delete decompressor;
    } catch (...) {
        delete decompressor;
        throw;
    }

    cout << "\nDecompression completed successfully.\n";
    cout << "Restored file saved as : " << outputPath << "\n";
    cout << "Restored file size     : " << getFileSize(outputPath) << " bytes\n";
}

// ============================================================
//  OPERATION HANDLER — compareFiles()
//
//  Proves that decompression is LOSSLESS — the restored file
//  is byte-for-byte identical to the original.
//  Calls filesAreEqual() which reads both files in 4096-byte
//  chunks and compares every single byte.
// ============================================================
void compareFiles() {
    string originalPath = getInputLine("Enter original file path           : ");
    string restoredPath = getInputLine("Enter restored file path           : ");

    if (originalPath.empty() || restoredPath.empty()) {
        throw runtime_error("File paths cannot be empty.");
    }
    if (!fileExists(originalPath)) {
        throw runtime_error("Original file not found: " + originalPath);
    }
    if (!fileExists(restoredPath)) {
        throw runtime_error("Restored file not found: " + restoredPath);
    }
    if (isDirectory(originalPath) || isDirectory(restoredPath)) {
        throw runtime_error("Please provide file paths, not directories.");
    }

    bool match = filesAreEqual(originalPath, restoredPath);

    cout << "\nVerification Report\n";
    cout << "Original size        : " << getFileSize(originalPath) << " bytes\n";
    cout << "Restored size        : " << getFileSize(restoredPath) << " bytes\n";
    cout << "Comparison result    : " << (match ? "MATCH" : "MISMATCH") << "\n";
}

// ── UTILITY: printMenu() ───────────────────────────────────
//
//  Prints the 4-option menu at the start of each loop iteration.
void printMenu() {
    cout << "\n===========================================\n";
    cout << "   Real File Compression / Decompression   \n";
    cout << "===========================================\n";
    cout << "1. Compress a file\n";
    cout << "2. Decompress a file\n";
    cout << "3. Compare original and restored files\n";
    cout << "4. Exit\n";
}

// ============================================================
//  main() — THE ENTRY POINT
//  ★ THE MOST IMPORTANT FUNCTION — EXECUTION STARTS HERE ★
//
//  The OS always calls main() first when the program runs.
//
//  STRUCTURE:
//  • while(true)  → keeps the program alive until the user exits
//  • try { ... }  → wraps ALL operations inside the loop
//  • catch(...)   → catches ANY error thrown by ANY function
//                   below — compressFile, decompressFile,
//                   compareFiles, getFileSize, filesAreEqual...
//                   The error is DISPLAYED and the loop CONTINUES.
//                   The program NEVER crashes.
//
//  FLOW:
//  Show menu → read choice → call the right function
//  → if error: catch and show → loop back to menu
//  → if choice == 4: break out of while → reach return 0
// ============================================================
int main() {
    while (true) {
        try {
            printMenu();
            string choice = getInputLine("Enter your choice                  : ");

            if (choice == "1") {
                compressFile();
            } else if (choice == "2") {
                decompressFile();
            } else if (choice == "3") {
                compareFiles();
            } else if (choice == "4") {
                cout << "Exiting program.\n";
                break;  // exits the while(true) loop → reaches return 0
            } else {
                cout << "Invalid choice. Please enter 1, 2, 3, or 4.\n";
            }

        } catch (const exception& e) {
            // e.what() returns the message passed to runtime_error().
            // Example: "Input file not found: report.pdf"
            // After printing, the loop continues — program stays alive.
            cout << "\n[ERROR] " << e.what() << "\n";
        }
    }

    return 0;  // tells the OS: program finished successfully
}