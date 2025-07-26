import numpy as np

ROWS = 4
COLS = 256
RANGE_BIN_OBJ = 30  # Oggetto vicino (≈1.2m se 4cm/bin)
MARGINE = 1
FILENAME = "radar_matrix_top_left.txt"

# Crea matrice complessa
matrix = np.zeros((ROWS, COLS), dtype=np.complex64)

# Fase decrescente tra i canali per simulare oggetto a sinistra
phase_step = -(np.pi*3) / 4

for col in range(COLS):
    if abs(col - RANGE_BIN_OBJ) <= MARGINE:
        amplitude = 100
        for row in range(ROWS):
            phase = row * phase_step
            real = amplitude * np.cos(phase) + np.random.uniform(-5, 5)
            imag = amplitude * np.sin(phase) + np.random.uniform(-5, 5)
            matrix[row, col] = complex(real, imag)
    else:
        for row in range(ROWS):
            real = np.random.uniform(0, 2)
            imag = np.random.uniform(0, 2)
            matrix[row, col] = complex(real, imag)

# Scrivi su file
with open(FILENAME, 'w') as f:
    for row in matrix:
        line = '   '.join([f"{val.real:.2f} {val.imag:.2f}" for val in row])
        f.write(line + '\n')

print(f"File '{FILENAME}' generato con successo.")
