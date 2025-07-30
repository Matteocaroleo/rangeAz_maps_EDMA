
# must be dependant on given argument
rm figs/rangeMap.png
rm figs/expectedAzMap.png
rm figs/radarAzMap.png

#range
python3 parserRange.py rangeRaw.txt --output rangeMatrix.txt
python3 plotterRange.py rangeMatrix.txt figs/rangeMap.png --angle no --dB yes

#azimuth: expected
python3 doFftAzFromRange.py rangeMatrix.txt --output azMatrix.txt --doshift yes --round_int16 yes
python3 plotterAzimuth.py azMatrix.txt --output figs/expectedAzMap.png  --dB yes --from_row 0 --to_row 256 # to see 1 chirp only

#azimuth: radar
python3 parserAzimuth.py azimuthRaw.txt --output radarAzMatrix.txt
python3 plotterAzimuth.py radarAzMatrix.txt --output figs/radarAzMap.png --dB yes --fftshift yes --from_row 0 --to_row 256 # to see 1 chirp only 

echo "done"
