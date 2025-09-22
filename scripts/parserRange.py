import numpy as np

def hex_to_signed_int16(val):
    return val - 0x10000 if val >= 0x8000 else val

def parse_words_from_file(filepath, endian='little'):
    """Parsa il file esadecimale in una lista di word int16."""
    byte_list = []
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.strip().split()
            for byte_str in parts:
                try:
                    byte_list.append(int(byte_str, 16))
                except ValueError:
                    continue

    if len(byte_list) < 2:
        raise ValueError("File troppo corto, servono almeno 2 byte.")

    words = []
    for i in range(0, len(byte_list) - 1, 2):
        b1, b2 = byte_list[i], byte_list[i + 1]
        word = (b2 << 8) | b1 if endian == 'little' else (b1 << 8) | b2
        words.append(hex_to_signed_int16(word))

    return words

def build_matrix(words, rows=4, cols=1024):
    required = rows * cols
    if len(words) < required:
        raise ValueError(f"Servono almeno {required} word, trovati {len(words)}.")
    return np.array(words[:required], dtype=np.int16).reshape((rows, cols))

def save_matrix(matrix, output_path):
    with open(output_path, 'w') as f:
        for row in matrix:
            f.write('\t'.join(str(x) for x in row) + '\n')
    print(f"✅ Matrice 4x512 salvata in '{output_path}'")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Parser esadecimale → matrice 1024x4 (int16)")
    parser.add_argument("filepath", help="File input esadecimale")
    parser.add_argument("--endian", choices=["little", "big"], default="little", help="Endianess")
    parser.add_argument("--output", default="matrix_4x512.txt", help="File di output")

    args = parser.parse_args()

    words = parse_words_from_file(args.filepath, endian=args.endian)
    
    # prendiamo solo 1 chirp
    matrix = build_matrix(words, rows=4, cols=512)
    save_matrix(matrix, args.output)
