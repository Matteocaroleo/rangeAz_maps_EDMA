
# must be dependant on given argument
rm figs/*.png

#range
python3 parserRange.py rangeRaw.txt --output rangeMatrix.txt
python3 plotterRange.py rangeMatrix.txt figs/rangeMap.png 

#azimuth: expected
python3 doFftAzFromRange.py rangeMatrix.txt --output azMatrix.txt
python3 plotterAzimuth.py azMatrix.txt --output figs/expectedAzMap.png

#azimuth: radar
python3 parserAzimuth.py azRaw.txt --output radarAzMatrix.txt
python3 plotterAzimuth.py radarAzMatrix.txt --output figs/radarAzMap.png

echo "done"
