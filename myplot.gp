reset
set title "Time cost between original and simplify version"
set xlabel "Size of bits to copy"
set ylabel "Time cost of execution (nsec)"
set term png enhanced font 'Verdana,10'
set output "result.png"
set xrange [0:8000]
set yrange [0:8000]
set xtic 0,1000
set ytic 0,1000
set key bottom right
plot [:][:7892]'processed.txt' using 1:2 with linespoints linewidth 2 title 'original', \
'processed.txt' using 1:3 with linespoints linewidth 2 title 'simplify' , \
