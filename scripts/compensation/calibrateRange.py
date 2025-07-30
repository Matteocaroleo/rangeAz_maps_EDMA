#TODO fare la griglia più fine


import numpy as np
import matplotlib.pyplot as plt
import argparse
# Carica la matrice 4x1024 dal file (real e imag alternati)

(0.6123789873959634-0.790564340073529j)
(0.4528381180335873-0.891592753927374j)
(0.45050519411583945-0.8927738067812304j)
(0.2199655932205092-0.9755076308256894j)


parser = argparse.ArgumentParser()
parser.add_argument ("input", default="rangeMatrix.txt")
parser.add_argument ("output", default="figs/rangePlot.png")
parser.add_argument ("--from_column", default=0, type=int)
parser.add_argument ("--to_column", default=256, type=int)
parser.add_argument ("--angle", default="no", choices = ["yes", "no"])
args= parser.parse_args()
data_raw = np.loadtxt(args.input)  # shape (4, 2048) se real/imag alternati

# Ricostruisci la matrice complessa: 4 righe, 1024 colonne
real_part = data_raw[:, 0::2]
imag_part = data_raw[:, 1::2]
data_complex = real_part + 1j * imag_part  # shape: (4, 1024)
data_calibrated = data_complex

data_calibrated [0,:] = data_complex[0,:] #* (0.6123789873959634-0.790564340073529*1j)
data_calibrated [1,:] = data_complex[1,:] #* (0.4528381180335873-0.891592753927374*1j)
data_calibrated [2,:] = data_complex[2,:] #* (0.45050519411583945-0.8927738067812304*1j)
data_calibrated [3,:] = data_complex[3,:] #* (0.2199655932205092-0.9755076308256894*1j)
                                              
# Calcola l'intensità in dB
# intensity_db = 10 * np.log10(np.abs(data_calibrated) + 1e-12)  # aggiunto epsilon per evitare log(0)
intensity = np.abs(data_calibrated) 
angle = np.angle (data_calibrated, deg=True)
# Asse X: distanza SAPENDO CHE RISOLUZIONE è 4cm
print(angle)
np.savetxt("angle.txt", angle)

radar_res = 4e-2; #m
x = np.arange(radar_res*args.from_column, radar_res*(args.to_column), radar_res)  # da 0 a 1023
intensity_filtered = intensity[:,args.from_column:(args.to_column )] #+1 è perchè non include il 256
angle_filtered = angle[:,args.from_column:(args.to_column  )] #+1 è perchè non include il 256


# FOR CALIBRATION
print ("antenna1",angle[0,57], "antenna2", angle[1,57], "antenna3", angle [2,57], "antenna4", angle [3,57])

# Plot
plt.figure(figsize=(12, 6))
if args.angle == "no":
    for i in range(4):
        plt.plot(x, intensity_filtered[i], label=f'Antenna {i+1}')
else:
    for i in range(4):
        plt.plot(x, angle_filtered[i], label=f'Antenna {i+1}')


plt.title("FFT Range")
plt.xlabel("Range [m]")
if args.angle == "yes":
    plt.ylabel("fase (Lineare)")
else: 
    plt.ylabel ("intensità (lineare)")

plt.legend()
plt.tight_layout()
plt.grid(visible=True, which='both', axis='both')
plt.savefig(args.output, dpi=300)
plt.close()

