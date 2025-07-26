
# must be dependant on given argument
rm figs/expectedAzMap_filter.png
rm figs/radarAzMap_filter.png
rm figs/rangeMap.png



#range
python3 parserRange.py rangeRaw.txt --output rangeMatrix.txt
python3 plotterRange.py rangeMatrix.txt figs/rangeMap_filter.png --from_column 50 --to_column 100 

#azimuth: expected
python3 doFftAzFromRange.py rangeMatrix.txt --output azMatrix.txt
python3 plotterAzimuth.py azMatrix.txt --output figs/expectedAzMap_filter.png --from_row 100 --to_row 200 --from_column 32 --to_column 64

#azimuth: radar
python3 parserAzimuth.py azimuthRaw.txt --output radarAzMatrix.txt
python3 plotterAzimuth.py radarAzMatrix.txt --output figs/radarAzMap_filter.png --from_row 100 --to_row 200 --from_column 32 --to_column 64

echo "done"
