# EASYCOMPRESS â€“ File Compression and Decompression System

EASYCOMPRESS is a beginner-friendly C++17 console application that compresses and decompresses files using **Run-Length Encoding (RLE)**. It also demonstrates file handling, a stack, a queue, searching, persistent history, and byte-by-byte file comparison.

The project uses only the C++ standard library. It does not use external compression libraries.

## Features

1. Compress text entered manually.
2. Decompress manually entered EASYCOMPRESS data.
3. Compress a file and save it with the `.rle` extension.
4. Decompress an `.rle` file and save the restored data as a separate `.txt` file.
5. Compare two files byte by byte.
6. View compression history.
7. Search history by original or compressed filename (case-insensitive substring search).
8. Add file paths to a FIFO compression queue and process them in order.
9. Undo the most recent manual text operation using a stack.
10. Display file sizes, bytes saved, compression percentage, compression ratio, and whether the output is smaller.

## Project structure

```text
EasyCompress/
â”œâ”€â”€ main.cpp
â”œâ”€â”€ compression.h
â”œâ”€â”€ compression.cpp
â”œâ”€â”€ decompression.h
â”œâ”€â”€ decompression.cpp
â”œâ”€â”€ file_handler.h
â”œâ”€â”€ file_handler.cpp
â”œâ”€â”€ history.h
â”œâ”€â”€ history.cpp
â”œâ”€â”€ structures.h
â”œâ”€â”€ structures.cpp
â”œâ”€â”€ history.txt
â””â”€â”€ README.md
```

There are **11 C++ source/header files**, one history storage file, and this README file.

## RLE file format

Each compressed file begins with this ASCII header:

```text
EASYCOMPRESS-RLE-1
```

The header is followed by records in this format:

```text
<count>:<two-digit-hex-byte>;
```

For example, the text:

```text
AAAAABBBCCDDDD
```

is represented as:

```text
EASYCOMPRESS-RLE-1
5:41;3:42;2:43;4:44;
```

`41`, `42`, `43`, and `44` are hexadecimal byte values for `A`, `B`, `C`, and `D`. Counts are decimal. Each record ends with a semicolon.

The encoded representation is unambiguous and can represent any byte value, including digits, spaces, line breaks, punctuation, and null bytes. The algorithm operates on **bytes**, not Unicode code points. UTF-8 files are therefore preserved byte-for-byte, although a repeated multi-byte UTF-8 character is treated as a sequence of individual bytes.

The format adds a header and record separators, so RLE is most effective when the input contains long runs of repeated bytes. Files with mostly unique bytes may become larger after compression. EASYCOMPRESS reports that result rather than hiding it.

## Data structures and algorithms

- **Strings:** hold original file bytes and encoded data.
- **Arrays:** fixed-size `std::array<char, 4096>` buffers are used during file comparison. The queue also uses a fixed array.
- **Stack:** `TextOperationStack` stores manual text compression operations. Undo removes the newest operation (LIFO). Undo is session-only and does not delete a saved `.rle` file.
- **Queue:** `CompressionQueue` is a circular, array-based queue with a capacity of 100 file paths. Files are processed FIFO.
- **Searching:** history search performs a linear, case-insensitive substring search over filenames.
- **RLE:** consecutive identical bytes are stored as a count and a hexadecimal byte value.
- **File handling:** files are read and written in binary mode to preserve all bytes and line endings.

## Requirements

- GCC / MinGW-w64 with C++17 support (GCC 9 or later is recommended for standard `<filesystem>` support).
- VS Code or another C++ editor is optional; the program is a standard console application.

## Compilation

Open a terminal in the `EasyCompress` folder and run:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp compression.cpp decompression.cpp file_handler.cpp history.cpp structures.cpp -o easycompress
```

On Windows, you can name the output executable `easycompress.exe`:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp compression.cpp decompression.cpp file_handler.cpp history.cpp structures.cpp -o easycompress.exe
```

## Run

Linux / macOS / WSL:

```bash
./easycompress
```

Windows PowerShell:

```powershell
.\easycompress.exe
```

Run the program with the current working directory set to the `EasyCompress` folder. The persistent `history.txt` file is read from and written to that folder.

## Using manual text input

For menu options **1. Compress Text** and **2. Decompress Text**, enter or paste the content one line at a time. Finish by entering this marker on a line by itself:

```text
.END
```

The marker is not included in the text. To enter an empty string, enter `.END` immediately.

## Output file naming and safety

- Compressing `notes.txt` creates `notes_compressed.rle` when that name is available.
- If the output already exists, EASYCOMPRESS tries `notes_compressed_1.rle`, then `_2`, and so on.
- Decompressing `notes_compressed.rle` creates `notes_compressed_decompressed.txt` when available.
- Original files are never selected as output files, and the file writer refuses to overwrite an existing path.
- Manual text compression uses a generated name such as `manual_text_compressed.rle`.

## Compression statistics

- **Space saved:** original size âˆ’ compressed size. This can be negative when RLE makes a file larger.
- **Compression percentage:** `((original size âˆ’ compressed size) / original size) Ã— 100`.
- **Compression ratio:** `original size / compressed size`, displayed as `x:1`.
- For an empty original file, the percentage is shown as `0.00%` to avoid division by zero. The encoded empty file still contains the format header.

## Testing checklist

Test with:

1. A file containing long repeated runs, such as `AAAAABBBBCCCC`.
2. A file containing mostly unique bytes, such as `ABCDEFGH`.
3. An empty file.
4. A file containing digits.
5. A file containing punctuation and symbols.
6. A file containing multiple lines and blank lines.
7. A large text file.
8. A malformed `.rle` file with a missing header or invalid record.

For each valid input, compress it, decompress the generated `.rle` file, and use **Compare Two Files** to verify that the original and restored files are byte-for-byte identical.

## Notes and limitations

- This is a teaching project, not a production compression utility. RLE may increase the size of many ordinary text files.
- The complete input file is loaded into memory before compression or decompression. Very large files may require substantial memory.
- The manual input terminator is `.END`; a line containing exactly `.END` cannot be entered as manual content using this interface.
- History is stored in a simple versioned text format. Filename fields are hex-encoded so tabs and other special bytes do not break the record structure