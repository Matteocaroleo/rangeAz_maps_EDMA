python3 plotterRange.py rangeMatrix.txt figs/rangeMap_filter.png --from_column 55 --to_column 60 --angle no --dB yes

python3 doFftAzFromRange.py rangeMatrix.txt --output azMatrix.txt --doshift yes --round_int16 no --calibrate no

python3 plotterLobe.py azMatrix.txt --output figs/beforeDspCalib.png --fftshift no

#python3 doFftAzFromRange.py rangeMatrix.txt --output azMatrix.txt --doshift yes --round_int16 no --calibrate yes

#python3 plotterLobe.py azMatrix.txt --output figs/afterCalib.png --fftshift no



