

#procedure to compute average queue length 
#proc queueLength {sum number file} { 
#    global ns qmonitor 
#    set time 0.1 
#    set len [$qmonitor set pkts_] 
#    set now [$ns now] 
#    set sum [expr $sum+$len] 
#    set number [expr $number+1] 
#    #write the average queue length in to a file 
#    puts $file "[expr $now+$time] [expr 1.0*$sum/$number]" 
#    $ns at [expr $now+$time] "queueLength $sum $number $file" 
#} 
#$ns at 0 "queueLength 0 0 $avgf"

set tick 0.00009;


#
# Function taken from them tcl/ex/simple-eqp.tcl file 
#
proc build-tcp { n0 n1 startTime } {
    global ns tick
    set tcp [new Agent/TCP]
    $ns attach-agent $n0 $tcp
	 
	$tcp set isElephant_ 1 ; # SMI 26-March ... Simply assuming that these flows will always be the elephants....

    set snk [new Agent/TCPSink]
    $ns attach-agent $n1 $snk

    $ns connect $tcp $snk

    set ftp [new Application/FTP]
    $ftp attach-agent $tcp
    $ns at $startTime "$ftp start"
    return $tcp
}

# SMI: 28-Dec-2015
# Create an unlimited TCP flow but with all details we need for LB_Scheme to run successfully...
#
proc build-tcp-more { src dest startTime tcp_src tcp_sink fid node_id pktSize dctcp flowBender FlowCell FlowcellSize RoundRobin FailureAware FailureRatio FailedLinkIndex FromFailedLeaf SelSpraying PoorFlow} {
    global ns tick sim_time

    set tcp [new Agent/$tcp_src]
    $ns attach-agent $src $tcp
    $tcp set isElephant_ 1 ; # SMI 26-March ... Simply assuming that these flows will always be the elephants....
    $tcp set starts $startTime
    $tcp set sess $fid
    $tcp set node $node_id
    $tcp set fid_ $fid
    $tcp set packetSize_ $pktSize

    # SMI 13-Dec-15
    $tcp set flowcell_ $FlowCell
    $tcp set flowcellSizePkts_ $FlowcellSize; # What is the size of the flowcell in packets?
    $tcp set roundRobin_ $RoundRobin ; ## To allow us to shift between probabilistic and deterministic (RR) flowcell spraying (14-Dec-15)
    $tcp set failureAware_ $FailureAware; ## 27-Dec-15 to allow to shift between failure aware and unaware
    $tcp set failureRatio_ $FailureRatio 
    $tcp set failedLinkIndex_ $FailedLinkIndex
    $tcp set fromFailedLeaf_ $FromFailedLeaf

    $tcp set selectiveSpraying_ $SelSpraying
    $tcp set poorPathFlow_ $PoorFlow

    puts "flowcell_ = [$tcp set flowcell_] ; flowcellSizePkts_ = [$tcp set flowcellSizePkts_] ; roundRobin_ = [$tcp set roundRobin_] failureAware_ = [$tcp set failureAware_] !!"
    puts "failureRatio_ = [$tcp set failureRatio_]; failedLinkIndex = [$tcp set failedLinkIndex_]; FromFailedLeaf = [$tcp set fromFailedLeaf_] SelectiveSpraying = [$tcp set selectiveSpraying_] poorPathFlow = [$tcp set poorPathFlow_] !!"

    set sink [new Agent/$tcp_sink]
    if {$dctcp} {
	$sink listen
    }
    $ns attach-agent $dest $sink
    $sink set fid_ fid

    $ns connect $tcp $sink

    set ftp [new Application/FTP]
    $ftp attach-agent $tcp
    $ns at $startTime "$ftp start"
    $ns at [expr $sim_time-0.01] "$ftp stop"

    ##$tcp set size $transfer_size
    ##$tcp set flow_size [expr $transfer_size/($pktSize+40)]

    ##puts "SMI-TEST: flowcell_=$FlowCell flowcellSizePkts_=$FlowcellSize roundRobin_=$RoundRobin FailureAware=$FailureAware !!"

    return $tcp
}


proc plotF {tcp remapped} {
	global ns tick

	set now [$ns now];
	set fid [$tcp set fid_];
	#set nodeID [$tcp set node_];
	set long [$tcp set isElephant_];
	
	#set fraction_of_marked_packets [$tcp set temp_alpha_];
	set fraction_of_marked_packets [$tcp set dctcp_alpha_];
	set bytes [$tcp set ndatabytes_];
	set sz [$tcp set size];

	set outfile $fid;
	append outfile "-$sz";
	if {$long == 1} {
		append outfile "-long.log";
	} else {
		append outfile "-short.log";
	}
	set ttl [$tcp set ttl_];
	set flh [open $outfile a];
	puts $flh "$now $bytes $ttl $fraction_of_marked_packets"
	close $flh;
	
	if {$bytes >= $sz} {
		return 0;
	}

	$ns at [expr $now+$tick] "plotF $tcp $remapped";
}


#
# Print periodic "i'am alive" message
#
proc print_time {interval} {
global ns 
        #puts stdout [format "\nTime: %.2f" [$ns now]]
        $ns at [expr [$ns now]+$interval] "print_time $interval"
}


#
# Dump the statistics of a (unidirectional) link periodically 
#
proc linkDump {link binteg pinteg qmon interval name linkfile util loss queue utilAll f_dropAll sIndex maxLink buf_bytes} {
	global ns
  set now_time [$ns now]
  $ns at [expr $now_time + $interval] "linkDump $link $binteg $pinteg $qmon $interval $name $linkfile $util $loss $queue $utilAll $f_dropAll $sIndex $maxLink $buf_bytes"
  set bandw [[$link link] set bandwidth_]
  set queue_bd [$binteg set sum_]
  set abd_queue [expr $queue_bd/[expr 1.*$interval]]
  set queue_pd [$pinteg set sum_]
  set apd_queue [expr $queue_pd/[expr 1.*$interval]]
  set utilz [expr 8*[$qmon set bdepartures_]/[expr 1.*$interval*$bandw]]    

  if {[$qmon set parrivals_] != 0} {
  	set drprt [expr [$qmon set pdrops_]/[expr 1.*[$qmon set parrivals_]]]
  } else {
  	set drprt 0
  }
  
	if {$utilz != 0} {;	# compute avg queueing delay based on Little's formula
		set a_delay [expr ($abd_queue*8*1000)/($utilz*$bandw)]
	} else {
		set a_delay 0.
	}

	if {($name!="T-A") && ($name!="H-T")} { 
		## do the normal output to $util, $loss, and $queue as well....
#   puts stdout [format "\nTime interval: %.2f-%.2f" [expr [$ns now] - $interval] [$ns now]]
		puts $linkfile [format "\nTime interval: %.5f-%.5f" [expr [$ns now] - $interval] [$ns now]]
		puts $linkfile [format "Link %s: Utiliz=%.3f LossRate=%.3f AvgDelay=%.2fms AvgQueue(P)=%.2f AvgQueue(B)=%.0f" $name $utilz $drprt $a_delay $apd_queue $abd_queue]			
	}

  #loss_sample, util_sample and queue_sample
  
  set av_qsize [expr [expr $abd_queue * 100] / $buf_bytes]
  set utilz [expr $utilz * 100]
  set drprt [expr $drprt * 100]

  set buf_pkts [expr $buf_bytes / 1000]

#        puts "Buffer Size (bytes) = $buf_bytes"
#        puts "Buffer Size (pkts) = $buf_pkts"

	if {($name!="T-A") && ($name!="H-T")} { 
		puts $util [format "%.5f %.3f" [$ns now] $utilz]
		puts $loss [format "%.5f %.3f" [$ns now] $drprt]
		puts $queue [format "%.5f %.3f" [$ns now] $apd_queue]
	#	puts $queue [format "%.5f   %.3f" [$ns now] $av_qsize]
	}

	if {$sIndex == 0} {
	  puts -nonewline $utilAll [format "%.5f %.3f" [$ns now] $utilz]
	  puts -nonewline $f_dropAll [format "%.5f %.3f" [$ns now] [$qmon set pdrops_]]
	} elseif {$sIndex == [expr $maxLink-1]} {
	  puts $utilAll [format " %.3f" $utilz]
 	  puts $f_dropAll [format " %.3f" [$qmon set pdrops_]]
	} else {
		puts -nonewline $utilAll [format " %.3f" $utilz]
 	  puts -nonewline $f_dropAll [format " %.3f" [$qmon set pdrops_]]
	}
    
  $binteg reset
  $pinteg reset
  $qmon reset
}


#
# Print the statistics of a flow
#
proc printFlow {f outfile fm interval} {
global ns 
#puts $outfile [format "FID: %d pckarv: %d bytarv: %d pckdrp: %d bytdrp: %d rate: %.0f drprt: %.3f" [$f set flowid_] [$f set parrivals_] [$f set barrivals_] [$f set pdrops_] [$f set bdrops_] [expr [$f set barrivals_]*8/($interval*1000.)] [expr [$f set pdrops_]/double([$f set parrivals_])] ]

# flow_id, rate and drprt,
#puts $outfile [format "%d %.0f  %.3f" [$f set flowid_] [expr [$f set barrivals_]*8/($interval*1000000.)] [expr [$f set pdrops_]/double([$f set parrivals_])] ]

puts $outfile [format "%d %.6f " [$f set flowid_] [expr [$f set barrivals_]*8/($interval*1000000.)] ]


####

set number 30
for {set a 0} { $a < $number} {incr a} {
	set fl "flow"
	append fl $a
	set flow($a) [open $fl a]

	if { [$f set flowid_] == $a } {
		puts $flow($a) [format "%.4f %.6f" [$ns now] [expr [$f set barrivals_]*8/($interval*1000000.)] ]
	}
	close $flow($a)
}

####

}


#
# Dump the statistics of all flows
#
proc flowDump {link fm file_p interval} {
global ns 

    $ns at [expr [$ns now] + $interval]  "flowDump $link $fm $file_p $interval"
        puts $file_p [format "\nTime: %.2f" [$ns now]] 
        set theflows [$fm flows]
        if {[llength $theflows] == 0} {
                return
        } else {
        	set total_arr [expr double([$fm set barrivals_])]
        	if {$total_arr > 0} {
                	foreach f $theflows {
                        	set arr [expr [expr double([$f set barrivals_])] / $total_arr]
                        	if {$arr >= 0.0001} {
				    printFlow $f $file_p $fm $interval
                        	}       
                        	$f reset
                	}       
                	$fm reset
        	}
        }
}



#
# Create "infinite-duration" FTP connection
#
proc inf_ftp {id src dst maxwin pksize starttm} {
global ns 
	set tcp [$ns create-connection TCP/Newreno $src TCPSink/DelAck $dst $id]
	set ftp [$tcp attach-source FTP]
  	$tcp set window_ 	$maxwin
  	$tcp set packetSize_ 	$pksize
	$ns at $starttm "$ftp start"
	return $tcp
}


#
# Create an Exponential On-Off source
#
proc build-exp-off { src dest pktSize burstTime idleTime rate id startTime } {
    global ns
    set cbr [new Agent/CBR/UDP]
    $ns attach-agent $src $cbr
    set null [new Agent/Null]
    $ns attach-agent $dest $null
    $ns connect $cbr $null
    set exp1 [new Traffic/Expoo]
    $exp1 set packet-size $pktSize
    $exp1 set burst-time  [expr $burstTime / 1000.0] 
    $exp1 set idle-time   [expr $idleTime / 1000.0]
    $exp1 set rate        [expr $rate * 1000.0]
    $cbr  attach-traffic  $exp1
    $ns at $startTime "$cbr start"
    $cbr set fid_      $id
    return $cbr
}


#
# Create Short-lived TCP flows
#
proc build-short-lived { src dest pktSize fid node_id startTime tcp_src tcp_sink transfer_size dctcp isElephant flowBender FlowCell FlowcellSize RoundRobin FailureAware FailureRatio FailedLeaf FailedLinkIndex FromFailedLeaf ToFailedLeaf SelSpraying HealthyPathOnly DA_SprayOnly PoorFlow NorthSouthFlow IntraRackFlow DAFlow SecondFailedLinkLeaf SecondFailedLinkSpine src_leaf_ind dst_leaf_ind} {
    global ns tick

    puts "inside: build-short-lived"

    # RealisticFailure FailureCase

    #set tcp [$ns create-connection $tcp_src $src $tcp_sink $dest fid]
    set tcp [new Agent/$tcp_src]
    set sink [new Agent/$tcp_sink]
    if {$dctcp} {
	$sink listen
    }
    $ns attach-agent $src $tcp
    $ns attach-agent $dest $sink
    $sink set fid_ fid
    $ns connect $tcp $sink
    #$tcp set window_ 10000
    $tcp set fid_ $fid
    set ftp [$tcp attach-source FTP]

    $tcp set starts $startTime
    $tcp set sess $fid
    $tcp set node $node_id
    $tcp set packetSize_ $pktSize
    $tcp set size $transfer_size
    #$tcp set flow_size [expr $transfer_size/1024]
    $tcp set flow_size [expr $transfer_size/1000] ; ## in KBs
    #$tcp set flow_size [expr $transfer_size/($pktSize+40)]

    $tcp set isElephant_ $isElephant ; # SMI 26-March

    # SMI 13-Dec-15
    $tcp set flowcell_ $FlowCell
    $tcp set flowcellSizePkts_ $FlowcellSize; # What is the size of the flowcell in packets?
    $tcp set roundRobin_ $RoundRobin ; ## To allow us to shift between probabilistic and deterministic (RR) flowcell spraying (14-Dec-15)
    $tcp set failureAware_ $FailureAware; ## 27-Dec-15 to allow to shift between failure aware and unaware
    $tcp set failureRatio_ $FailureRatio 

    $tcp set failedLinkLeaf_ $FailedLeaf ; ## added 25-feb-17

    $tcp set failedLinkIndex_ $FailedLinkIndex
    $tcp set fromFailedLeaf_ $FromFailedLeaf
    $tcp set toFailedLeaf_ $ToFailedLeaf ; ## 13-Mar-16

    $tcp set selectiveSpraying_ $SelSpraying
    $tcp set healthyPathOnly_ $HealthyPathOnly
    $tcp set DA_sprayOnly_ $DA_SprayOnly
    $tcp set poorPathFlow_ $PoorFlow

    $tcp set secondFailedLinkLeaf_ $SecondFailedLinkLeaf
    $tcp set secondFailedLinkSpine_ $SecondFailedLinkSpine
    ##$tcp set flowFacingMultipleFailures_ $FlowFacingMultipleFailures 
    $tcp set DA_Flow_ $DAFlow  

    $tcp set srcLeaf_ $src_leaf_ind
    $tcp set destLeaf_ $dst_leaf_ind

    $tcp set northSouthFlow_ $NorthSouthFlow
    $tcp set intraRackFlow_ $IntraRackFlow

    puts -nonewline "2-way: fcell_=[$tcp set flowcell_]; fcellSizePkts_=[$tcp set flowcellSizePkts_]; rRobin_=[$tcp set roundRobin_] fAware_=[$tcp set failureAware_] " ;
    ##puts -nonewline "failureRatio_=[$tcp set failureRatio_]; fLinkIndex=[$tcp set failedLinkIndex_]; FromFLeaf=[$tcp set fromFailedLeaf_]; ToFLeaf=[$tcp set toFailedLeaf_]; " ;
    puts "SelSpraying=[$tcp set selectiveSpraying_]; PoorFlow=[$tcp set poorPathFlow_]; NorthSouthFlow=[$tcp set northSouthFlow_] startTime=[$tcp set starts] size=[expr [$tcp set size]/1000.0]" ; 
    ## RealisticFailure=$RealisticFailure; FailureCase=$FailureCase;

    $ns at [$tcp set starts] "$ftp send [$tcp set size]"

    ##$tcp set flowbender_ $flowBender;  #to check if flowbender is running or not.
    ##if {$flowBender==1} {
    ##	$ns at [expr $startTime+$tick] "plotF $tcp 0";
    ##}

    return $tcp
}

proc flowLevel { num_hosts num_tors src_ind dst_ind } {

	if { $src_ind/$num_hosts == $dst_ind/$num_hosts } {
		return 1
	} elseif { $src_ind/($num_hosts*$num_tors) == $dst_ind/($num_hosts*$num_tors) } {
		return 2
	} else {
		return 3
	}
}



