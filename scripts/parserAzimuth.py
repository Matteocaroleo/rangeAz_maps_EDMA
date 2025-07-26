import numpy as np

def hex_to_signed_int16(val):
    return val - 0x10000 if val >= 0x8000 else val

def parse_words_from_file(filepath, endian='little'):
    """
    Legge un file esadecimale (1 byte = 2 cifre hex) e ritorna una lista di signed int16.
    """
    with open(filepath, 'r') as f:
        byte_list = []
        for line in f:
            parts = line.strip().split()
            for byte_str in parts:
                try:
                    byte_list.append(int(byte_str, 16))
                except ValueError:
                    print(f"⚠️ Byte non valido: {byte_str}")

    if len(byte_list) < 2:
        raise ValueError("File troppo corto, non contiene nemmeno un word.")

    words = []
    for i in range(0, len(byte_list) - 1, 2):
        b1, b2 = byte_list[i], byte_list[i + 1]
        word = (b2 << 8) | b1 if endian == 'little' else (b1 << 8) | b2
        words.append(hex_to_signed_int16(word))

    return words

def build_matrix(words, rows=512, cols=128):
    """
    Costruisce una matrice 512x64 dai valori letti.
    """
    required = rows * cols
    if len(words) < required:
        raise ValueError(f"Servono almeno {required} word, trovati {len(words)}.")
    return np.array(words[:required], dtype=np.int16).reshape((rows, cols))

def save_matrix(matrix, output_path):
    with open(output_path, 'w') as f:
        for row in matrix:
            f.write('\t'.join(str(x) for x in row) + '\n')
    print(f"✅ Matrice 512x4 salvata in: '{output_path}'")

# --- MAIN ---
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Parser esadecimale → matrice 512x4 (int16)")
    parser.add_argument("filepath", help="File input (dump esadecimale)")
    parser.add_argument("--endian", choices=["little", "big"], default="little", help="Endianess (default: little)")
    parser.add_argument("--output", default="matrix_512x4.txt", help="File di output")

    args = parser.parse_args()

    words = parse_words_from_file(args.filepath, endian=args.endian)
    matrix = build_matrix(words, rows=512, cols=128)
    save_matrix(matrix, args.output)
