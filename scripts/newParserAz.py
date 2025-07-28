import numpy as np

def parse_hex_dump_to_matrix(input_file, rows=1024, cols=64, endian='big'):
    """
    Legge un file di testo con valori esadecimali (dump di memoria),
    ignora la prima e l'ultima riga, e costruisce una matrice di int16 [rows x cols].

    Args:
        input_file: path al file
        rows: righe della matrice
        cols: colonne della matrice
        endian: 'big' o 'little'

    Returns:
        Numpy array shape (rows, cols), dtype=int16
    """
    with open(input_file, 'r') as f:
        lines = f.readlines()

    # Ignora prima e ultima riga
    hex_strs = []
    for line in lines[1:-1]:
        hex_strs.extend(line.strip().split())

    expected_words = rows * cols  # ogni parola = 2 byte
    if len(hex_strs) != expected_words * 2:
        raise ValueError(f"Byte totali errati: attesi {expected_words * 2}, trovati {len(hex_strs)}")

    # Ricostruisci i valori int16 gestendo l’endianess
    int16_list = []
    for i in range(0, len(hex_strs), 2):
        byte1 = int(hex_strs[i], 16)
        byte2 = int(hex_strs[i+1], 16)

        if endian == 'little':
            value = (byte2 << 8) | byte1  # LSB prima
        else:  # 'big'
            value = (byte1 << 8) | byte2  # MSB prima

        # Interpreta come signed 16-bit
        if value >= 0x8000:
            value -= 0x10000
        int16_list.append(value)

    # Converti in numpy array
    matrix = np.array(int16_list, dtype=np.int16).reshape((rows, cols))
    return matrix

# Esempio d'uso:
if __name__ == "__main__":
    mat = parse_hex_dump_to_matrix("azimuthRaw.txt", rows=1024, cols=64, endian='little')
    print("✅ Matrice caricata:", mat.shape)
    np.savetxt("parsed_matrix.txt", mat, fmt="%d", delimiter="\t")
