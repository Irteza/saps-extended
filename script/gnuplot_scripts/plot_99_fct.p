# Gnuplot script file for plotting data in file "99_avg.txt"
   # This file is called   plot_99_fct.p
#      set   autoscale                        # scale axes automatically
   unset log                              # remove any log-scaling
   unset label                            # remove any previous labels

#   set terminal postscript eps enhanced color font 'Helvetica,22'
#   set output '99th_pct_fct.eps'

   set terminal pngcairo size 700,524 enhanced font 'Helvetica,12'
     set output '99th_pct_fct.png' 
   
      set style fill solid border
      set boxwidth 0.9
      set style fill solid 1.00 border 0
      set style histogram errorbars gap 2 lw 1
      set style data histograms
#set style histogram cluster gap 1
	 set xtic auto
	 set ytic auto                          # set ytics automatically
      set xlabel "Load"
      set ylabel "FCT (seconds)"
      # set key 0.01,100
      set key left top
      # set label "Yield Point" at 0.003,260 
	 # set arrow from 0.0028,250 to 0.003,280
#  set xr [0.2:0.8]
	 # set yr [0.0:0.12]
	 datafile = "../results/99_avg.txt"

#plot datafile i 0 using 2:3:xtic(1) t "  PS", '' i 1 u 2:3:xtic(1) t "  ECMP", '' i 2 u 2:3:xtic(1) t "  WFCS", '' i 3 u 2:3:xtic(1) t "  SAPS"

#plot datafile i 0 using 2:3:xtic(1) t "  PS" lc rgbcolor "black" lt 2 fs pattern 1, '' i 1 u 2:3:xtic(1) t "  ECMP" lc rgbcolor "red" lt 1 fs pattern 2, '' i 2 u 2:3:xtic(1) t "  WFCS" lc rgbcolor "blue" lt 3 fs pattern 4, '' i 3 u 2:3:xtic(1) t "  SAPS" lc rgbcolor "green" lt 2 fs pattern 3
 plot datafile i 0 using 2:3:xtic(1) t "RPS", '' i 1 u 2:3:xtic(1) t "ECMP", '' i 2 u 2:3:xtic(1) t "WPS", '' i 3 u 2:3:xtic(1) t "WFCS", '' i 4 u 2:3:xtic(1) t "SAPS", '' i 5 u 2:3:xtic(1) t "SAPS-DAU"
