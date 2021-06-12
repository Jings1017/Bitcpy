reset
set title "comparsion"
set xlabel "data size"
set ylabel "time cost"
set term png enhanced font 'Verdana,10'
set output "result.png"
set xrange [0:8000]
set yrange [0:50000]
set xtic 0,1000
set ytic 0,10000
set key left
plot [:][:7892]'comp_data.txt' using 1:2 with linespoints linewidth 2 title 'original', \
'comp_data.txt' using 1:3 with linespoints linewidth 2 title 'version2' , \
