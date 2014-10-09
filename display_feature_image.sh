((k=203-$1))
export DISPLAY=90.5.2.$k:0.$2
cd ../global/pranav/
killall -9 project_original_image
killall -9 project_blank_image
sleep 2
./project_original_image&



