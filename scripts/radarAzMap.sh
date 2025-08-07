
#azimuth: from radar
python3 parserAzimuth.py measures/empty/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/empty/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/empty/azAtTarget.png --output_range_png measures/empty/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/empty_elevation/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/empty_elevation/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/empty_elevation/azAtTarget.png --output_range_png measures/empty_elevation/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/empty_right/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/empty_right/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/empty_right/azAtTarget.png --output_range_png measures/empty_right/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/person_elevation/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/person_elevation/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/person_elevation/azAtTarget.png --output_range_png measures/person_elevation/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/person/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/person/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/person/azAtTarget.png --output_range_png measures/person/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/person_right/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/person_right/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/person_right/azAtTarget.png --output_range_png measures/person_right/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/center_2m_elevation/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/center_2m_elevation/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/center_2m_elevation/azAtTarget.png --output_range_png measures/center_2m_elevation/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/center_2m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/center_2m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/center_2m/azAtTarget.png --output_range_png measures/center_2m/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/left_2m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/left_2m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/left_2m/azAtTarget.png --output_range_png measures/left_2m/rangeProfile.png --fftshift yes

python3 parserAzimuth.py measures/right_2m/azimuthRaw.txt --output data/radarAzMatrix.txt
python3 plotterAzimuth.py data/radarAzMatrix.txt --output measures/right_2m/radarAzMap.png --dB yes --fftshift yes --from_row 7 --to_row 100 # to see 1 chirp only 
python3 plotterLobe.py data/radarAzMatrix.txt --output_azimuth_png measures/right_2m/azAtTarget.png --output_range_png measures/right_2m/rangeProfile.png --fftshift yes

echo "done"
