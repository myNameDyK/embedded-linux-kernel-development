set terminal pngcairo size 1600,1000 enhanced font "DejaVu Sans,12"
set output "latency_histogram.png"

set multiplot layout 2,1 title \
"LAB-03: Scheduling Latency - Standard Kernel" \
font "DejaVu Sans,17"

set grid
set key top right
set xlabel "Latency (microseconds)"
set ylabel "Number of samples"

# Top: zoom into the typical latency range
set title "Latency Distribution: 0 - 80 us"
set xrange [0:80]
set yrange [0:*]

plot \
  "hist_standard.txt" using 1:($2+$3+$4+$5) \
    with lines linewidth 2 title "Baseline", \
  "hist_stress.txt" using 1:($2+$3+$4+$5) \
    with lines linewidth 2 title "Under stress"

# Bottom: all observed latency bins
set title "Full Latency Distribution and Deadline"
set xrange [0:5000]
set yrange [0.9:*]
set logscale y

set arrow 1 from 500, graph 0 to 500, graph 1 \
  nohead dt 2 linewidth 2 lc rgb "red"

set label 1 "500 us deadline" \
  at 600, graph 0.9 textcolor rgb "red"

plot \
  "hist_standard.txt" using 1:($2+$3+$4+$5) \
    with points pointtype 7 pointsize 0.7 title "Baseline", \
  "hist_stress.txt" using 1:($2+$3+$4+$5) \
    with points pointtype 5 pointsize 0.8 title "Under stress"

unset multiplot
