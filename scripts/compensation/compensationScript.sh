
python3 ../parserRange.py rangeNoCalRaw.txt --output rangeNoCalMatrix.txt

python3 ../plotterRange.py rangeNoCalMatrix.txt rangeMapNOCAL.png --angle no --dB yes

python3 ../doFftAzFromRange.py rangeNoCalMatrix.txt --output azMatrixNoCal.txt --doshift yes --round_int16 yes --calibrate no

python3 ../plotterAzimuth.py azMatrixNoCal.txt --output expectedAzMapNoCal.png  --dB yes --from_row 0 --to_row 256 # to see 1 chirp only

python3 plotterLobe.py azMatrix.txt --output_azimuth_png measures/empty/azAtTarget.png --output_range_png measures/empty/rangeProfile.png --fftshift yes



