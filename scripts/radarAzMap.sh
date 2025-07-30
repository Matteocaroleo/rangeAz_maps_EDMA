rm figs/radarAzMap.png
rm figs/azAtTarget.png

#azimuth: from radar
python3 parserAzimuth.py measures/empty/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/empty/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/empty/azAtTarget.png --fftshift yes


echo "done"
