python3 plotterRange.py compensation/rangeMatrix.txt figs/rangeMap_filter.png --from_column 55 --to_column 60 --angle no --dB yes

#python3 doFftAzFromRange.py compensation/rangeMatrix.txt --output compensation/azMatrix.txt --doshift yes --round_int16 no --calibrate no

#python3 plotterLobeForCalib.py compensation/azMatrix.txt --output_azimuth_png figs/beforeDspCalib.png --output_range_png figs/beforeDspCalibRange.png --fftshift no

python3 doFftAzFromRange.py compensation/rangeMatrix.txt --output compensation/azMatrix.txt --doshift yes --round_int16 no --calibrate yes

python3 plotterLobeForCalib.py compensation/azMatrix.txt --output_azimuth_png figs/afterCalib.png --output_range_png figs/afterCalibRange.png --fftshift no



