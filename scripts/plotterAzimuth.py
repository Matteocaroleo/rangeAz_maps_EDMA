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

# 1. Carica matrice rangeAzimuth (1024 righe, 128 colonne = real+imag)
data_raw = np.loadtxt('azimuthMatrix.txt')  # shape: (1024, 128)

# 2. Ricostruisci la matrice complessa 1024×64
real = data_raw[:, 0::2]
imag = data_raw[:, 1::2]
data_complex = real + 1j * imag  # shape: (1024, 64)




# 3. Calcola modulo
magnitude = np.abs(data_complex)

# filtra righe e colonne in base a parametro passato
filtered_mag = magnitude[args.from_row:args.to_row,args.from_column:args.to_column]

print (filtered_mag.shape)

radar_res = 4e-2 #[m]

# 4. Plot heatmap in dB
plt.figure(figsize=(10, 6))
im = plt.imshow(
    filtered_mag,
    aspect='auto',
    norm = 'linear',
    cmap='inferno',
    origin='lower',
    extent = [-90, 90, radar_res*(args.from_row/2), radar_res*(args.to_row/2)] # si divide per 2 perchè sulle colonne c'erano sia reali che immaginari
)
plt.colorbar(im, label='Intensity')
plt.xlabel('Azimuth degrees [FAKE SCALE]')
plt.ylabel('Range [m]')
plt.title('Range–Azimuth Intensity ')
plt.tight_layout()

# 5. Salva la figura
plt.savefig(args.output_png, dpi=400)
plt.close()
