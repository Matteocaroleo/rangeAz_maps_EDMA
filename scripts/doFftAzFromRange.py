import numpy as np
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("input_filename")
parser.add_argument("--output", default="azimuthMatrix.txt")
parser.add_argument("--doshift", default="yes", choices=["yes", "no"])
parser.add_argument("--round_int16", default="yes", choices=["yes", "no"])
parser.add_argument("--calibrate", default="yes", choices=["yes","no"])
args = parser.parse_args()
input_filename = Path(args.input_filename)

if not input_filename.exists():
    print ("ERROR: input file does not exists or has not been given")
    raise SystemExit(1)

# 1. Carica la matrice 4×1024
data = np.loadtxt(input_filename)  # shape: (4, 1024)
print ("caricata:", data, data.shape)

real = data[:, 0::2]
imag = data[:, 1::2]
data_complex = real + 1j*imag 

#CALIBRATION
print ("CALIBRATION")
if args.calibrate == "yes":
    data_complex [0,:] = data_complex[0,:] * (0.6123789873959634-0.790564340073529*1j)
    data_complex [1,:] = data_complex[1,:] * (0.4528381180335873-0.891592753927374*1j)
    data_complex [2,:] = data_complex[2,:] * (0.45050519411583945-0.8927738067812304*1j)
    data_complex [3,:] = data_complex[3,:] * (0.2199655932205092-0.9755076308256894*1j)
    data_complex.real = np.int16(data_complex.real)
    data_complex.imag = np.int16(data_complex.imag)
    np.savetxt ("data/rangeCalibrated.txt", data_complex, fmt='%.4e')
    print ("IMMAGINARI:",data_complex.imag)
            

angle=np.angle(data_complex)

print ("antenna1",angle[0,57], "antenna2", angle[1,57], "antenna3", angle [2,57], "antenna4", angle [3,57])


print ("data complex:", data_complex, data_complex.shape)

# 2. Trasponi → 1024×4 e fai zero padding 
data_t = data_complex.T  # shape: (1024, 4)
#debug 
print ("trasposta:", data_t)

data_t = np.pad (data_t, pad_width=[(0,0),(0,60)], mode='constant')
print ("shape after padding")
print (data_t, data_t.shape)
# 3. Applica FFT + fftshift per riga
if args.doshift== "yes":
    fft_out = np.fft.fftshift(np.fft.fft(data_t, axis=1), axes=1)  # shape: (1024, 4)
elif args.doshift == "no":
    fft_out = np.fft.fft(data_t, axis=1)  # shape: (1024, 4)
else:
    print ("wrong argument for doshift")
    raise SystemExit(1)

print ("output of fft:")
print (fft_out)
# 4. Separa reali e immaginari per salvataggio
output_data = np.empty((fft_out.shape[0], fft_out.shape[1] * 2))
output_data[:, 0::2] = np.real(fft_out)
output_data[:, 1::2] = np.imag(fft_out)

# 5. Salva su file
print ("ORA ARROTONDA USCITA DA FLOAT A INTERI 16bit")
if args.round_int16 == "yes":
    np.savetxt(args.output, np.int16(output_data))
elif args.round_int16 == "no":
    np.savetxt(args.output, output_data)
else:
    print ("wrong argument to round")
    raise SystemExit(1)


