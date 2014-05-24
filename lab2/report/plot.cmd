set term png
set output "times.png"

plot "plot.dat" using 1:2 title 'P = 1' with lines, "plot.dat" using 1:3 title 'P = 2' with lines, "plot.dat" using 1:4 title 'P = 3' with lines, "plot.dat" using 1:5 title 'P = 4' with lines, "plot.dat" using 1:6 title 'P = 5' with lines, "plot.dat" using 1:7 title 'P = 6' with lines, "plot.dat" using 1:8 title 'P = 7' with lines, "plot.dat" using 1:9 title 'P = 8' with lines

