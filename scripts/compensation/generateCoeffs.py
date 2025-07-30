import numpy as np
import cmath
# degree


ant1=0.9117300036238558
ant2= 1.1008503514671852
ant3=1.1034651982591532
ant4=1.349017127079419

# e^(j*phi) where phi=ant1,2...

phase_coeff_1 = -ant1
mag_coeff_1 = 1
coeff_1 = mag_coeff_1 * cmath.exp(1j*phase_coeff_1)

print (coeff_1)


phase_coeff_2 = -ant2
mag_coeff_2 = 1
coeff_2 = mag_coeff_2 * cmath.exp(1j*phase_coeff_2)

print (coeff_2)

phase_coeff_3 = -ant3
mag_coeff_3 = 1
coeff_3 = mag_coeff_3 * cmath.exp(1j*phase_coeff_3)

print (coeff_3)

phase_coeff_4 = -ant4
mag_coeff_4 = 1
coeff_4 = mag_coeff_4 * cmath.exp(1j*phase_coeff_4)

print (coeff_4)
