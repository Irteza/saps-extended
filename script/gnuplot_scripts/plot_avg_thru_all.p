# Gnuplot script file for plotting data in file "avg_thru_all.txt"
   # This file is called   plot_avg_thru_all.p
#      set   autoscale                        # scale axes automatically
   unset log                              # remove any log-scaling
   unset label                            # remove any previous labels

#   set terminal postscript eps enhanced color font 'Helvetica,12'
#      set output 'avg_thru_all.eps'

   set terminal pngcairo size 700,524 enhanced font 'Helvetica,12'
      set output 'avg_thru_all.png'

   set style fill solid border
      set boxwidth 0.9
      set style fill solid 1.00 border 0
      set style histogram errorbars gap 2 lw 1
      set style data histograms
#set style histogram cluster gap 1
	 set xtic auto
	 set ytic auto                          # set ytics automatically
      set xlabel "Load"
      set ylabel "Throughput (Mbps)"
      set key right top

# set label "Yield Point" at 0.003,260 
	 # set arrow from 0.0028,250 to 0.003,280
#  set xr [0.2:0.8]
	 # set yr [0.0:0.12]
   datafile = "../results/avg_thru_all.txt"

# plot datafile i 0 using 2:3:xtic(1) t "RPS", '' i 1 u 2:3:xtic(1) t "ECMP", '' i 2 u 2:3:xtic(1) t "WPS", '' i 3 u 2:3:xtic(1) t "WFCS", '' i 4 u 2:3:xtic(1) t "HPO", '' i 5 u 2:3:xtic(1) t "SPS"
#plot datafile i 0 using 2:3:xtic(1) t "RPS", '' i 1 u 2:3:xtic(1) t "ECMP", '' i 2 u 2:3:xtic(1) t "WPS", '' i 3 u 2:3:xtic(1) t "WFCS", '' i 4 u 2:3:xtic(1) t "SAPS"

plot datafile i 0 using 2:3:xtic(1) t "RPS", '' i 1 u 2:3:xtic(1) t "ECMP", '' i 2 u 2:3:xtic(1) t "WPS", '' i 3 u 2:3:xtic(1) t "WFCS", '' i 4 u 2:3:xtic(1) t "SAPS", '' i 5 u 2:3:xtic(1) t "SAPS-DAU"