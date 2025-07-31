
#azimuth: from radar
python3 parserAzimuth.py measures/empty/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/empty/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/empty/azAtTarget.png --fftshift yes

python3 parserAzimuth.py measures/center_2m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/center_2m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/center_2m/azAtTarget.png --fftshift yes

python3 parserAzimuth.py measures/center_1m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/center_1m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/center_1m/azAtTarget.png --fftshift yes

python3 parserAzimuth.py measures/left_2m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/left_2m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/left_2m/azAtTarget.png --fftshift yes

python3 parserAzimuth.py measures/right_2m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/right_2m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/right_2m/azAtTarget.png --fftshift yes

python3 parserAzimuth.py measures/left_45deg_1m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/left_45deg_1m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/left_45deg_1m/azAtTarget.png --fftshift yes

python3 parserAzimuth.py measures/right_45deg_1m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/right_45deg_1m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output measures/right_45deg_1m/azAtTarget.png --fftshift yes

echo "done"
