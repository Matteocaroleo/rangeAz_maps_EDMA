import numpy as np

# Parametri radar
ROWS = 4                # numero di canali azimutali
COLS = 256              # numero di range bin
RANGE_BIN_MURO = 125    # posizione del muro (a 5m con risoluzione 4cm)
MARGINE = 1             # occupazione range
FILENAME = "radar_matrix_small_azimuth.txt"

# Matrice complessa: righe = azimuth, colonne = range bin
matrix = np.zeros((ROWS, COLS), dtype=np.complex64)

# Costruzione della matrice
for col in range(COLS):
    if abs(col - RANGE_BIN_MURO) <= MARGINE:
        amplitude = 100
        for row in range(ROWS):
            # Fase irregolare tra canali per ridurre la coerenza angolare
            base_phase = row * (np.pi / 4)
            noise_phase = np.random.uniform(-np.pi / 4, np.pi / 4)
            phase = base_phase + noise_phase

            real = amplitude * np.cos(phase) + np.random.uniform(-5, 5)
            imag = amplitude * np.sin(phase) + np.random.uniform(-5, 5)
            matrix[row, col] = complex(real, imag)
    else:
        # Rumore di fondo
        for row in range(ROWS):
            real = np.random.uniform(0, 2)
            imag = np.random.uniform(0, 2)
            matrix[row, col] = complex(real, imag)

# Salvataggio su file di testo
with open(FILENAME, 'w') as f:
    for row in matrix:
        line = '   '.join([f"{val.real:.2f} {val.imag:.2f}" for val in row])
        f.write(line + '\n')

print(f"File '{FILENAME}' generato con successo.")
