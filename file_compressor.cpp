#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace std;

// Abstract base class.
// It defines the common interface that any compressor class must follow.
class ICompressor {
public:
    virtual void compress() = 0;
    virtual void decompress() = 0;
    virtual ~ICompressor() {}
};

// This derived class handles real compression by calling the gzip utility.
// The C++ code manages the user interaction, file paths, reports, and errors.
class GzipCompressor : public ICompressor {
private:
    string inputFile;
    string outputFile;

    // Wrap a file path in single quotes so the shell can safely handle spaces.
    // If a single quote appears inside the path, it is escaped first.
    static string quotePath(const string& path) {
        string quoted = "'";
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] == '\'') {
                quoted += "'\\''";
            } else {
                quoted += path[i];
            }
        }
        quoted += "'";
        return quoted;
    }

    // Execute the shell command and translate shell failure into a C++ exception.
    // This allows main() to catch all failures in one consistent way.
    static void runCommand(const string& command, const string& errorMessage) {
        int result = system(command.c_str());
        if (result != 0) {
            throw runtime_error(errorMessage);
        }
    }

public:
    GzipCompressor(const string& inputPath, const string& outputPath)
        : inputFile(inputPath), outputFile(outputPath) {}

    void compress() override {
        // gzip -c compresses the input file and prints the compressed bytes.
        // The shell redirect '>' writes those bytes into the output file.
        string command = "gzip -c " + quotePath(inputFile) + " > " + quotePath(outputFile);
        runCommand(command, "Compression failed. Check the input file path and gzip availability.");
    }

    void decompress() override {
        // gzip -d restores the original content.
        // The -c option prints the restored bytes, which we redirect into the output file.
        string command = "gzip -dc " + quotePath(inputFile) + " > " + quotePath(outputFile);
        runCommand(command, "Decompression failed. Ensure the file is a valid .gz file.");
    }
};

// Remove trailing spaces or newlines from what the user typed.
// This prevents accidental input issues such as invisible newline characters in paths.
string trimTrailingWhitespace(const string& text) {
    size_t end = text.find_last_not_of(" \t\r\n");
    if (end == string::npos) {
        return "";
    }
    return text.substr(0, end + 1);
}

// Common helper for taking input from the user.
string getInputLine(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    return trimTrailingWhitespace(value);
}

// Basic file existence check before compression or decompression begins.
bool fileExists(const string& path) {
    ifstream file(path.c_str(), ios::binary);
    return file.is_open();
}

// Check whether the given path is a directory.
// This is important because the user is allowed to enter either a full output file path
// or just a folder where the program should create the output file automatically.
bool isDirectory(const string& path) {
    struct stat pathInfo;
    if (stat(path.c_str(), &pathInfo) != 0) {
        return false;
    }
    return (pathInfo.st_mode & S_IFDIR) != 0;
}

// Extract only the final file name from a full path.
// Example: "TestingFolder/data.txt" becomes "data.txt".
string getFileName(const string& path) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == string::npos) {
        return path;
    }
    return path.substr(slashPos + 1);
}

// Combine a directory path and file name into one full path.
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

// Open the file at the end using ios::ate and use tellg() to read its size in bytes.
long long getFileSize(const string& path) {
    ifstream file(path.c_str(), ios::binary | ios::ate);
    if (!file.is_open()) {
        throw runtime_error("Cannot open file: " + path);
    }

    streampos position = file.tellg();
    if (position < 0) {
        throw runtime_error("Unable to determine file size: " + path);
    }

    return static_cast<long long>(position);
}

// Compare two files chunk by chunk.
// This is better than reading the entire file into memory at once.
// If every chunk matches and both files end together, the files are identical.
bool filesAreEqual(const string& firstPath, const string& secondPath) {
    ifstream first(firstPath.c_str(), ios::binary);
    ifstream second(secondPath.c_str(), ios::binary);

    if (!first.is_open()) {
        throw runtime_error("Cannot open file: " + firstPath);
    }
    if (!second.is_open()) {
        throw runtime_error("Cannot open file: " + secondPath);
    }

    const size_t bufferSize = 4096;
    vector<char> firstBuffer(bufferSize);
    vector<char> secondBuffer(bufferSize);

    while (true) {
        first.read(firstBuffer.data(), static_cast<streamsize>(bufferSize));
        second.read(secondBuffer.data(), static_cast<streamsize>(bufferSize));

        streamsize firstRead = first.gcount();
        streamsize secondRead = second.gcount();

        if (firstRead != secondRead) {
            return false;
        }

        if (firstRead == 0) {
            return true;
        }

        for (streamsize i = 0; i < firstRead; ++i) {
            if (firstBuffer[static_cast<size_t>(i)] != secondBuffer[static_cast<size_t>(i)]) {
                return false;
            }
        }
    }
}

// If the user presses Enter during compression, this becomes the default output file.
string defaultCompressedName(const string& inputPath) {
    return inputPath + ".gz";
}

// If the user presses Enter during decompression, restore into a readable default file name.
// Example: "report.pdf.gz" becomes "report.pdf_restored".
string defaultRestoredName(const string& compressedPath) {
    const string extension = ".gz";
    if (compressedPath.size() >= extension.size() &&
        compressedPath.substr(compressedPath.size() - extension.size()) == extension) {
        return compressedPath.substr(0, compressedPath.size() - extension.size()) + "_restored";
    }
    return compressedPath + "_restored";
}

// Decide where the compressed file should be written.
// Cases:
// 1. User presses Enter -> use default file name beside the input file.
// 2. User enters a folder -> place the compressed file inside that folder.
// 3. User enters a full file path -> use it directly.
string resolveCompressedOutputPath(const string& inputPath, const string& userOutputPath) {
    if (userOutputPath.empty()) {
        return defaultCompressedName(inputPath);
    }

    if (isDirectory(userOutputPath)) {
        return joinPath(userOutputPath, getFileName(inputPath) + ".gz");
    }

    return userOutputPath;
}

// Same path resolution logic as compression, but for restored output files.
string resolveRestoredOutputPath(const string& inputPath, const string& userOutputPath) {
    if (userOutputPath.empty()) {
        return defaultRestoredName(inputPath);
    }

    if (isDirectory(userOutputPath)) {
        return joinPath(userOutputPath, defaultRestoredName(getFileName(inputPath)));
    }

    return userOutputPath;
}

// Print a clear report after compression so the user can see whether the file became
// smaller, larger, or stayed the same. This is especially useful for viva discussion.
void printCompressionReport(const string& originalFile, const string& compressedFile) {
    long long originalSize = getFileSize(originalFile);
    long long compressedSize = getFileSize(compressedFile);

    cout << "\nCompression Report\n";
    cout << "Original file size   : " << originalSize << " bytes\n";
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

// Full compression handoff:
// main() -> compressFile() -> create GzipCompressor object -> call compress()
// -> gzip utility runs -> report is printed.
void compressFile() {
    string inputPath = getInputLine("Enter input file path              : ");
    if (inputPath.empty()) {
        throw runtime_error("Input file path cannot be empty.");
    }
    if (!fileExists(inputPath)) {
        throw runtime_error("Input file not found: " + inputPath);
    }

    string outputPath = getInputLine("Enter output compressed file path or folder: ");
    outputPath = resolveCompressedOutputPath(inputPath, outputPath);

    ICompressor* compressor = new GzipCompressor(inputPath, outputPath);
    try {
        compressor->compress();
        delete compressor;
    } catch (...) {
        delete compressor;
        throw;
    }

    cout << "\nCompression completed successfully.\n";
    cout << "Compressed file saved as: " << outputPath << "\n";
    printCompressionReport(inputPath, outputPath);
}

// Full decompression handoff:
// main() -> decompressFile() -> create GzipCompressor object -> call decompress()
// -> gzip restores the original bytes into the chosen output file.
void decompressFile() {
    string inputPath = getInputLine("Enter compressed file path         : ");
    if (inputPath.empty()) {
        throw runtime_error("Compressed file path cannot be empty.");
    }
    if (!fileExists(inputPath)) {
        throw runtime_error("Compressed file not found: " + inputPath);
    }

    string outputPath = getInputLine("Enter restored output file path or folder   : ");
    outputPath = resolveRestoredOutputPath(inputPath, outputPath);

    ICompressor* decompressor = new GzipCompressor(inputPath, outputPath);
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

// This verification step is independent from compression and decompression.
// It proves that the restored file is exactly the same as the original file.
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

// Menu shown at the start of every loop iteration in main().
void printMenu() {
    cout << "\n===========================================\n";
    cout << "   Real File Compression / Decompression   \n";
    cout << "===========================================\n";
    cout << "1. Compress a file\n";
    cout << "2. Decompress a file\n";
    cout << "3. Compare original and restored files\n";
    cout << "4. Exit\n";
}

int main() {
    // Main loop of the whole program.
    // It keeps showing the menu until the user chooses Exit.
    // All errors thrown anywhere below are caught here, so the program does not crash.
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
                break;
            } else {
                cout << "Invalid choice. Please enter 1, 2, 3, or 4.\n";
            }
        } catch (const exception& e) {
            cout << "\n[ERROR] " << e.what() << "\n";
        }
    }

    return 0;
}
