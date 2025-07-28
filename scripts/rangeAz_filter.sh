
# must be dependant on given argument
rm figs/expectedAzMap_filter.png

rm figs/rangeMap_filter.png



#range
python3 parserRange.py rangeRaw.txt --output rangeMatrix.txt
python3 plotterRange.py rangeMatrix.txt figs/rangeMap_filter.png --from_column 30 --to_column 200 
echo "range done"

#azimuth: expected
python3 doFftAzFromRange.py rangeMatrix.txt --output azMatrix.txt --doshift yes --round_int16 no
python3 plotterAzimuth.py azMatrix.txt --output figs/expectedAzMap_filter.png --from_row 70 --to_row 100 --fftshift no
echo "expected az done"

#azimuth: radar
python3 parserAzimuth.py azimuthRaw.txt --output radarAzMatrix.txt
python3 plotterAzimuth.py radarAzMatrix.txt --output figs/radarAzMap_filter.png --from_row 70 --to_row 100 --fftshift yes
echo "az done"

echo "all done, results are in /figs"
