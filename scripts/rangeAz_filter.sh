
# must be dependant on given argument
rm figs/expectedAzMap_filter.png
rm figs/radarAzMap_filter.png
rm figs/rangeMap_filter.png



#range
python3 parserRange.py rangeNoCalRaw.txt --output rangeMatrix.txt
python3 plotterRange.py rangeMatrix.txt figs/rangeMap_filter.png --from_column 33 --to_column  37 --angle no --dB yes
echo "range done"

#azimuth: expected
python3 doFftAzFromRange.py rangeMatrix.txt --output azMatrix.txt --doshift yes --round_int16 no
python3 plotterAzimuth.py azMatrix.txt --output figs/expectedAzMap_filter.png --from_row 33 --to_row 37 --fftshift no
echo "expected az done"

#azimuth: radar
python3 parserAzimuth.py azimuthRaw.txt --output radarAzMatrix.txt
python3 plotterAzimuth.py radarAzMatrix.txt --output figs/radarAzMap_filter.png --from_row 40 --to_row 150 --fftshift yes
echo "az done"

echo "all done, results are in /figs"
