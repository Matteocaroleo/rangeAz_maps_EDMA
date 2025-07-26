import numpy as np
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument ("input", default="azimuthMatrix.txt")
parser.add_argument ("--output_png", default="figs/rangeAzMap.png")
parser.add_argument ("--from_row", default=0, type=int)
parser.add_argument ("--to_row", default=1024, type=int)
parser.add_argument ("--from_column", default=0, type=int)
parser.add_argument ("--to_column", default=64, type=int)

args=parser.parse_args()

# 1. Carica dado fft_output (1024 righe, 128 colonne = real+imag)
data_raw = np.loadtxt('azimuthMatrix.txt')  # shape: (1024, 128)

# 2. Ricostruisci la matrice complessa 1024×64
real = data_raw[:, 0::2]
imag = data_raw[:, 1::2]
data_complex = real + 1j * imag  # shape: (1024, 64)

# 3. Calcola modulo
magnitude = np.abs(data_complex)

filtered_mag = magnitude[args.from_row:args.to_row,args.from_column:args.to_column]

print (filtered_mag.shape)
# 4. Plot heatmap in dB
plt.figure(figsize=(10, 6))
im = plt.imshow(
    filtered_mag,
    aspect='auto',
    cmap='inferno',
    origin='lower'
)
plt.colorbar(im, label='Intensity (dB)')
plt.xlabel('Azimuth sample index')
plt.ylabel('Range sample index')
plt.title('Range–Azimuth Intensity (dB)')
plt.tight_layout()

# 5. Salva la figura
plt.savefig(args.output_png, dpi=300)
plt.close()
