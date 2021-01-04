#!/bin/sh


printf "Content-type: text/html\n\n"

printf "<html>\n"
printf "\t<head>\n"
printf "\t\t<title>CS410 Webserver</title>\n"
printf "\t</head>\n"
printf "\t<body>\n"
printf "\t\t<h1 style=\"color:red;font-size:16pt;text-align:center;\">CS410 Webserver</h1>\n"
printf "\t\t<br />\n"
printf "\t\t<div style=\"text-align:center;\">\n"

gnuplot -p -e "set term svg; plot \"./serverroot/sensorUpdate.csv\" title 'Sensor values'" 2>/dev/null
printf "\t\t</div>\n"
printf "\t</body>\n"
printf "</html>\n"