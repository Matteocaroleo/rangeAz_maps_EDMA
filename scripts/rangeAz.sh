
# must be dependant on given argument
rm figs/rangeMap.png
rm figs/expectedAzMap.png
rm figs/radarAzMap.png

#range
python3 parserRange.py data/rangeNoCalRaw.txt --output data/rangeMatrix.txt
python3 plotterRange.py data/rangeMatrix.txt figs/rangeMap.png --angle no --dB yes

#azimuth: expected
python3 doFftAzFromRange.py data/rangeMatrix.txt --output data/azMatrix.txt --doshift yes --round_int16 yes --calibrate no
python3 plotterAzimuth.py data/azMatrix.txt --output figs/expectedAzMap.png  --dB yes --from_row 0 --to_row 256 # to see 1 chirp only

#azimuth: radar
python3 parserAzimuth.py data/azimuthCalRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output figs/radarAzMap.png --dB yes --fftshift yes --from_row 0 --to_row 256 # to see 1 chirp only 

echo "done"
