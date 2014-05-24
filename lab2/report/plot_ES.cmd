set term png
set output "E.png"

plot "plot_ES.dat" using 1:4 title 'dubina = 9' with lines

set output "S.png"

plot "plot_ES.dat" using 1:5 title 'dubina = 9' with lines 
