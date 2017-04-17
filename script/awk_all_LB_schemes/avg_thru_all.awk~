
BEGIN{
    total_lb = 4 ; ## "RPS" "ECMP" "WPS" "WFCS" "SPPS"
    lb_scheme[1] = "WFCS-P"
    lb_scheme[2] = "SPPS"
    lb_scheme[3] = "SPPS-DASO"
    lb_scheme[4] = "WFCS"
    #lb_scheme[5] = "SPPS"
    ##lb_scheme[6] = "SPFS"

    for (i = 1; i <= total_lb; i++) {
        sum_l20[i] = 0;
	sumsq_l20[i] = 0;
	count_l20[i] = 0;
        sum_l40[i] = 0;
	sumsq_l40[i] = 0;
	count_l40[i] = 0;
        sum_l60[i] = 0;
	sumsq_l60[i] = 0;
	count_l60[i] = 0;
        sum_l80[i] = 0;
	sumsq_l80[i] = 0;
	count_l80[i] = 0;
        sum_l90[i] = 0;
	sumsq_l90[i] = 0;
	count_l90[i] = 0;
    }
}
{

    l=0;
    i=1;
    while (i <= total_lb) {
	if($4==lb_scheme[i]) {
	    l = i;
	    break;
	}
	i++;
    }

	## 96

    if(l != 0) {
	if($1==0.2) {
	    sum_l20[l] += $96;
	    sumsq_l20[l] += ($96)^2;
	    count_l20[l] += 1;
	} else if ($1==0.4) {
	    sum_l40[l] += $96;
	    sumsq_l40[l] += ($96)^2;
	    count_l40[l] += 1;
	} else if ($1==0.6) {
	    sum_l60[l] += $96;
	    sumsq_l60[l] += ($96)^2;
	    count_l60[l] += 1;
	} else if ($1==0.8) {
	    sum_l80[l] += $96;
	    sumsq_l80[l] += ($96)^2;
	    count_l80[l] += 1;
	} else if ($1==0.9) {
	    sum_l90[l] += $96;
	    sumsq_l90[l] += ($96)^2;
	    count_l90[l] += 1;
	} else { }
    }    
} END {

    for (i = 1; i <= total_lb; i++) {
	lbname = lb_scheme[i];
	printf "#\"%s\"\n\n", lbname;
	printf "0.20 %f %f \n", sum_l20[i]/count_l20[i], sqrt((sumsq_l20[i]-sum_l20[i]^2/count_l20[i])/count_l20[i]);
	printf "0.40 %f %f \n", sum_l40[i]/count_l40[i], sqrt((sumsq_l40[i]-sum_l40[i]^2/count_l40[i])/count_l40[i]);
	printf "0.60 %f %f \n", sum_l60[i]/count_l60[i], sqrt((sumsq_l60[i]-sum_l60[i]^2/count_l60[i])/count_l60[i]);
	printf "0.80 %f %f \n", sum_l80[i]/count_l80[i], sqrt((sumsq_l80[i]-sum_l80[i]^2/count_l80[i])/count_l80[i]);
	printf "0.90 %f %f \n\n", sum_l90[i]/count_l90[i], sqrt((sumsq_l90[i]-sum_l90[i]^2/count_l90[i])/count_l90[i]);
	##print "%f %f \n", sum[i]/NR, sqrt((sumsq[i]-sum[i]^2/NR)/NR)
    }
}
#' response_times.log > results/avg_thru_all.txt
