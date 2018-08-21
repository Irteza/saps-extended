# Gnuplot script file for plotting data in file "afct_large.txt"
   # This file is called   plot_afct_large.p
#      set   autoscale                        # scale axes automatically
   unset log                              # remove any log-scaling
   unset label                            # remove any previous labels

#   set terminal postscript eps enhanced color font 'Helvetica,12'
#     set output 'afct_large.eps'

   set terminal pngcairo size 700,524 enhanced font 'Verdana,12'
      set output 'afct_large.png'

   set style fill solid border
      set boxwidth 0.9
      set style fill solid 1.00 border 0
      set style histogram errorbars gap 2 lw 1
      set style data histograms
#set style histogram cluster gap 1
	 set xtic auto
	 set ytic auto                          # set ytics automatically
      set xlabel "Load"
      set ylabel "AFCT (seconds)"
#set key 0.01,100
      set key left top

#  set xr [0.2:0.8]
	 # set yr [0.0:0.12]
      datafile = "../results/afct_large.txt"

#plot datafile index 0 using 2:3:xtic(1) title "RPS", '' index 1 using 2:3:xtic(1) title "ECMP", '' index 2 using 2:3:xtic(1) title "HPO", '' index 3 using 2:3:xtic(1) title "SPS"
#plot datafile i 0 using 2:3:xtic(1) t "RPS", '' i 1 u 2:3:xtic(1) t "ECMP", '' i 2 u 2:3:xtic(1) t "WPS", '' i 3 u 2:3:xtic(1) t "WFCS", '' i 4 u 2:3:xtic(1) t "SAPS"

   plot datafile i 0 using 2:3:xtic(1) t "RPS", '' i 1 u 2:3:xtic(1) t "ECMP", '' i 2 u 2:3:xtic(1) t "WPS", '' i 3 u 2:3:xtic(1) t "WFCS", '' i 4 u 2:3:xtic(1) t "SAPS", '' i 5 u 2:3:xtic(1) t "SAPS-DAU"
