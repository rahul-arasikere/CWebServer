#!/bin/sh

defaultDir=$1

tempFile="$(mktemp)"

printf "Content-type: text/html\n\n"

printf "<html>\n"
printf "\t<head>\n"
printf "\t\t<title>CS410 Webserver</title>\n"
printf "\t</head>\n"
printf "\t<body>\n"
printf "\t\t<h1 style=\"color:red;font-size:16pt;text-align:center;\">CS410 Webserver</h1>\n"
printf "\t\t<br />\n"
printf "\t\t<div style=\"text-align:center;\">\n"
ls -lR $defaultDir | awk '{printf "%s\n", substr($1,1,1)}' >${tempFile}

regular_files=$(cat ${tempFile} | grep - | wc -l)
directories=$(cat ${tempFile} | grep d | wc -l)
character=$(cat ${tempFile} | grep c | wc -l)
block=$(cat ${tempFile} | grep b | wc -l)
link=$(cat ${tempFile} | grep l | wc -l)
fifo=$(cat ${tempFile} | grep p | wc -l)
socket=$(cat ${tempFile} | grep s | wc -l)

echo "regular ${regular_files}" >${tempFile}
echo "directory ${directories}" >>${tempFile}
echo "character ${character}" >>${tempFile}
echo "block ${block}" >>${tempFile}
echo "link ${link}" >>${tempFile}
echo "fifo ${fifo}" >>${tempFile}
echo "socket ${socket}" >>${tempFile}
gnuplot -p -e "set term svg; set style data histograms; set style fill solid; set xlabel 'File type'; set ylabel 'Number of Files'; plot for [COL=2:2] '<&3' using COL:xticlabels(1) title 'Files in \"${defaultDir}\"'" 3<${tempFile} 2>/dev/null
rm ${tempFile}
printf "\t\t</div>\n"
printf "\t</body>\n"
printf "</html>\n"
