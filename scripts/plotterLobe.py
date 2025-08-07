import numpy as np
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument ("input", default="azimuthMatrix.txt")
parser.add_argument ("--output_azimuth_png", default="figs/rangeAzMap.png")
parser.add_argument ("--output_range_png", default="figs/rangeMap.png")
parser.add_argument ("--fftshift", default="no", choices=["yes","no"])
parser.add_argument ("--dB", default="yes", choices=["yes", "no"])
args=parser.parse_args()

# 1. Carica matrice rangeAzimuth (1024 righe, 128 colonne = real+imag)
data_raw = np.loadtxt(args.input)  # shape: (1024, 128)
print ("LOADED MATRIX SHAPE:", data_raw.shape)
# 2. Ricostruisci la matrice complessa 1024×64
real = data_raw[:, 0::2]
imag = data_raw[:, 1::2]
data_complex = real + 1j * imag  # shape: (1024, 64)


if args.fftshift=="yes":
    data_complex=np.fft.fftshift(data_complex, axes=1)
elif args.fftshift!="no":
    print ("wrong argument to fftshift")
    raise SystemExit(1)

# 3. Calcola modulo
if args.dB == "yes":
    magnitude = 20*np.log10 (np.abs(data_complex) + 1e-12)
else:
    magnitude = np.abs(data_complex)

# filtra righe e colonne in base a parametro passato

# resolution
radar_res = 4e-2 #[m]
degrees = np.arange (start= -90, stop=90, step=180/64)

peak = 0
for  x in range (7, 255):
    temp = max(magnitude[x,:])
    if peak < temp :
        peak = temp
        row_num = x
        for y in range (0, 63):
            if magnitude [x,y] == peak:
                column_number = y



#AZIMUTH AT RANGE 
#db=np.arange (start=min(magnitude[row_num,:]),stop=max(magnitude[row_num,:]) + (max(magnitude[row_num,:])-min(magnitude[row_num,:])) / 20, step= (max(magnitude[row_num,:])-min(magnitude[row_num,:])) / 20)
db = np.arange (50, 80, 1)
print (peak)
# 4. Plot heatmap 
plt.figure(figsize=(10, 6))
plt.plot (degrees, magnitude[row_num,:])
plt.ylim (bottom=50, top=80)
plt.xlabel('Azimuth gradi')
plt.ylabel('Intensità [dB]')
plt.title('Azimuth al range del target a ' + str( row_num*radar_res) + ' metri')
plt.tight_layout()
plt.hlines (y=max(magnitude[row_num,:]) - 3, xmin = degrees[0], xmax = degrees[63], linestyle="--", label='-3dB')
plt.hlines (y=max(magnitude[row_num,:]) - 13, xmin = degrees[0], xmax = degrees[63], linestyle=":", label='-13dB')
plt.grid(which = "both",linestyle="--")
plt.xticks(ticks=degrees[0::4])
plt.yticks(ticks=db[0::2])
plt.legend()

# 5. Salva la figura
plt.savefig(args.output_azimuth_png, dpi=400)
plt.close()


# RANGE PROFILE
radar_res = 4e-2 # metri
radar_ang_res = 180/64 # degrees
meters = np.arange (radar_res*8, radar_res*101, radar_res)
plt.figure(figsize=(10, 6))
plt.plot (meters, magnitude[7:100,column_number])
plt.ylim (bottom=50, top=80)
plt.xlabel('Range [m]')
plt.ylabel('Intensity [dB]')
plt.title('Range at target azimuth ' + str(column_number*radar_ang_res - 90) + ' degrees')
plt.tight_layout()
plt.grid(which = "both",linestyle="--")
plt.xticks(ticks=meters[0::4])
plt.yticks(ticks=db[0::2])

# 5. Salva la figura
plt.savefig(args.output_range_png, dpi=400)
plt.close()




