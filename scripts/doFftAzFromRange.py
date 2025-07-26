import numpy as np
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("input_filename")
parser.add_argument("--output", default="azimuthMatrix.txt")
parser.add_argument("--doshift", default="yes", choices=["yes", "no"])

args = parser.parse_args()
input_filename = Path(args.input_filename)

if not input_filename.exists():
    print ("ERROR: input file does not exists or has not been given")
    raise SystemExit(1)

# 1. Carica la matrice 4×1024
data = np.loadtxt(input_filename)  # shape: (4, 1024)

# 2. Trasponi → 1024×4 e fai zero padding 
data_t = data.T  # shape: (1024, 4)
#debug 


data_t = np.pad (data_t, pad_width=[(0,0),(0,60)], mode='constant')

# 3. Applica FFT + fftshift per riga
if args.doshift== "yes":
    fft_out = np.fft.fftshift(np.fft.fft(data_t, axis=1), axes=1)  # shape: (1024, 4)
elif args.doshift == "no":
    fft_out = np.fft.fft(data_t, axis=1)  # shape: (1024, 4)
else:
    print ("wrong argument for doshift")
    raise SystemExit(1)
print (fft_out)
# 4. Separa reali e immaginari per salvataggio
output_data = np.empty((fft_out.shape[0], fft_out.shape[1] * 2))
output_data[:, 0::2] = np.real(fft_out)
output_data[:, 1::2] = np.imag(fft_out)

# 5. Salva su file
np.savetxt(args.output, output_data, fmt="%.6f")

