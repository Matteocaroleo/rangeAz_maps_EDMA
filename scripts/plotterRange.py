#TODO fare la griglia più fine


import numpy as np
import matplotlib.pyplot as plt
import argparse
# Carica la matrice 4x1024 dal file (real e imag alternati)

parser = argparse.ArgumentParser()
parser.add_argument ("input", default="rangeMatrix.txt")
parser.add_argument ("output", default="figs/rangePlot.png")
args= parser.parse_args()
data_raw = np.loadtxt(args.input)  # shape (4, 2048) se real/imag alternati

# Ricostruisci la matrice complessa: 4 righe, 1024 colonne
real_part = data_raw[:, 0::2]
imag_part = data_raw[:, 1::2]
data_complex = real_part + 1j * imag_part  # shape: (4, 1024)

# Calcola l'intensità in dB
# intensity_db = 10 * np.log10(np.abs(data_complex) + 1e-12)  # aggiunto epsilon per evitare log(0)
intensity = np.abs(data_complex) 

# Asse X: distanza SAPENDO CHE RISOLUZIONE è 4cm

radar_res = 4e-2; #m
n_samples = 256;
x = np.arange(0, n_samples*radar_res, radar_res)  # da 0 a 1023

# Plot
plt.figure(figsize=(12, 6))
for i in range(4):
    plt.plot(x, intensity[i], label=f'Antenna {i+1}')

plt.title("FFT Range")
plt.xlabel("Range [m]")
plt.ylabel("Intensità (Lineare)")

plt.legend()
plt.tight_layout()
plt.savefig(args.output, dpi=300)
plt.close()
