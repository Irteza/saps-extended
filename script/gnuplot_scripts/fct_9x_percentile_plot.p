# Gnuplot script file for plotting data in file "99_pct_fct.txt"
   # This file is called   fct_99_percentile_plot.p
#      set   autoscale                        # scale axes automatically
   unset log                              # remove any log-scaling
   unset label                            # remove any previous labels 
	 set style fill solid						  
	 set boxwidth 0.9
	 set style data histogram
	 set style histogram cluster gap 1
	 set xtic auto
	 set ytic auto                          # set ytics automatically
	 set title "99th Percentile FCTs, Partial Failure"
      set xlabel "Load"
      set ylabel "FCT (seconds)"
      # set key 0.01,100
	 # set label "Yield Point" at 0.003,260 
	 # set arrow from 0.0028,250 to 0.003,280
#  set xr [0.2:0.8]
	 # set yr [0.0:0.12]
	    datafile = "percentiles/99_pct_fct.txt"

	 plot datafile index 0 using 13:xtic(8) title "RPS", '' index 1 using 13:xtic(8) title "ECMP", '' index 2 using 13:xtic(8) title "HPO", '' index 3 using 13:xtic(8) title "SPS"


