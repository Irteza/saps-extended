### Size-Aware Load Balancing for Data Centers.... started by SMI on 25_mar_2015

## File Last Modified: 27-May-2015 6:32 PM (SMI)
## Author: SMI

source twoway_basic_functions.tcl


# input parameters
set mice_load    	[expr [lindex $argv 0]]; # diff mice loads
set av_fsize     	[expr [lindex $argv 1]]; # avg flow size in KB
set LB_SCHEME		[lindex $argv 2]; # CONGA? or UPNA
set sim_time     	[lindex $argv 3];
set SRC            	[lindex $argv 4]; # multiple values...
set TOPOLOGY 		[lindex $argv 5];
set fs_threshold	[expr [lindex $argv 6]];
set num_hosts_per_leaf	[lindex $argv 7]; ## serversPerRack: servers per ToR 
set numToRs		[lindex $argv 8]; ## number of ToR switches
set numAggs 		[lindex $argv 9]; ## number of Agg Switches 
set numCores 		[lindex $argv 10]; ## number of core switches
set link_delay 		[lindex $argv 11]; ## 
set queueSize 		[lindex $argv 12]; ## 
set dctcp_enable	[lindex $argv 13]; ## 
##set switch_Algo		[lindex $argv 13]; ## 
set min_rto		[lindex $argv 14]; ##
set K 			[lindex $argv 15]; # K-ary Fat-Tree # this dictates the form of the fat-tree 
set singleShortFlowSize [lindex $argv 16]; # in packets, just 1 flow
set run_i 		[lindex $argv 17]; # i_th run
set AlltoAllFlows [lindex $argv 18]; # all-to-all traffic generation
set simpleLeftRightFlows [lindex $argv 19]; # left-to-right traffic generation, all thru the core(s)
set simpleOneLeftRightFlow [lindex $argv 20]; # just 1 small TCP flow traffic generation
set pkSize [lindex $argv 21]; # packet size to be used in whole simulation
set webSearchCDF [lindex $argv 22] ; # if 0, then data mining CDF to be used, if 1, then web search to be used
set getQmonLinkLevelStats [lindex $argv 23]; # this is set to 1 if we want the Queue Monitoring at link level for agg->core links

set STATS_START 	[lindex $argv 24];
set STATS_INTR 		[lindex $argv 25];
set traceAll		[lindex $argv 26];

set FlowcellSize [lindex $argv 27]; ## Informs the flowcell size in Packets
set FailedLinkIndex [lindex $argv 28]; ## This tells the failed link index 23-Dec-15
set FailureRatio [lindex $argv 29]; ## This tells the ratio by which the failed link is degraded 23-Dec-15
set FailedLeaf [lindex $argv 30]; ## Not used as of now. 03-Jan-15
set FailureCase [lindex $argv 31]; ## 0 means no failure; 1 means partial failure in uplink; 2 means full failure
set TrafficSourceLeftmostRackOnly [lindex $argv 32]; ## Allows us to make sure traffic only comes from leftmost rack :: 17-feb-2016
set TrafficDestinationSingleRack [lindex $argv 33]; ## Allows us to make sure traffic only goes to leftmost rack in right subtree of topology :: 17-feb-2016
set RealisticFailure [lindex $argv 34]; ## 8-Mar
set FailureStartTime [lindex $argv 35]; ## 8-Mar
set FailureDuration [lindex $argv 36];  ## 8-Mar
set FailureDetectionDelayInRTT [expr [lindex $argv 37]];  ## 8-Mar
set TinyMiceThresh [lindex $argv 38];  ## 24-Mar-16 (Allow us to look at stats of very small mice separately)
set dctcp_K [expr [lindex $argv 39]]; ## 10-May-2016 (Have different values for DCTCP K threshold)
set DupAckThresh [expr [lindex $argv 40]]; ## 10-May-2016 (Have different values for Duplicate ACK threshold)

set NorthSouthTraffic [lindex $argv 41];  ## 3-June
set NorthSouthLoad [lindex $argv 42]; ## 4-June
set NorthSouthInterArrival [lindex $argv 43];  ## 3-June
set SPS_Thresh [lindex $argv 44];  ## 13-June
set SPS_PoorPathFlow_Multiplier [lindex $argv 45];  ## 13-June
set flow_trace [lindex $argv 46];  ## 15-June

set bw_endhostTor [lindex $argv 47];  ## 24-June
set bw_torAgg [lindex $argv 48];  ## 24-June
set bw_aggCore [lindex $argv 49];  ## 24-June

set debugMsgs [lindex $argv 50];  ## 24-June
set SPS_Thresh_GL2GL [lindex $argv 51]; ## July 13th
set AckOnSamePath [lindex $argv 52]; ## July 16th, 2016
set PutFailedLinkDown [lindex $argv 53]; ## July 30th, 2016

set MultipleFailure [lindex $argv 54] ; ## 18-Feb-17
set SecondFailedLinkLeaf [lindex $argv 55] ; ## 18-Feb-17
set SecondFailedLinkSpine [lindex $argv 56] ; ## 18-Feb-17
set NumFailures [lindex $argv 57] ; ## 20-Feb-17

set flowBender 0;

set LB_SCHEME1 $LB_SCHEME;

if {$MultipleFailure == 1} {
    if {$NumFailures == 2} {
	set failedLinkLeafs(0) $FailedLeaf
	set failedLinkSpines(0) $FailedLinkIndex
	set failedLinkLeafs(1) $SecondFailedLinkLeaf
	set failedLinkSpines(1) $SecondFailedLinkSpine
    }
}

#if {$LB_SCHEME == "FLOWBENDER"} {
#    set LB_SCHEME "ECMP";
#    if { $debugMsgs==1 } {
#	puts "FLOW BENDER RUNNING!!!!!!!!!!!!!"
#    }
#    set flowBender 1;
#}

#set SINK           [lindex $argv 5]; 
#set QUEUE          [lindex $argv 6];

set Details [open details.txt w]

#puts "I am alive and kicking!!!!"
puts "Num-of-Sims=$run_i; All-to-All=$AlltoAllFlows; Simple-Left-Right-Flows=$simpleLeftRightFlows;"

if {$flow_trace == 1} {
    set flow_trace_out [open "flow_trace_details.out" w]
}

puts $Details "load=$mice_load" 
puts $Details "Avg-F-Size=$av_fsize; LB_Scheme=$LB_SCHEME; Simulation_Time=$sim_time; Transport=$SRC;"
puts $Details "Topology=$TOPOLOGY; File-size_Threshold=$fs_threshold; Num-of-hosts-per-leaf=$num_hosts_per_leaf;"
puts $Details "Num-of-ToRs=$numToRs; Num-of-Aggs=$numAggs; Num-of-Cores=$numCores; Link-Delay=$link_delay; Queue-Size=$queueSize;"
puts $Details "DCTCP=$dctcp_enable; Min-RTO=$min_rto; K=$K; Single-Short-FlowSize=$singleShortFlowSize;"
puts $Details "Num-of-Sims=$run_i; All-to-All=$AlltoAllFlows; Simple-Left-Right-Flows=$simpleLeftRightFlows;"
puts $Details "simpleOneLeftRightFlow=$simpleOneLeftRightFlow; Packet-Size=$pkSize; web-Search-Traffic=$webSearchCDF;"
puts $Details "TrafficSourceLeftmostRackOnly=$TrafficSourceLeftmostRackOnly; TrafficDestinationSingleRack=$TrafficDestinationSingleRack;"
puts $Details "Qmon-Stats=$getQmonLinkLevelStats;"
puts $Details "RealisticFailure=$RealisticFailure; FailureStartTime=$FailureStartTime; FailureDuration=$FailureDuration;"
puts $Details "FailureDetectionDelayInRTT=$FailureDetectionDelayInRTT; TinyMiceThresh=$TinyMiceThresh;"
puts $Details "dctcp_K=$dctcp_K; DupAckThresh=$DupAckThresh;"
puts $Details "NorthSouthTraffic=$NorthSouthTraffic; NorthSouthLoad=$NorthSouthLoad; NorthSouthInterArrival=$NorthSouthInterArrival;"
puts $Details "SPS_Thresh=$SPS_Thresh;"
puts $Details "SPS_PoorPathFlow_Multiplier=$SPS_PoorPathFlow_Multiplier;"

close $Details

################# Switch Options ######################
set qLimit $queueSize; # queue limit for our links

if {$dctcp_enable == 1} {
    set switch_Algo "RED"
    set DCTCP_g_ 0.0625 ; # EWMA weight of alpha
    set K_dctcp $dctcp_K; ## 10-may-2016

    if { $debugMsgs==1 } {
	puts "DCTCP has been enabled!"
	puts "K (DCTCP) = $K_dctcp !"
    }
    Agent/TCP set ecn_ 1
    Agent/TCP set old_ecn_ 1
    Agent/TCP set slow_start_restart_ false ;
    Agent/TCP set windowOption_ 0 ; # IMP
    Agent/TCP set dctcp_ true ; # IMP adnan
    Agent/TCP set dctcp_g_ $DCTCP_g_; # IMP adnan
    #Agent/TCP set window_ 1256 ; # why bound?
    ##Agent/TCP set tcpTick_ 0.01 ; # tcpTick_
    ##Agent/TCP set minrto_ 0.01 ; # minRTO = 200ms & 10ms (To avoid incast)

    Queue/RED set bytes_ false
    Queue/RED set queue_in_bytes_ false ; # IMP
    Queue/RED set mean_pktsize_ [expr $pkSize+40] ; # IMP
    Queue/RED set setbit_ true
    Queue/RED set gentle_ false
    Queue/RED set q_weight_ 1.0
    Queue/RED set mark_p_ 1.0
    Queue/RED set thresh_ $K_dctcp
    Queue/RED set maxthresh_ $K_dctcp
    
    DelayLink set avoidReordering_ true ; # Why?

} else {
    if { $debugMsgs==1 } {
	puts "DCTCP has been disabled!"
    }
	set switch_Algo "DropTail"
	Queue/DropTail set queue_in_bytes_ true
	Queue/DropTail set mean_pktsize_ [expr $pkSize+40]
	Queue/DropTail set keep_order_ "true"
}

Queue set limit_ $qLimit

# TCP specific settings...
Agent/TCP set packetSize_ $pkSize
Agent/TCP set tcpTick_ 0.00009; # 90us RTT
Agent/TCP set rtxcur_init_ $min_rto; #Agent/TCP set rtxcur_init_ 0.03 ;
Agent/TCP set minrto_ $min_rto; ###Agent/TCP set minrto_ $RTO_min ;		# Default changed to 200ms on 
Agent/TCP set maxrto_ 2; #Agent/TCP set maxrto_ 1
Agent/TCP/FullTcp set segsize_ $pkSize

Agent/TCP set numdupacks_ $DupAckThresh ; ## Added on 10-May-2016, will allow us to modify this threshold for managing reordering

if {$qLimit > 12} {
   Agent/TCP set maxcwnd_ [expr $qLimit - 1];
} else {
   Agent/TCP set maxcwnd_ 12;
}



if {$SRC == "TCP/Sack1"} {
	set SINK "TCPSink/Sack1"
} elseif {$SRC == "TCP/FullTcp/Sack"} {
	set SINK $SRC
} else {
	set SINK "TCPSink"
	## "TCPSink/Sack1/DelAck" ## TCPSink/DelAck
}

set simpleFewFTPflows 0
set shortFlowsInterRackThruAggs 0; # this flag indicates if we want to build flows that have the Agg Switch as their root

# Topology details
set nodeIdOffset 1000; # this helps to separate NodeIds for different levels (i.e. endhost=0, leaf=1, spine=2)

## Default values for important variables for the flowcell and packet spraying cases - SMI: 3-Jan-2016
set FlowCell 0;
set RoundRobin 0;
set FailureAware 0;
set SelectiveSpraying 0; ## Default value
set HealthyPathOnly 0; ## Default Value
set DynamicMapping 0; ## Default value (16-June-2016)
set DA_SprayOnly 0; ## Default state ... 15 July 2016
set DA_HashSome 0; ## Default Case

if {$LB_SCHEME == "CONGA"} {
    if { $debugMsgs==1 } { 
    	puts "CONGA chosen!"
    }
    Node set NumHostsPerLeaf $num_hosts_per_leaf;
    Node set K $K; # this propagates value of K to C++ level (K in Node class is type static)
    Node set NodeIdOffset $nodeIdOffset; # propagate to C++ classes	
    ##Node set T_fl 0.008 ; # Setting it about 4 times the value of tau
    ##Node set T_fl 0.0005 ; # Setting it about 1/4 the value of tau
    Node set T_fl 0.00035 ; # Setting it about less than 1/4 the value of tau
    ##Node set T_fl 0.00025 ; # Setting it about 1/8 the value of tau
    # larger than the network RTT perhaps recommended .... 0.0005
    Queue/DropTail set conga_tau 0.0018 ; # Set on 20-March
    # Just set like this for the time being
    Queue/DropTail set conga_alpha 0.5 ; # Set on 20-March

    ## set perflow_ 0 ; ns-default.tcl
    Classifier/MultiPath set packetSpraying_ 0;

    # To let DropTail know about leaf spine BW, it will pass it onto DRE constructor (to help quantize X)
    Queue/DropTail set link_BW_leaf_spine $bw_torAgg ; # Added on Feb-10 by SMI

} elseif {$LB_SCHEME == "SAPS"} {
    if { $debugMsgs==1 } {
	puts "SAPS chosen!" ; # set perflow_ 0 ; ns-default.tcl
    }
    Classifier/MultiPath set packetSpraying_ 0;
} elseif {$LB_SCHEME == "ECMP"} {
    if { $debugMsgs==1 } {
	puts "ECMP chosen!"; 
    }
    Classifier/MultiPath set perflow_ 1 ; #Shuang!
    Classifier/MultiPath set packetSpraying_ 0;
} elseif {$LB_SCHEME == "FLOWBENDER"} {
    if { $debugMsgs==1 } {
	puts "FLOWBENDER chosen!"; 
    }
    Classifier/MultiPath set perflow_ 1 ; #Shuang!
    Classifier/MultiPath set packetSpraying_ 0;
    Agent/TCP set flowBender_ 1; 
} elseif {$LB_SCHEME == "RPS"} {
    if { $debugMsgs==1 } {
	puts "RPS chosen!"; # set perflow_ 0 ;  ns-default.tcl
    }
    Classifier/MultiPath set packetSpraying_ 1;
} elseif {$LB_SCHEME == "WFCS"} {
    set RoundRobin 1; # deterministic
    Classifier/MultiPath set flowcellSpraying_ 1;
    set FlowCell 1;
    set FailureAware 1;
} elseif {$LB_SCHEME == "WFCS-P"} {
    Classifier/MultiPath set flowcellSpraying_ 1; # probabilistic weighted flowcell spraying
    set FlowCell 1;
    set FailureAware 1;
} elseif {$LB_SCHEME == "UFCS"} {
    set RoundRobin 1; # deterministic
    Classifier/MultiPath set flowcellSpraying_ 1;
    set FlowCell 1;
} elseif {$LB_SCHEME == "UFCS-P"} {
    Classifier/MultiPath set flowcellSpraying_ 1; # probabilistic unweighted flowcell spraying 
    set FlowCell 1;
} elseif {$LB_SCHEME == "WPS"} {
    set RoundRobin 1; # deterministic
    Classifier/MultiPath set flowcellSpraying_ 1; # For now, we use flowcell spraying, with flowcell-size = 1; this is weighted packet spraying
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
} elseif {$LB_SCHEME == "WPS-P"} {
    Classifier/MultiPath set flowcellSpraying_ 1; # For now, we use flowcell spraying, with flowcell-size = 1; this is weighted packet spraying - probabilistic
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
} elseif {$LB_SCHEME == "SPFS"} {
    ## Indicates Selective-Path Flowcell Spraying
    set RoundRobin 1; # deterministic
    set FlowCell 1;
    set FailureAware 1;
    set SelectiveSpraying 1;
    ##set FlowcellSize 25;
    set DynamicMapping 1;

    Agent/TCPSink set ACK_onSamePath_ $AckOnSamePath ; ## SMI 16 Jul 16
 
    if {$RealisticFailure==1} {
	Classifier/MultiPath set packetSpraying_ 1;
	Classifier/MultiPath set selectiveSpraying_ 1;
    } else {
	Classifier/MultiPath set flowcellSpraying_ 1;
    }

} elseif {$LB_SCHEME == "SPPS"} {
    ## Indicates Selective-Path Packet Spraying
    set RoundRobin 1; # deterministic
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
    set SelectiveSpraying 1;

    Agent/TCPSink set ACK_onSamePath_ $AckOnSamePath ; ## SMI 16 Jul 16

    if {$RealisticFailure==1} {
	Classifier/MultiPath set packetSpraying_ 1;
	Classifier/MultiPath set selectiveSpraying_ 1;
    } else {
	Classifier/MultiPath set flowcellSpraying_ 1;
    }
} elseif {$LB_SCHEME == "SPPS-DASO"} {
    ## SPPS-DASO ~ Delay Asymmetry flows Sprayed Only (that too across all spines, not just healthy spines)

    set RoundRobin 1; # deterministic
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
    set SelectiveSpraying 1;
    set DA_SprayOnly 1;

    Agent/TCPSink set ACK_onSamePath_ $AckOnSamePath ; ## SMI 16 Jul 16

    if {$RealisticFailure==1} {
	Classifier/MultiPath set packetSpraying_ 1;
	Classifier/MultiPath set selectiveSpraying_ 1;
    } else {
	Classifier/MultiPath set flowcellSpraying_ 1;
    }

} elseif {$LB_SCHEME == "SPPS-DASH"} {
    ## SPPS-DASO ~ Delay Asymmetry flows Sprayed all spines, or Hashed

    set RoundRobin 1; # deterministic
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
    set SelectiveSpraying 1;
    set DA_SprayOnly 1;
    set DA_HashSome 1;

    Agent/TCPSink set ACK_onSamePath_ $AckOnSamePath ; ## SMI 16 Jul 16

    if {$RealisticFailure==1} {
	Classifier/MultiPath set packetSpraying_ 1;
	Classifier/MultiPath set selectiveSpraying_ 1;
    } else {
	Classifier/MultiPath set flowcellSpraying_ 1;
    }

} elseif {$LB_SCHEME == "SPS"} {
    ## Indicates Selective-Path Spraying with Dynamic Mapping
    set RoundRobin 1; # deterministic
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
    set SelectiveSpraying 1;
    set DynamicMapping 1;

    Agent/TCPSink set ACK_onSamePath_ $AckOnSamePath ; ## SMI 16 Jul 16

    if {$RealisticFailure==1} {
	Classifier/MultiPath set packetSpraying_ 1;
	Classifier/MultiPath set selectiveSpraying_ 1;
    } else {
	Classifier/MultiPath set flowcellSpraying_ 1;
    }
} elseif {$LB_SCHEME == "SPS-DASO"} {
    ## SPS with Delay Asymmetry flows Sprayed Only (that too across all spines, not just healthy spines)
    ## Indicates Selective-Path Spraying with Dynamic Mapping
    set RoundRobin 1; # deterministic
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
    set SelectiveSpraying 1;
    set DynamicMapping 1;
    set DA_SprayOnly 1;

    Agent/TCPSink set ACK_onSamePath_ $AckOnSamePath ; ## SMI 16 Jul 16

    if {$RealisticFailure==1} {
	Classifier/MultiPath set packetSpraying_ 1;
	Classifier/MultiPath set selectiveSpraying_ 1;
    } else {
	Classifier/MultiPath set flowcellSpraying_ 1;
    }
} elseif {$LB_SCHEME == "HPO"} {
    ## Indicates Healthy Path-Only Packet Spraying
    set RoundRobin 1; # deterministic
    set FlowCell 1;
    set FlowcellSize 1;
    set FailureAware 1;
    set SelectiveSpraying 1;
    set HealthyPathOnly 1;

    if {$RealisticFailure==1} {
	Classifier/MultiPath set packetSpraying_ 1;
	Classifier/MultiPath set selectiveSpraying_ 1;
    } else {
	Classifier/MultiPath set flowcellSpraying_ 1;
    }
} else {
    if { $debugMsgs==1 } {
	puts "Unknown LB Scheme!"
    }
    Classifier/MultiPath set packetSpraying_ 1; # just setting this for default case
}

#puts "I am alive and kicking!!!! 2"

set num_of_links 8 ; ## Leaf-Spine
Agent/TCP set numUplinks $numAggs ; ## Informs TCP of the number of uplinks for spraying purposes SMI: 9-Mar-16 // May change when we implement for Fat-Tree (multiple levels)
Agent/TCPSink set numUplinks $numAggs ; ## Informs TCP of the number of uplinks for spraying purposes SMI: 9-Mar-16 // May change when we implement for Fat-Tree (multiple levels)

if {$TOPOLOGY=="FATTREE"} {
    Classifier/MultiPath set K_ $K; # this enables the Modified ECMP code to work.....
    set num_of_links 12 ; ## Fat-Tree
} else {
    Classifier/MultiPath set K_ 0; # just to make sure the Modified ECMP code does not execute for leaf-spine
}

Classifier/MultiPath set failureCase_ $FailureCase; ## specially used for full failure, i.e. case 2 ## Jul-30, 2016

## 9-Mar-2016 SMI ##
set networkRTT [expr $link_delay * $num_of_links / 1000] ; ## Need to convert it to seconds from ms
set FailureDetectionDelay [expr $FailureDetectionDelayInRTT * $networkRTT] ; ## Now this is in terms of seconds

Agent/TCP set realisticFailure_ $RealisticFailure
Agent/TCP set failureCase_ $FailureCase
Agent/TCP set failureStartTime_ $FailureStartTime
Agent/TCP set failureDuration_ $FailureDuration
Agent/TCP set failureDetectionDelay_ $FailureDetectionDelay

## 18-Feb-17 :: Multiple Failures...
Agent/TCP set multipleFailure_ $MultipleFailure
Agent/TCP set numFailures_ $NumFailures
#Agent/TCP set secondFailedLinkLeaf_ $SecondFailedLinkLeaf
#Agent/TCP set secondFailedLinkSpine_ $SecondFailedLinkSpine

if {$DynamicMapping==1} {
    Agent/TCP set dynamicMapping_ 1;
    Agent/TCP set dynamicMappingThreshold_ [expr $SPS_Thresh * 1000]
    Agent/TCP set dynamicMappingThresholdGL2GL_ [expr $SPS_Thresh_GL2GL * 1000]
}

Agent/TCPSink set realisticFailure_ $RealisticFailure

## 9-Mar-2016 SMI ##

if { $debugMsgs==1 } {
    puts "DEBUG: FlowcellSize=$FlowcellSize; RoundRobin=$RoundRobin; FailedLinkIndex=$FailedLinkIndex; FailureRatio=$FailureRatio; FailureAware=$FailureAware; FailureCase=$FailureCase; "
    puts "DEBUG: RealisticFailure=$RealisticFailure; FailureStartTime=$FailureStartTime; FailureDuration=$FailureDuration; FailureDetectionDelay=$FailureDetectionDelay;"
}

# Statistics
set Out [open Out.ns w]

####	FUNCTIONS BODIES STARTED

#puts "I am alive and kicking!!!! 3"

# this procedure creates the 2-way links between each child and parent node
Simulator instproc makeLink {childNode parentNode linkBW switchAlgo linkDelay pktSize qLimit qmonIndex} {
	global ns 
	$ns duplex-link $childNode $parentNode [expr $linkBW]Mb [expr $linkDelay]ms $switchAlgo; # not RED, rather it is DropTail
	$ns queue-limit $childNode $parentNode $qLimit
}

# TODO: Build this....
Agent/$SRC instproc done {} {
    global tcp ptcp rtcp stcp rstcp ptcp rptcp flows_n ns s_arrival arrival_t short_tcp Out ftp SRC tr TinyMiceThresh debugMsgs
    set duration [expr [$ns now] - [$self set starts]]

    if { $debugMsgs==1 } {
	puts "now = [$ns now] ; startTime = [$self set starts] ; duration = $duration ; packets=[$self set ndatapack_] !!"
    } 
    set flowEndState 0; ## Was originally set to -1, but now set to 0, so we can use sub category sums for overall tinyMice AFCT, etc ## 2-June-16
    set tinyMice 0;

    if { [$self set realisticFailure_] == 1} {
	if { [$self set failureCase_] == 1 || [$self set failureCase_] == 2 } {
	    ## 2-June-16: Added the case of failureCase_ == 2 since we need this for Full Link Failure as well
	    if { [$ns now] < [$self set failureStartTime_] } {
		set flowEndState 0; ## endBeforeFailure
	    } else {
		if { [$ns now] < [expr [$self set failureStartTime_] + [$self set failureDetectionDelay_]] } {
		    set flowEndState 1; ## endAfterFailureOccurred
		} else {
		    if { [$self set failureDuration_] == 0 } {
			set flowEndState 2; ## endAfterFailureDetected
		    } else {
			if { [$ns now] < [expr [$self set failureStartTime_] + [$self set failureDuration_]] } {
			    set flowEndState 2; ## endAfterFailureDetected
			} else {
			    if { [$ns now] < [expr [$self set failureStartTime_] + [$self set failureDuration_] + [$self set failureDetectionDelay_]] } {
				set flowEndState 3; ## endAfterFailureFixed
			    } else {
				set flowEndState 4; ## endAfterRestorationDetected
			    }
			}
		    }
		}
	    }
	}
    }

    if { [$self set isElephant_] == 0} {
	if { [expr [$self set size] / 1000.0] < $TinyMiceThresh } {
	    set tinyMice 1;
	}
    }

##    puts "FORMAT: node sess startTime currTime flowDur dataPkts dataBytes rexmitBytes throughput cwnd_cuts1 rexmit isElephant rexmitPkts dup_count1 dup_count2 fromFailedLeaf necn_responses selSpraying \
##poorPathFlow flowEndState tinyMice toFailedLeaf northSouthFlow originallyHashed intraRackFlow "

    puts $Out "[$self set node] \t [$self set sess] \t [$self set starts] \t\
		   [$ns now] \t $duration \t [$self set ndatapack_] \t\
		   [$self set ndatabytes_] \t [$self set nrexmitbytes_] \t\
		   [expr [$self set ndatabytes_]/$duration] \t\
		   [$self set ncwndcuts1_] \t [$self set nrexmit_]\t [$self set isElephant_]\t [$self set nrexmitpack_]\t\
		   [$self set dupCase1Count_] \t [$self set dupCase2Count_] \t [$self set fromFailedLeaf_] \t [$self set necnresponses_]\t\
                   [$self set selectiveSpraying_]\t [$self set poorPathFlow_]\t $flowEndState \t $tinyMice \t [$self set toFailedLeaf_] \t [$self set northSouthFlow_] \t [$self set originallyHashed_]\t\
                   [$self set intraRackFlow_]\t [$self set size]"
 
    if { $SRC == "TCP/CTP" } {
	set level_ [$self set level]
    }
}

proc curr_time {} {
    #puts "In current time proc!"
    global ns debugMsgs
    set now [$ns now]
    if { $debugMsgs==1 } {
	puts "Current Time: $now"
    }
}

## TODO: Would need to write this anew.....
proc record {} {

}

#Define a 'finish' procedure
proc finish {} {

#      global file_CBQ_allQ_util file_CBQ_allQ_drops file_CBQ_allQ_avgqlen file_CBQ_allQ_pkts numClasses Out SRC mice_load fq_mon f_util f_loss f_queue SRC #f_u_t f_u_e num_cores num_tors num_aggs num_hosts paseArbInvokerTor paseArbInvokerHost getQmonLinkLevelStats getQmonQueueLevelStats host

    global Out SRC LB_SCHEME LB_SCHEME1 TOPOLOGY av_fsize fs_threshold mice_load getQmonLinkLevelStats ns K numToRs numAggs dctcp_K DupAckThresh fq_mon f_util f_loss f_queue f_utilAll f_utilAllToRAgg f_utilAllHostToR f_utilAllHostLeaf sim_time simpleLeftRightFlows AlltoAllFlows webSearchCDF num_nodes_per_pod f_dropAll_LS f_dropAll_HL f_dropAll_AC f_dropAll_TA f_dropAll_HT cwnd01_file cwnd02_file tr flow_trace flow_trace_out

    $ns flush-trace
    #close $f
    #close $nf
    close $tr

    if { $flow_trace == 1 } {
	close $flow_trace_out
    }
    
    if { $simpleLeftRightFlows == 1 } {
	set trafficType "L2R"
    } elseif { $AlltoAllFlows == 1 } {			
	set trafficType "A2A"
    } else {
	set trafficType "NA"
    }	
    
    if { $webSearchCDF == 1 } {
	set t_cdf "WEB"
    } else {
	set t_cdf "DAT"
    }	

    if {$getQmonLinkLevelStats == 1} {
	if {$TOPOLOGY=="LEAFSPINE"} {	
	    ## for { set i 0 } { $i < $numToRs } { incr i } 
	    ## for now just settling for left-most leaf (ToR) and all its linked spines
	    close $f_utilAll
	    close $f_utilAllHostLeaf

	    close $f_dropAll_LS
	    close $f_dropAll_HL

	    if { $AlltoAllFlows==1 } {
		set range $numToRs
	    } else { 
		set range [expr $numToRs/2]; ## default case and $simpleLeftRightFlows
	    }

	    for { set i 0 } { $i < $range } { incr i } {			
		for { set j 0 } { $j < $numAggs } { incr j } {
		    close $fq_mon($i$j) 
		    close $f_util($i$j) 
		    close $f_loss($i$j) 
		    close $f_queue($i$j)
		}
	    }
	} elseif {$TOPOLOGY=="FATTREE"} {

	    close $f_utilAllHostToR
	    close $f_utilAllToRAgg
	    close $f_utilAll

	    close $f_dropAll_AC
	    close $f_dropAll_TA
	    close $f_dropAll_HT

	    if { $AlltoAllFlows==1 } {
		set range $K
	    } else {	 
		set range [expr $K/2]; ## default case and $simpleLeftRightFlows
	    }

	    for {set i 0} {$i < $range} {incr i} { #pod i
		for {set j 0} {$j < $num_nodes_per_pod} {incr j} { #aggr switch j in pod i
		    set aIndex [expr ($i*$num_nodes_per_pod)+$j]
		    for {set k 0} {$k < [expr $K/2]} {incr k} { #
			set cIndex [expr $j*($K/2)+$k]

			close $fq_mon($aIndex$cIndex)
			close $f_util($aIndex$cIndex)
			close $f_loss($aIndex$cIndex)
			close $f_queue($aIndex$cIndex)
		    }
		}
	    }					
	}
    }	


    if {$LB_SCHEME == "CONGA"} { 
	###
    } else {
	###
    }

    ## File for tracing congestion window size
    close $cwnd01_file
    close $cwnd02_file

    ## AFCT calculations
    close $Out

    set awk_response {
	BEGIN{
	    av=0;
	    av_medium=0;
	    av_long=0;
	    afct_mice_sprayed=0;
	    afct_mice_hashed=0;
	    av_thruput_long=0;
	    av_goodput_long=0; ## 24-feb-16
	    av_goodput_fromFailedLeaf=0; ## 24-feb-16
	    av_goodput_ToAndFromGoodLeaf=0; ## 24-feb-16
	    avg_thru_fromFailedLeaf=0;
	    avg_thru_ToAndFromGoodLeaf=0;
	    avg_thru_flowsFromFailedLeafSprayed=0;
	    avg_thru_flowsOnPoorPath=0;
	    avg_thru_long_onPoorPath=0;
	    avg_thru_mice_onPoorPath=0;
	    medium_count=0;
	    long_count=0;
	    sum_medium=0;
	    sum_long=0;
	    sum_gput_long=0; ## 24-feb-16
	    sum_gput_fromFailedLeaf=0; ## 24-feb-16
	    sum_gput_ToAndFromGoodLeaf=0; ## 24-feb-16

	    ## 4-June 16
	    countNorthSouth = 0;
	    sumThruNorthSouth = 0;
	    avgThruNorthSouth = 0;
	    ## 11 June 16
	    afct_NorthSouth = 0;
	    sum_fct_NorthSouth = 0;

	    ## 2 June 2016
	    afct_mice = 0;
	    sum_fct_mice = 0;
	    count_mice = 0;

	    ## 16-Mar-16 MICE
	    afct_medium_endBeforeFailure=0; ## endBeforeFailure
	    afct_medium_endAfterFailureOccurred=0;
	    afct_medium_endAfterFailureDetected=0;
	    afct_medium_endAfterFailureFixed=0;
	    afct_medium_endAfterRestorationDetected=0;

	    afct_mice_endBeforeFailure=0; ## endBeforeFailure
	    afct_mice_endAfterFailureOccurred=0;
	    afct_mice_endAfterFailureDetected=0;
	    afct_mice_endAfterFailureFixed=0;
	    afct_mice_endAfterRestorationDetected=0;

	    sum_fct_mice_endBeforeFailure=0; 
	    sum_fct_mice_endAfterFailureOccurred=0;
	    sum_fct_mice_endAfterFailureDetected=0;
	    sum_fct_mice_endAfterFailureFixed=0;
	    sum_fct_mice_endAfterRestorationDetected=0;

	    sum_fct_medium_endBeforeFailure=0; 
	    sum_fct_medium_endAfterFailureOccurred=0;
	    sum_fct_medium_endAfterFailureDetected=0;
	    sum_fct_medium_endAfterFailureFixed=0;
	    sum_fct_medium_endAfterRestorationDetected=0;

	    count_mice_endBeforeFailure=0; 
	    count_mice_endAfterFailureOccurred=0;
	    count_mice_endAfterFailureDetected=0;
	    count_mice_endAfterFailureFixed=0;
	    count_mice_endAfterRestorationDetected=0; ## endAfterRestorationDetected

	    count_medium_endBeforeFailure=0; 
	    count_medium_endAfterFailureOccurred=0;
	    count_medium_endAfterFailureDetected=0;
	    count_medium_endAfterFailureFixed=0;
	    count_medium_endAfterRestorationDetected=0; ## endAfterRestorationDetected

	    ## AFCT Long Flows -- 28-Mar-2016
	    afct_long_endBeforeFailure=0; ## endBeforeFailure
	    afct_long_endAfterFailureOccurred=0;
	    afct_long_endAfterFailureDetected=0;
	    afct_long_endAfterFailureFixed=0;
	    afct_long_endAfterRestorationDetected=0;

	    sum_fct_long_endBeforeFailure=0; 
	    sum_fct_long_endAfterFailureOccurred=0;
	    sum_fct_long_endAfterFailureDetected=0;
	    sum_fct_long_endAfterFailureFixed=0;
	    sum_fct_long_endAfterRestorationDetected=0;

	    ## 16-Mar-16 LONGs
	    avg_thru_long_endBeforeFailure=0; ## endBeforeFailure
	    avg_thru_long_endAfterFailureOccurred=0;
	    avg_thru_long_endAfterFailureDetected=0;
	    avg_thru_long_endAfterFailureFixed=0;
	    avg_thru_long_endAfterRestorationDetected=0;

	    sum_thru_long_endBeforeFailure=0; 
	    sum_thru_long_endAfterFailureOccurred=0;
	    sum_thru_long_endAfterFailureDetected=0;
	    sum_thru_long_endAfterFailureFixed=0;
	    sum_thru_long_endAfterRestorationDetected=0;

	    count_long_endBeforeFailure=0; 
	    count_long_endAfterFailureOccurred=0;
	    count_long_endAfterFailureDetected=0;
	    count_long_endAfterFailureFixed=0;
	    count_long_endAfterRestorationDetected=0; ## endAfterRestorationDetected

	    ## 11-May-2016
	    avg_thru_toAndFromFailedLeaf = 0;
	    avg_thru_toFailedLeaf = 0;
	    sum_thru_toFailedLeaf = 0;
	    flowsToFailedLeaf = 0;


	    ## 12-May-2016
	    drops_toAndFromGoodLeaf = 0;
	    drops_toFailedLeaf = 0;
	    drops_fromFailedLeaf = 0;
	    rexmit_pk_toAndFromGoodLeaf = 0;
	    rexmit_pk_toFailedLeaf = 0;
	    rexmit_pk_fromFailedLeaf = 0;
	    
	    ## Talal 14-July-2016 
	    rexmit_pk_GL2GL_hash_end = 0; 
	    rexmit_pk_GL2GL_remapped = 0;
	    rexmit_pk_GL2GL_sprayed = 0;

	    ## 27 May 2016
	    sum_fct_toAndFromGoodLeaf=0;
	    sum_fct_mice_toAndFromGoodLeaf=0;
	    afct_toAndFromGoodLeaf=0;
	    afct_mice_toAndFromGoodLeaf=0;
	    miceFlowsToAndFromGoodLeaf=0;

	    ## Old is Gold :)
	    sum_fct_mice_sprayed=0;
	    sum_fct_mice_hashed=0;
	    sum_long_onPoorPath=0;
	    sum_mice_onPoorPath=0;
	    sum_thru_fromFailedLeaf=0;
	    sum_thru_ToAndFromGoodLeaf=0;
	    sum_thru_flowsOnPoorPath=0;
	    sum_thru_flowsFromFailedLeafSprayed=0;
	    sum_thru_long_onPoorPath=0;
	    sum_thru_mice_onPoorPath=0;
	    flowsFromFailedLeaf=0;
	    flowsToAndFromGoodLeaf=0;
	    flowsOnPoorPath=0;
	    flowsFromFailedLeafSprayed=0;
	    flowsMiceSprayed=0;
	    flowsMiceHashed=0;
	    sum_rexmit_pk=0;
	    sum_rexmit_pk_medium=0;
	    thruput_long=0;
	    #dupCount1=0;
	    #dupCount2=0;
	    sum_cwnd_cuts1=0;
	    sum_ecn_responses=0;
	    ##avg_thru_all=0;
	    sum_thru_all=0;
	    avg_thruput_all=0;

	    sumThruputAll=0; ## 25-july-16
	    avgThruputAll=0; ## 25-july-16

	    countIntraRackFlows=0; ## 22 Jul
	    sum_fct_IntraRackFlows=0;
	    countInterRackFlows=0;
	    sum_fct_InterRackFlows=0;
	    afct_IntraRackFlows=0;
	    afct_InterRackFlows=0;
	}
	{
	    if ($23==0) {
		drops += $11;
		sum += $5;
		sum_thru_all += $9;
		nl += 1;
		sum_rexmit_pk += $13;
		#dupCount1 += $14;
		#dupCount2 += $15;
		sum_cwnd_cuts1 += $10;
		sum_ecn_responses += $17;

		sumThruputAll += ($26 / $5); ## 25-july-16 ## bytes per second

		## 22 July
		if($25==0) {
		    sum_fct_InterRackFlows += $5;
		    countInterRackFlows += 1;
		} else {
		    sum_fct_IntraRackFlows += $5;
		    countIntraRackFlows += 1;
		}

		if ($12==0) {
		    ## Not a long flow for sure
		    if ($21==1) {
			## Is a mice flow (formerly a.k.a. tiny mice)

			if ($20==0) {
			    sum_fct_mice_endBeforeFailure += $5; 
			    count_mice_endBeforeFailure += 1; 
			} else if ($20==1) {
			    sum_fct_mice_endAfterFailureOccurred += $5;
			    count_mice_endAfterFailureOccurred += 1;
			} else if ($20==2) {
			    sum_fct_mice_endAfterFailureDetected += $5;
			    count_mice_endAfterFailureDetected += 1;
			} else if ($20==3) {
			    sum_fct_mice_endAfterFailureFixed += $5;
			    count_mice_endAfterFailureFixed += 1;
			} else if ($20==4) {
			    sum_fct_mice_endAfterRestorationDetected += $5;
			    count_mice_endAfterRestorationDetected += 1; ## endAfterRestorationDetected
			}
		    } else {
			## Medium Size flow (formerly a.k.a. mice flow)
			sum_medium += $5;
			medium_count += 1;
			sum_rexmit_pk_medium += $13;

			if ($20==0) {
			    sum_fct_medium_endBeforeFailure += $5; 
			    count_medium_endBeforeFailure += 1; 
			} else if ($20==1) {
			    sum_fct_medium_endAfterFailureOccurred += $5;
			    count_medium_endAfterFailureOccurred += 1;
			} else if ($20==2) {
			    sum_fct_medium_endAfterFailureDetected += $5;
			    count_medium_endAfterFailureDetected += 1;
			} else if ($20==3) {
			    sum_fct_medium_endAfterFailureFixed += $5;
			    count_medium_endAfterFailureFixed += 1;
			} else if ($20==4) {
			    sum_fct_medium_endAfterRestorationDetected += $5;
			    count_medium_endAfterRestorationDetected += 1; ## endAfterRestorationDetected
			}
		    }
		}
		else
		{
		    sum_long += $5;
		    thruput_long += $9;
		    sum_gput_long += (($7-$8)/$5); ## 24-feb-16
		    long_count += 1;

		    if ($20==0) {
			sum_fct_long_endBeforeFailure += $5; 
			sum_thru_long_endBeforeFailure += $9; 
			count_long_endBeforeFailure += 1;  
		    } else if ($20==1) {
			sum_fct_long_endAfterFailureOccurred += $5;
			sum_thru_long_endAfterFailureOccurred += $9;
			count_long_endAfterFailureOccurred += 1;
		    } else if ($20==2) {
			sum_fct_long_endAfterFailureDetected += $5;
			sum_thru_long_endAfterFailureDetected += $9;
			count_long_endAfterFailureDetected += 1;
		    } else if ($20==3) {
			sum_fct_long_endAfterFailureFixed += $5;
			sum_thru_long_endAfterFailureFixed += $9;
			count_long_endAfterFailureFixed += 1;
		    } else if ($20==4) {
			sum_fct_long_endAfterRestorationDetected += $5;
			sum_thru_long_endAfterRestorationDetected += $9;
			count_long_endAfterRestorationDetected += 1; ## endAfterRestorationDetected
		    }
		}

		if ($16==0 && $22==0)
		{
		    flowsToAndFromGoodLeaf += 1;
		    sum_thru_ToAndFromGoodLeaf += $9;
		    sum_gput_ToAndFromGoodLeaf += (($7-$8)/$5); ## 24-feb-16
		    drops_toAndFromGoodLeaf += $11;
		    rexmit_pk_toAndFromGoodLeaf += $13;
		    
		    
		    #Talal
		    if ($24==0)	
			{
            			rexmit_pk_GL2GL_sprayed += $13; # Sprayed Flows 
			} else {
				if($24==1 && $19==1)
				{
				 rexmit_pk_GL2GL_hash_end += $13; #Hashed till end 
				}	
				if($24==1 && $19==0)
				{
		            	 rexmit_pk_GL2GL_remapped += $13;    # Hashed then remapped 
				}
			}

		    sum_fct_toAndFromGoodLeaf += $5;
		    if ($21==1) {
			sum_fct_mice_toAndFromGoodLeaf += $5;
			miceFlowsToAndFromGoodLeaf += 1;
		    }
		} else {
		    if ($22==1) {
			flowsToFailedLeaf += 1;
			sum_thru_toFailedLeaf += $9;
			drops_toFailedLeaf += $11;
			rexmit_pk_toFailedLeaf += $13;
		    }

		    if ($16==1) {
			flowsFromFailedLeaf += 1;
			sum_thru_fromFailedLeaf += $9;
			sum_gput_fromFailedLeaf += (($7-$8)/$5); ## 24-feb-16
			drops_fromFailedLeaf += $11;
			rexmit_pk_fromFailedLeaf += $13;

			if ($18 == 1) {
			    if ($19==0) {
				flowsFromFailedLeafSprayed += 1;
				sum_thru_flowsFromFailedLeafSprayed += $9;
				if ($21==1)
				{
				    flowsMiceSprayed += 1;
				    sum_fct_mice_sprayed += $5;
				}
			    } else {
				flowsOnPoorPath += 1;
				sum_thru_flowsOnPoorPath += $9;

				if ($21==1)
				{
				    sum_mice_onPoorPath += 1;
				    sum_thru_mice_onPoorPath += $9;
				    flowsMiceHashed += 1;
				    sum_fct_mice_hashed += $5;
				} else {
				    if ($12==1) {
					sum_long_onPoorPath += 1;
					sum_thru_long_onPoorPath += $9;
				    }
				}
			    }
			}
		    }
		}
	    } else {
		## This is the case when the flow is North-South type
		sum_fct_NorthSouth += $5;
		countNorthSouth += 1;
		sumThruNorthSouth += $9;
	    }
	}
	END{
	    count_mice = count_mice_endBeforeFailure+count_mice_endAfterFailureOccurred+count_mice_endAfterFailureDetected+count_mice_endAfterFailureFixed + count_mice_endAfterRestorationDetected;
	    sum_fct_mice = sum_fct_mice_endBeforeFailure + sum_fct_mice_endAfterFailureOccurred + sum_fct_mice_endAfterFailureDetected + sum_fct_mice_endAfterFailureFixed + sum_fct_mice_endAfterRestorationDetected;

	    printf "Number of lines = %f\n", nl;
	    printf "sum = %f\n", sum;
	    printf "Number of small flows = %f", count_mice;
	    printf "Number of medium flows = %f", medium_count;
	    printf "Number of large flows = %f", long_count;
	    incr nl;

	    if (nl==0) {
		av = 0;
	    } else {
		av = sum/nl;
		#avg_thru_all = ((sum_thru_ToAndFromGoodLeaf + sum_thru_toFailedLeaf + sum_thru_fromFailedLeaf) / (flowsToAndFromGoodLeaf + flowsToFailedLeaf + flowsFromFailedLeaf)) * 8 / 1000000;
		avg_thruput_all = ( sum_thru_all / nl ) * 8 / 1000000;
		avgThruputAll = ( sumThruputAll / nl ) * 8 / 1000000;
	    }

	    if (countNorthSouth > 0) {
		avgThruNorthSouth = ( sumThruNorthSouth / countNorthSouth) * 8 / 1000000;
		afct_NorthSouth = sum_fct_NorthSouth / countNorthSouth;
	    }

	    if (count_mice > 0) {
		afct_mice = sum_fct_mice / count_mice;

		if (count_mice_endBeforeFailure > 0) {
		    afct_mice_endBeforeFailure = sum_fct_mice_endBeforeFailure / count_mice_endBeforeFailure; 
		}
		if (count_mice_endAfterFailureOccurred > 0) {
		    afct_mice_endAfterFailureOccurred = sum_fct_mice_endAfterFailureOccurred / count_mice_endAfterFailureOccurred;
		}
		if (count_mice_endAfterFailureDetected > 0) {
		    afct_mice_endAfterFailureDetected = sum_fct_mice_endAfterFailureDetected / count_mice_endAfterFailureDetected;
		}
		if (count_mice_endAfterFailureFixed > 0) {
		    afct_mice_endAfterFailureFixed = sum_fct_mice_endAfterFailureFixed / count_mice_endAfterFailureFixed;
		}
		if (count_mice_endAfterRestorationDetected > 0) {
		    afct_mice_endAfterRestorationDetected = sum_fct_mice_endAfterRestorationDetected / count_mice_endAfterRestorationDetected;
		}
	    }

	    if (medium_count > 0) {
		av_medium = sum_medium/medium_count;

		if (count_medium_endBeforeFailure > 0) {
		    afct_medium_endBeforeFailure = sum_fct_medium_endBeforeFailure / count_medium_endBeforeFailure; 
		}
		if (count_medium_endAfterFailureOccurred > 0) {
		    afct_medium_endAfterFailureOccurred = sum_fct_medium_endAfterFailureOccurred / count_medium_endAfterFailureOccurred;
		}
		if (count_medium_endAfterFailureDetected > 0) {
		    afct_medium_endAfterFailureDetected = sum_fct_medium_endAfterFailureDetected / count_medium_endAfterFailureDetected;
		}
		if (count_medium_endAfterFailureFixed > 0) {
		    afct_medium_endAfterFailureFixed = sum_fct_medium_endAfterFailureFixed / count_medium_endAfterFailureFixed;
		}
		if (count_medium_endAfterRestorationDetected > 0) {
		    afct_medium_endAfterRestorationDetected = sum_fct_medium_endAfterRestorationDetected / count_medium_endAfterRestorationDetected;
		}
	    }

	    if (long_count > 0) {
		av_long = sum_long/long_count;
		av_thruput_long = (thruput_long/long_count) * 8 / 1000000;
		av_goodput_long = (sum_gput_long/long_count)*8/1000000; ## 24-feb-16

		if (count_long_endBeforeFailure > 0) {
		    avg_thru_long_endBeforeFailure = (sum_thru_long_endBeforeFailure / count_long_endBeforeFailure) * 8 / 1000000; 
		    afct_long_endBeforeFailure = sum_fct_long_endBeforeFailure / count_long_endBeforeFailure;
		}

		if (count_long_endAfterFailureOccurred > 0) {
		    avg_thru_long_endAfterFailureOccurred = (sum_thru_long_endAfterFailureOccurred / count_long_endAfterFailureOccurred) * 8 / 1000000;
		    afct_long_endAfterFailureOccurred = sum_fct_long_endAfterFailureOccurred / count_long_endAfterFailureOccurred;
		}

		if (count_long_endAfterFailureDetected > 0) {
		    avg_thru_long_endAfterFailureDetected = (sum_thru_long_endAfterFailureDetected / count_long_endAfterFailureDetected) * 8 / 1000000;
		    afct_long_endAfterFailureDetected = sum_fct_long_endAfterFailureDetected / count_long_endAfterFailureDetected;
		}

		if (count_long_endAfterFailureFixed > 0) {
		    avg_thru_long_endAfterFailureFixed = (sum_thru_long_endAfterFailureFixed / count_long_endAfterFailureFixed) * 8 / 1000000;
		    afct_long_endAfterFailureFixed = sum_fct_long_endAfterFailureFixed / count_long_endAfterFailureFixed;
		}

		if (count_long_endAfterRestorationDetected > 0) {
		    avg_thru_long_endAfterRestorationDetected = (sum_thru_long_endAfterRestorationDetected / count_long_endAfterRestorationDetected) * 8 / 1000000;
		    afct_long_endAfterRestorationDetected = sum_fct_long_endAfterRestorationDetected / count_long_endAfterRestorationDetected;
		}
	    }

	    if (flowsToAndFromGoodLeaf > 0) {
		avg_thru_ToAndFromGoodLeaf = (sum_thru_ToAndFromGoodLeaf/flowsToAndFromGoodLeaf) * 8 / 1000000;
		av_goodput_ToAndFromGoodLeaf = (sum_gput_ToAndFromGoodLeaf/flowsToAndFromGoodLeaf)*8/1000000; ## 24-feb-16
		afct_toAndFromGoodLeaf = sum_fct_toAndFromGoodLeaf / flowsToAndFromGoodLeaf;

		if (miceFlowsToAndFromGoodLeaf > 0) {
		    afct_mice_toAndFromGoodLeaf = sum_fct_mice_toAndFromGoodLeaf / miceFlowsToAndFromGoodLeaf;
		}
	    } 

	    if (flowsToFailedLeaf > 0) {
		avg_thru_toFailedLeaf = (sum_thru_toFailedLeaf / flowsToFailedLeaf) * 8 / 1000000 ;
	    }

	    if (flowsFromFailedLeaf > 0) {
		avg_thru_fromFailedLeaf = (sum_thru_fromFailedLeaf/flowsFromFailedLeaf) * 8 / 1000000;
		av_goodput_fromFailedLeaf = (sum_gput_fromFailedLeaf/flowsFromFailedLeaf)*8/1000000; ## 24-feb-16
	    }

	    if (flowsFromFailedLeafSprayed > 0) {
		avg_thru_flowsFromFailedLeafSprayed = (sum_thru_flowsFromFailedLeafSprayed/flowsFromFailedLeafSprayed) * 8 / 1000000;
	    }

	    if (flowsOnPoorPath > 0) {
		avg_thru_flowsOnPoorPath = (sum_thru_flowsOnPoorPath/flowsOnPoorPath) * 8 / 1000000;
		avg_thru_long_onPoorPath = (sum_thru_long_onPoorPath/sum_long_onPoorPath) * 8 / 1000000;
		avg_thru_mice_onPoorPath = (sum_thru_mice_onPoorPath/sum_mice_onPoorPath) * 8 / 1000000;
		afct_mice_sprayed = sum_fct_mice_sprayed/flowsMiceSprayed; 
		afct_mice_hashed = sum_fct_mice_hashed/flowsMiceHashed;
	    } 

	    if(countIntraRackFlows > 0) {
		afct_IntraRackFlows = sum_fct_IntraRackFlows / countIntraRackFlows;
	    }

	    if(countInterRackFlows > 0) {
		afct_InterRackFlows = sum_fct_InterRackFlows / countInterRackFlows;
	    }

	    ## added the five Specialized AFCT Mice after MiceHashed -- added 5 specialized throughput longs after Thru-on-Poor-Path

	    printf "%.2f %s %s %s %s %d %d %d AFCT:(All,Large,Medium,Mice,All-GL2GL,Mice-GL2GL,Mice-Sprayed,Mice-Hashed,NorthSouth) %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f Mice %.5f %.5f %.5f %.5f %.5f Medium %.5f %.5f %.5f %.5f %.5f Large %.5f %.5f %.5f %.5f %.5f Flow-Counts:(All,Large,Medium,Mice,ToAndFomGoodLeaf,fromFailedLeaf,toFailedLeaf,NorthSouth) %d %d %d %d %d %d %d %d Failed-Leaf-Sprayed %d Failed-Leaf-Hashed %d Mice-Sprayed %d Mice-Hashed %d rex-pkts(G2G_L,to_F_L,from_F_L,all): %d %d %d %d cwnd-cuts: %d ecn-responses: %d rex-medium-pkts: %d drops_rexmTO(G2G_L,to_F_L,from_F_L,all): %d %d %d %d Avg-Thruput:Large %.3f (To-And-From-Good-Leaf,From-Failed-Leaf,To-Failed-Leaf) %.3f %.3f %.3f Failed-Leaf-Sprayed %.3f Failed-Leaf-Hashed %.3f LargeFlowsHashed %.3f ShortFlowsHashed %.3f %.3f %.3f %.3f %.3f %.3f NorthSouthThru %.3f Avg-Goodput:(LargeFlows,To-And-From-Good-Leaf,From-Failed-Leaf) %.3f %.3f %.3f AvgThruput-All %.3f %.3f rex-pkts(HashedTillEnd,Remapped,Sprayed): %d %d %d AFCT(intra,inter) %.5f %.5f Counts(intra,inter) %d %d  \n", load_, type, cdf, lbs, topo, dctcpK, dupack, fst, av, av_long, av_medium, afct_mice, afct_toAndFromGoodLeaf, afct_mice_toAndFromGoodLeaf, afct_mice_sprayed, afct_mice_hashed, afct_NorthSouth, afct_mice_endBeforeFailure, afct_mice_endAfterFailureOccurred, afct_mice_endAfterFailureDetected, afct_mice_endAfterFailureFixed, afct_mice_endAfterRestorationDetected , afct_medium_endBeforeFailure, afct_medium_endAfterFailureOccurred, afct_medium_endAfterFailureDetected, afct_medium_endAfterFailureFixed, afct_medium_endAfterRestorationDetected,afct_long_endBeforeFailure, afct_long_endAfterFailureOccurred, afct_long_endAfterFailureDetected, afct_long_endAfterFailureFixed, afct_long_endAfterRestorationDetected, nl, long_count,medium_count, count_mice, flowsToAndFromGoodLeaf, flowsFromFailedLeaf, flowsToFailedLeaf, countNorthSouth, flowsFromFailedLeafSprayed, flowsOnPoorPath, flowsMiceSprayed, flowsMiceHashed, rexmit_pk_toAndFromGoodLeaf, rexmit_pk_toFailedLeaf, rexmit_pk_fromFailedLeaf, sum_rexmit_pk, sum_cwnd_cuts1, sum_ecn_responses, sum_rexmit_pk_medium, drops_toAndFromGoodLeaf, drops_toFailedLeaf, drops_fromFailedLeaf, drops, av_thruput_long , avg_thru_ToAndFromGoodLeaf, avg_thru_fromFailedLeaf, avg_thru_toFailedLeaf, avg_thru_flowsFromFailedLeafSprayed, avg_thru_flowsOnPoorPath, avg_thru_long_onPoorPath, avg_thru_mice_onPoorPath, avg_thru_long_endBeforeFailure, avg_thru_long_endAfterFailureOccurred, avg_thru_long_endAfterFailureDetected, avg_thru_long_endAfterFailureFixed, avg_thru_long_endAfterRestorationDetected, avgThruNorthSouth, av_goodput_long, av_goodput_ToAndFromGoodLeaf, av_goodput_fromFailedLeaf, avg_thruput_all, avgThruputAll, rexmit_pk_GL2GL_hash_end, rexmit_pk_GL2GL_remapped, rexmit_pk_GL2GL_sprayed, afct_IntraRackFlows, afct_InterRackFlows, countIntraRackFlows, countInterRackFlows  >> fil;


	}
    }

    set to_read "Out.ns"
    set res "response_times.log"

    exec awk -v type=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME1 -v topo=$TOPOLOGY -v fst=$fs_threshold -v dctcpK=$dctcp_K -v dupack=$DupAckThresh -v K=$K $awk_response $to_read 

  set awk_util {
      BEGIN{
	  av=0;

      }
      {
	  sum += $2;
	  nl += 1;
      }
      END{
	  printf "Number of lines = %f\n", nl;
	  printf "sum = %f\n", sum;
	  if (nl==0) {
	      av = 0;
	  } else {
	      av = sum/nl;
	  }
	  printf "%f %f\n", bw, av >> file;
      }
  }

	set awk_updates {
	    BEGIN{
		av=0;
	    }
	    {
		sum += $1;
		nl += 1;
	    }
	    END{
		printf "sum = %f\n", sum;
		printf "%.2f %i %i updates\n", load_, enabled, sum >> fil;
		#printf "%.2f %s %.2fms %d flows\n", load_, src, av, nl >> "log_response_times";
	    }
	}

	if {$getQmonLinkLevelStats == 1} {
		set awk_qmon_link_utilz {
			BEGIN{
				util_sum=0;
				util_avg=0;
			}
			{
				total=0;
				##maxticks = (simtime/2) * 1000;
				maxticks = simtime * 1000;
				if (nl < maxticks) {
					for (i = 2; i <= NF; i++) total = total+$i
					avg = total/(NF-1);
					###if (topo=="FATTREE") { } else { avg = ($2+$3+$4+$5)/4; }
					util_sum = util_sum+avg;
					nl += 1;
				}
			}
			END{
			    if (nl==0) {
				util_avg = 0;
			    } else {
				util_avg = util_sum/nl;
			    }
    		#printf "%.2f %s %s %s %d %d msecs type %s Avg-Util %.5f\n", load_, src, lbs, topo, fst, nl, type, util_avg >> fil;
				if (type=="A-C" || type=="L-S") {
    			printf "%.2f %s %s %s %s %d %d msecs Avg_Util %s %.3f", load_, trafficType, cdf, lbs, topo, fst, nl, type, util_avg >> fil;
    		} else if (type=="T-A") {
    			printf " %s %.5f", type, util_avg >> fil;
    		} else {
    			printf " %s %.5f\n", type, util_avg >> fil;
    		}
			}
		}
		set res "avg_link_utilization.log"

		#set foldername "stats-load-$mice_load-avfs-$av_fsize-thresh-$fs_threshold-lb-$LB_SCHEME-top-$TOPOLOGY-run-$run_i/"

		if {$TOPOLOGY=="LEAFSPINE"} {	## will have four cols of interest in normal situation
			##set to_read_from "qmon.utilAllLeaf0"

			exec awk -v type=L-S -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_link_utilz "qmon.utilAllLeafSpine"

			exec awk -v type=H-L -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_link_utilz "qmon.utilAllHostLeaf"


		} else {						## will have two cols of interest in normal 4-ary topology
			##set to_read_from "qmon.utilAllAgg"

			exec awk -v type=A-C -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_link_utilz "qmon.utilAllAgg"

			exec awk -v type=T-A -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_link_utilz "qmon.utilAllToR"

			exec awk -v type=H-T -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_link_utilz "qmon.utilAllHostToR"
		}		
		##exec awk -v type="all" -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_updates $to_read_from		

		set awk_qmon_maxmin_diff {
			BEGIN{
				diff_sum=0;
			}
			{
				##maxticks = (simtime/2) * 1000;
				maxticks = simtime * 1000;
				if (nl < maxticks) {
					max_l = $2;
					min_l = $2;
					for (i = 3; i <= NF; i++) {
						if ($i > max_l) max_l = $i;
						if ($i < min_l) min_l = $i;
					}
					diff_sum = diff_sum + (max_l - min_l);
					nl += 1;
				}
			}
			END{
				diff_avg = diff_sum/nl;
				
				if (type=="A-C" || type=="L-S") {
    			printf "%.2f %s %s %s %s %d %d msecs Avg_MaxLink-MinLink %s %.3f", load_, trafficType, cdf, lbs, topo, fst, nl, type, diff_avg >> fil;
    		} else if (type=="T-A") {
    			printf " %s %.5f", type, diff_avg >> fil;
    		} else {
    			printf " %s %.5f\n", type, diff_avg >> fil;
    		}
				
				#printf "%.2f %s %.2fms %d flows\n", load_, src, av, nl >> "log_response_times";
			}
		}
		set res "maxmin_link_util_diff.log"

		##set foldername "stats-load-$mice_load-avfs-$av_fsize-thresh-$fs_threshold-lb-$LB_SCHEME-top-$TOPOLOGY-run-$run_i/"

		if {$TOPOLOGY=="LEAFSPINE"} {	## will have four cols of interest in normal situation
			##set to_read_from $foldername"qmon.utilAllLeaf0"
			
			exec awk -v type=L-S -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_maxmin_diff "qmon.utilAllLeafSpine"

			exec awk -v type=H-L -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_maxmin_diff "qmon.utilAllHostLeaf"

		} else {						## will have two cols of interest in normal 4-ary topology
			##set to_read_from $foldername"qmon.utilAllAgg0"

			exec awk -v type=A-C -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_maxmin_diff "qmon.utilAllAgg"

			exec awk -v type=T-A -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_maxmin_diff "qmon.utilAllToR"

			exec awk -v type=H-T -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_maxmin_diff "qmon.utilAllHostToR"

		}		
		
		set awk_qmon_drops {
			BEGIN{
				sum_drops=0;
			}
			{
				total=0;
				##maxticks = (simtime/2) * 1000;
				maxticks = simtime * 1000;
				if (nl < maxticks) {
					for (i = 2; i <= NF; i++) total = total+$i
					sum_drops = sum_drops+total;
					nl += 1;
				}
			}
			END{
				if (type=="A-C" || type=="L-S") {
    			printf "%.2f %s %s %s %s %d %d msecs Drops %s %d", load_, trafficType, cdf, lbs, topo, fst, nl, type, sum_drops >> fil;
    		} else if (type=="T-A") {
    			printf " %s %d", type, sum_drops >> fil;
    		} else {
    			printf " %s %d\n", type, sum_drops >> fil;
    		}
			}
		}
		set res "drops_per_level.log"
		
		if {$TOPOLOGY=="LEAFSPINE"} {	## will have four cols of interest in normal situation
			##set to_read_from "qmon.utilAllLeaf0"

			exec awk -v type=L-S -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME1 -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_drops "qmon.dropAll_LS"

			exec awk -v type=H-L -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME1 -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_drops "qmon.dropAll_HL"


		} else {						## will have two cols of interest in normal 4-ary topology
			##set to_read_from "qmon.utilAllAgg"

			exec awk -v type=A-C -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME1 -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_drops "qmon.dropAll_AC"

			exec awk -v type=T-A -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME1 -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_drops "qmon.dropAll_TA"

			exec awk -v type=H-T -v trafficType=$trafficType -v cdf=$t_cdf -v load_=$mice_load -v src=$SRC -v fil=$res -v lbs=$LB_SCHEME1 -v topo=$TOPOLOGY -v fst=$fs_threshold -v K=$K -v simtime=$sim_time $awk_qmon_drops "qmon.dropAll_HT"
		}				
		
		
	}

#    if { $SRC == "TCP/CTP" } {
#        #puts $f_u_t "PRUNING [$paseArbInvokerTor(0) set pruning] Load $mice_load"
#        for { set k 0 } { $k < $num_tors*$num_aggs } { incr k } {
#		    puts $f_u_t "[$paseArbInvokerTor($k) set updt_req_sent]\t [$paseArbInvokerTor($k) set nrexmit_]"
#        }
        #puts $f_u_e "PRUNING [$paseArbInvokerHost(0) set pruning] Load $mice_load"
#        for { set i 0 } { $i < $num_tors*$num_aggs*$num_hosts } { incr i } {
#		    puts $f_u_e "[$paseArbInvokerHost($i) set updt_req_sent]\t [$paseArbInvokerHost($i) set nrexmit_]"
#        }

#		close $f_u_t
#		close $f_u_e
#		set to_read "tor-agg_updates.txt"
#		set res "tor-aggr_updates.log"
#		exec awk -v load_=$mice_load -v enabled=[$host(0) set pruning] -v fil=$res $awk_updates $to_read

#		set to_read "end-tor_updates.txt"
#		set res "end-tor_updates.log"
#		exec awk -v load_=$mice_load -v anabled=0 -v fil=$res $awk_updates $to_read
#    }

	#exec nam out.nam &

    exit 0

}

####	FUNCTIONS BODIES ENDED

#Agent/TCP set window_ 80

##### SMI: June 10, 2015 - Trying to enable the correction for spurious retransmissions
##Agent/TCP set frto_enabled_ true;
##Agent/TCP set sfrto_enabled_ true;
##Agent/TCPSink set generateDSacks_ true;
##Agent/TCP set spurious_response_ 1;
##### SMI: June 10, 2015 - Trying to enable the correction for spurious retransmissions

#set btnk_bw [expr $bw_aggCore]

set ns [new Simulator]

#set nf [open out.nam w]
#$ns namtrace-all $nf

set tr [open trac.tr w]
##$ns trace-all $tracer

if {$traceAll==1} {
	$ns trace-all $tr 
}

Node set multiPath_ 1

$ns color 0 Red
$ns color 1 Orange
$ns color 2 Yellow
$ns color 3 Green
$ns color 4 Blue
$ns color 5 Violet
$ns color 6 Brown
$ns color 7 Black

#			 _   _	  _   _
#		    |_| |_|  |_| |_| Spine/Agg
#		    | \/  |	 | \/  |
#		    |_/\ _|  |_/\ _|
#		    |_|	|_|  |_| |_| Leaf/TOR
#		    / \ / \  / \  / \
#    	    ....  ...  ...  .... Hosts
#
#	Core -> Agg [2 Aggs] (10G)
#	Agg -> TOR [2 TORs per Agg] (10G)
#	TOR -> Endhosts [20 hosts per rack] (1G)

#end-host: local arb-agent at port 20 (this port number can also be made a var)
#end-host: arb-agent at port $arb_port (currently I have $arb_port=30)
#TOR-switch: arb-agent-sink at any port (other than $arb_port)

#TOR-switch: arb-agent at port $arb_port (currently $arb_port=30)
#Agg-switch: arb-agent-sink at any port 

#set f_u_t [open tor-agg_updates.txt w]
#set f_u_e [open end-tor_updates.txt w]

# Begin: setup topology ----------------------------------------

# this counter will count the number of times the stats for CBQ have been generated
set timeIntsElapsed 0

#puts "I am alive and kicking!!!! 4"

if {$TOPOLOGY=="LEAFSPINE"} {
    if { $debugMsgs==1 } {
	puts "Leaf-Spine Topology"
    }
    ###set totalHosts [expr $num_hosts_per_leaf * $k] ; # 
    set totalHosts [expr $num_hosts_per_leaf * $numToRs] ; # $k

    ##TODO Is this correct? 4-dec-2014 (SMI).... Aqib+Ghous+Adil ----> maybe we can ask Sir about this
    set leafspine_bw [expr $bw_torAgg*$numAggs]
    set endleaf_bw [expr $bw_endhostTor*$num_hosts_per_leaf]; # not sure if this is correct, maybe just $bw_endhostTor ??? 

    # TODO : Need to uncomment this, and then modify the traffic generation line so that it accommodates the fact that
    # btnk_bw is now representing the cross-section BW as opposed to a single link's BW
    # btnk_bw_link represents BW of a single link
    if {$leafspine_bw <= $endleaf_bw} {
	set btnk_bw $leafspine_bw 
	set btnk_bw_link $bw_torAgg
    } else {
	set btnk_bw $endleaf_bw
	set btnk_bw_link $bw_endhostTor
    }

    #if {$bw_torAgg < $bw_endhostTor} {
    #	set btnk_bw $bw_torAgg
    #} else {
    #	set btnk_bw $bw_endhostTor
    #}

    # create the switches....
    for { set i 0 } { $i < $numToRs } { incr i } {
	set upna_ID_ [expr (1*$nodeIdOffset)+$i]
	if {$LB_SCHEME == "CONGA"} { 
	    set leaf($i) [$ns node $upna_ID_]; # TOR LEVEL
	    $leaf($i) set level 1
	    puts "Leaf Name = [$leaf($i) set upnaID]"
	    $leaf($i) set link_bw [expr ($bw_torAgg/1000.0)]
	    $leaf($i) configureCongaTables ; # instantiate the CongaTables through a C++ function			
	} else {
	    set leaf($i) [$ns node]; # TOR LEVEL
	    $leaf($i) shape box
	    $leaf($i) color green
	}
    }

    for { set i 0 } { $i < $numAggs } { incr i } {
	set upna_ID_ [expr (2*$nodeIdOffset)+$i]
	if {$LB_SCHEME == "CONGA"} { 
	    set spine($i) [$ns node $upna_ID_]; #	AGG LEVEL
	    $spine($i) set level 2 
	    puts "Spine Name = [$spine($i) set upnaID]"
	    $spine($i) configureCongaTables ; # instantiate the CongaTables through a C++ function
	} else {
	    set spine($i) [$ns node]; #	AGG LEVEL
	    $spine($i) color blue; #Agg
	    $spine($i) shape box; #Agg
	}
    }

    puts "FailedLinkIndex = $FailedLinkIndex; FailureRatio = $FailureRatio; FailureCase=$FailureCase; "
    ## now setup links here between leaf and spine.....
    for { set i 0 } { $i < $numToRs } { incr i } {		
	for { set j 0 } { $j < $numAggs } { incr j } {
	    set ls_LinkBW $bw_torAgg; ## Default Case

	    if { $i == $FailedLeaf && $j == $FailedLinkIndex } {
		if { $RealisticFailure==0 && $FailureCase==1 } {
		    set ls_LinkBW [expr ($bw_torAgg/$FailureRatio)]
		}
	    }
	    $ns makeLink $leaf($i) $spine($j) $ls_LinkBW $switch_Algo $link_delay $pkSize $qLimit -1
	    if { $debugMsgs==1 } {
		puts "Leaf = $i, Spine = $j, BW = $ls_LinkBW";
	    }
	}
    }

    # create the endhosts....
    for { set i 0 } { $i < $numToRs } { incr i } {
	for { set j 0 } { $j < $num_hosts_per_leaf } { incr j } {
	    set hostIndex [expr ($i*$num_hosts_per_leaf)+$j]

	    if {$LB_SCHEME == "CONGA"} { 
		set host($hostIndex) [$ns node $hostIndex]
		$host($hostIndex) set level 0
		$host($hostIndex) set link_bw [expr ($bw_endhostTor/1000.0)]
		if { $debugMsgs==1 } {
		    puts "Host Name = [$host($hostIndex) set upnaID]" 
		}
		$host($hostIndex) configureCongaTables ; # instantiate the CongaTables through a C++ function
	    } else {
		set host($hostIndex) [$ns node]
	    }
	    
	    ## now make links with the leaf as well.....
	    $ns makeLink $leaf($i) $host($hostIndex) $bw_endhostTor $switch_Algo $link_delay $pkSize $qLimit -1 
	    ##puts "makeLink between LEAF $i and HOST $hostIndex"
	}
    }
    if { $debugMsgs==1 } {
	puts "All endhosts created!"
    }
    if { $NorthSouthTraffic==1 } {
	## setup a remote node/user, connected to each spine
	for { set i 0 } { $i < $numAggs } { incr i } {
	    set remoteNode($i) [$ns node]; # REMOTE-USER LEVEL
	    $ns makeLink $spine($i) $remoteNode($i) $ls_LinkBW $switch_Algo $link_delay $pkSize $qLimit -1
	}
    }
	
} elseif {$TOPOLOGY=="FATTREE"} {
    ### case of LB_SCHEME="UPNA"
    if { $debugMsgs==1 } {
	puts "Fat-Tree Topology"
    }
    set num_tors_per_pod [expr $K/2]; # number of ToR switches per aggregator switch
    set num_nodes_per_pod [expr $K/2]; # number of aggregator switches per core switch
    set num_hosts_per_tor [expr $K/2]; # number of endhosts per rack --> later change it to 20
    set totalHosts [expr ($K*$K*$K)/4] ; # use this (div by 2) in place of hostsPerAgg

    # TODO : Need to review this.....
    ### Is this correct? 4-dec-2014 (SMI)
    set endtor_bw [expr $bw_endhostTor*$num_hosts_per_tor]; # not sure if this is correct
    set toragg_bw [expr $bw_torAgg*$K/2]; # not sure if this is correct
    set aggcore_bw [expr $bw_aggCore*$K/2]; # not sure if this is correct

    if {$endtor_bw < $toragg_bw} {
	if {$endtor_bw < $aggcore_bw} {
	    set btnk_bw $endtor_bw
	    set btnk_bw_link $bw_endhostTor
	} else {
	    set btnk_bw $aggcore_bw
	    set btnk_bw_link $bw_aggCore
	}
    } else {
	if {$toragg_bw < $aggcore_bw} {
	    set btnk_bw $toragg_bw
	    set btnk_bw_link $bw_torAgg
	} else {
	    set btnk_bw $aggcore_bw
	    set btnk_bw_link $bw_aggCore
	}
    }

    ## now setup topology here.....
    #create core switch
    set num_cores [expr ($K*$K)/4]; # total number of cores

    for {set i 0} {$i < $num_cores} {incr i} { 
	if {$LB_SCHEME == "CONGA"} {
	    set upna_ID_ [expr (3*$nodeIdOffset)+$i]
	    set core($i) [$ns node $upna_ID_]
	    $core($i) set level 3
	    $core($i) set link_bw [expr $bw_aggCore/1000.0)]
	$core($i) configureCongaTables ; # instantiate the CongaTables through a C++ function
    } else {
	set core($i) [$ns node]
	$core($i) color blue
    }
}
 
## TODO now setup AGGs, TORs, and endhosts here.....

#puts "I am alive and kicking!!!! 5"

#create aggregation and Edge switch

for {set i 0} {$i < $K} {incr i} { 
    for {set j 0} {$j < $num_nodes_per_pod} {incr j} {
	set index [expr ($i*$num_nodes_per_pod)+$j]
	if {$LB_SCHEME == "CONGA"} {
	    set upna_ID_a [expr (2*$nodeIdOffset)+$index]
	    set aggr($index) [$ns node $upna_ID_a]
	    $aggr($index) set level 2
	    $aggr($index) set link_bw [expr $bw_aggCore/1000.0)]
	$aggr($index) configureCongaTables ; # instantiate the CongaTables through a C++ function
	
	set upna_ID_t [expr (1*$nodeIdOffset)+$index]
	set tor($index) [$ns node $upna_ID_t]
	$tor($index) set level 1
	$tor($index) set link_bw [expr $bw_torAgg/1000.0)]
    $tor($index) configureCongaTables ; # instantiate the CongaTables through a C++ function
} else {
    set aggr($index) [$ns node]
    $aggr($index) color red
    set tor($index) [$ns node]
    $tor($index) color green
}     
} 
}

#create hosts and links between hosts and edge switch
for {set i 0} {$i < $K} {incr i} { #pod i
    for {set j 0} {$j < $num_nodes_per_pod} {incr j} { #aggr switch j in pod i
	set tIndex [expr ($i*$num_nodes_per_pod)+$j]
	for {set k 0} {$k < $num_hosts_per_tor} {incr k} { #connect host to switch j in pod i
	    set hIndex [expr ($tIndex*$num_hosts_per_tor)+$k]
	    ##puts "hIndex=$hIndex"
	    if {$LB_SCHEME=="CONGA"} {
		set host($hIndex) [$ns node $hIndex]
		$host($hIndex) set level 0
		$host($hIndex) set link_bw [expr ($bw_endhostTor/1000.0)]
		puts "Host Name = [$host($hIndex) set upnaID]" 
		$host($hIndex) configureCongaTables ; # instantiate the CongaTables through a C++ function
	    } else {
		set host($hIndex) [$ns node]
		$host($hIndex) shape box
	    }
	    $ns makeLink $tor($tIndex) $host($hIndex) $bw_endhostTor $switch_Algo $link_delay $pkSize $qLimit -1 
	}
    } 
}

#create links inside the pod
for {set i 0} {$i < $K} {incr i} { # pod i
    for {set j 0} {$j < $num_nodes_per_pod} {incr j} { # agg j of pod i
	set aIndex [expr ($i*$num_nodes_per_pod)+$j]
	for {set k 0} {$k < [expr $K/2]} {incr k} { # tor k of pod i
	    set tIndex [expr ($i*$num_nodes_per_pod)+$k] 
	    $ns makeLink $tor($tIndex) $aggr($aIndex) $bw_torAgg $switch_Algo $link_delay $pkSize $qLimit -1	
	    ##$ns duplex-link $aggr($i$j) $edge($i$k) $lineRate [expr $RTT/4] $switchAlg
	}
    }
}

#create links between the aggregation tier and the core tier
for {set i 0} {$i < $K} {incr i} { #pod i
    for {set j 0} {$j < $num_nodes_per_pod} {incr j} { #aggr switch j in pod i
	set aIndex [expr ($i*$num_nodes_per_pod)+$j]
	for {set k 0} {$k < [expr $K/2]} {incr k} { #
	    $ns makeLink $core([expr $j*($K/2)+$k]) $aggr($aIndex) $bw_aggCore $switch_Algo $link_delay $pkSize $qLimit -1
	    ##$ns duplex-link $core([expr $j*($K/2)+$k]) $aggr($i$j) $lineRate [expr $RTT/4] $switchAlg
	}
    }
}
## TODO Obviously will need to write code for creating lbControllers in similar fashion as addArbitrators

} else {
	# some other topology from leaf-spine and fat-tree
if { $debugMsgs==1 } {
    puts "some other topology from leaf-spine and fat-tree"
}
}

# End: topology developed ----------------------------------------


# Begin: agents and sources ------------------------------------

## SETUP for LONG FTP FLOWS...
set longFlowCount 0 ; # keep this counter

#### Irteza 13-Dec.... 2 Long flows
if { $simpleFewFTPflows == 1 } { 
    [build-tcp $host(0) $host([expr $totalHosts-2]) 0.01] set class_ 0
    [build-tcp $host(1) $host([expr $totalHosts-1]) 0.01] set class_ 1
    ##[build-tcp $host(0) $host(2) 0.26] set class_ 2
    if { $debugMsgs==1 } {
	puts "made the FTP flows!"
    }
}

#puts "I am alive and kicking!!!! 6"

set cwnd01_file [open cwnd01.tr w]
set cwnd02_file [open cwnd02.tr w]

proc plotWindow {tcpSource outfile} {
    global ns

    set now [$ns now]
    set cwnd [$tcpSource set cwnd_]
    ##    set flowsize [$tcpSource set flow_size]
    set fsize [$tcpSource set seqno_]
    set pkt_sent [$tcpSource set ndatapack_]
    set rex_pkts [$tcpSource set nrexmitpack_]

    ###Print TIME CWND   for  gnuplot to plot progressing on CWND
    puts $outfile "$now $cwnd $rex_pkts "

    if { [expr $pkt_sent-$rex_pkts] < $fsize } {
	$ns at [expr $now+0.0001] "plotWindow $tcpSource $outfile"
    }
}



if { $simpleOneLeftRightFlow == 1 } { 
    if { $debugMsgs==1 } {
	puts "building single TCP short flow!"
    }
    if { $singleShortFlowSize > $fs_threshold } { #  was previously : $fl_size > $fs_threshold
	set isElephant 1;
    } else {
	set isElephant 0;
    }

    set singleShortFlowSize [expr $singleShortFlowSize * $pkSize]
    ## flow-id 1 sink 0 global_time 0.01 isElephant 0

    set fromFailedLeaf 1; # indicates the flow is from a failed leaf
    set PoorFlow 0

    set stcp [build-short-lived $host(0) $host([expr $totalHosts/2]) $pkSize 1 0 0.01 $SRC $SINK $singleShortFlowSize $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0]

## 	if {$LB_SCHEME == "WFCS"} {
##		$stcp set flowcell_ 1;
##		$stcp set flowcellSizePkts_ $FlowcellSize; # What is the size of the flowcell in packets?
##	}

    $ns at 0.01 "plotWindow $stcp $cwnd01_file"; ##  Start the probe !!
	  
} elseif { $simpleOneLeftRightFlow == 2 } {
    set fromFailedLeaf 1; # indicates the flow is from a failed leaf
    set PoorFlow 0
    set stcp [build-tcp-more $host(0) $host([expr $totalHosts/2]) 0.01 $SRC $SINK 1 0 $pkSize $dctcp_enable $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $PoorFlow]

    $ns at 0.01 "plotWindow $stcp $cwnd01_file"; ##  Start the probe !!

} elseif { $simpleOneLeftRightFlow == 3 } {
    set fromFailedLeaf 1; # indicates the flow is from a failed leaf
    set PoorFlow 0
    set stcp1 [build-tcp-more $host(0) $host([expr $totalHosts/2]) 0.01 $SRC $SINK 1 0 $pkSize $dctcp_enable $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $PoorFlow]
    set stcp2 [build-tcp-more $host(1) $host([expr 1+($totalHosts/2)]) 0.01 $SRC $SINK 2 1 $pkSize $dctcp_enable $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $PoorFlow]

    ##proc build-tcp-more { src dest startTime tcp_src tcp_sink fid node_id pktSize dctcp flowBender FlowCell FlowcellSize RoundRobin FailureAware FailureRatio FailedLinkIndex FromFailedLeaf SelSpraying PoorFlow}
    $ns at 0.01 "plotWindow $stcp1 $cwnd01_file"; ##  Start the probe !!
    $ns at 0.01 "plotWindow $stcp2 $cwnd02_file"; ##  Start the probe !!

} elseif { $simpleOneLeftRightFlow == 4 } {
    ## run 4 size-limited flows from diff hosts

    if { $singleShortFlowSize > $fs_threshold } { #  was previously : $fl_size > $fs_threshold
	set isElephant 1;
    } else {
	set isElephant 0;
    }

    ##set singleShortFlowSize [expr $singleShortFlowSize * $pkSize]
    set singleShortFlowSize [expr $singleShortFlowSize * 1000] ; ## assuming value from Bash Script is in KB
    set fromFailedLeaf 1; # indicates the flow is from a failed leaf
    set PoorFlow 0

   
    for { set x 0 } { $x < 4 } {incr x } {
	if { $x==3 && $SelectiveSpraying==1 } {
	    set PoorFlow 1;
	}
	set stcp($x) [build-short-lived $host($x) $host([expr $x+($totalHosts/2)]) $pkSize [expr 1+$x] $x 0.01 $SRC $SINK $singleShortFlowSize $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0];  ## $RealisticFailure $FailureCase
    }

    $ns at 0.01 "plotWindow $stcp(0) $cwnd01_file"; ##  Start the probe !!
    ##$ns at 0.01 "plotWindow $stcp(1) $cwnd02_file"; ##  Start the probe !!

} elseif { $simpleOneLeftRightFlow == 5 } {

    if { $singleShortFlowSize > $fs_threshold } { #  was previously : $fl_size > $fs_threshold
	set isElephant 1;
    } else {
	set isElephant 0;
    }

    set PoorFlow 0
    set singleShortFlowSize [expr $singleShortFlowSize * $pkSize]
    set fromFailedLeaf 1; # indicates the flow is from a failed leaf
    for { set x 0 } { $x < 4 } {incr x } {
	if { $x==3 && $SelectiveSpraying==1 } {
	    set PoorFlow 1;
	}
	set stcp($x) [build-short-lived $host($x) $host([expr $x+($totalHosts/2)]) $pkSize [expr 1+$x] $x 0.01 $SRC $SINK $singleShortFlowSize $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0]
    }

    $ns at 0.01 "plotWindow $stcp(0) $cwnd01_file"; ##  Start the probe !!
    set PoorFlow 0;
    set fromFailedLeaf 0; # indicates the flow is not from a failed leaf
    for { set y 0 } { $y < 4 } {incr y } {
	set stcp_other($y) [build-short-lived $host([expr $x+$y]) $host([expr $y+($totalHosts/2)]) $pkSize [expr 1+$x+$y] [expr $x+$y] 0.01 $SRC $SINK $singleShortFlowSize $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0]
	##set stcp_other($y) [build-tcp-more $host([expr $x+$y]) $host([expr $y+($totalHosts/2)]) 0.01 $SRC $SINK [expr 1+$x+$y] [expr $x+$y] $pkSize $dctcp_enable $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $SelectiveSpraying $PoorFlow]
    }

    $ns at 0.01 "plotWindow $stcp_other(0) $cwnd02_file"; ##  Start the probe !!

} elseif { $simpleOneLeftRightFlow == 6 } {

    if { $singleShortFlowSize > $fs_threshold } { #  was previously : $fl_size > $fs_threshold
	set isElephant 1;
    } else {
	set isElephant 0;
    }

    set PoorFlow 0;
    set singleShortFlowSize [expr $singleShortFlowSize * $pkSize]

    set fromFailedLeaf 1; # indicates the flow is from a failed leaf
    for { set x 0 } { $x < 1 } {incr x } {
	set stcp($x) [build-short-lived $host($x) $host([expr $x+($totalHosts/2)]) $pkSize [expr 1+$x] $x 0.01 $SRC $SINK $singleShortFlowSize $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0]
    }

    $ns at 0.01 "plotWindow $stcp(0) $cwnd01_file"; ##  Start the probe !!

    set x 4;
    set PoorFlow 0;
    set fromFailedLeaf 0; # indicates the flow is not from a failed leaf
    for { set y 0 } { $y < 1 } {incr y } {
	set stcp_other($y) [build-short-lived $host([expr $x+$y]) $host([expr 1+$y+($totalHosts/2)]) $pkSize [expr 1+$x+$y] [expr $x+$y] 0.01 $SRC $SINK $singleShortFlowSize $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0]
	##set stcp_other($y) [build-tcp-more $host([expr $x+$y]) $host([expr $y+($totalHosts/2)]) 0.01 $SRC $SINK [expr 1+$x+$y] [expr $x+$y] $pkSize $dctcp_enable $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $SelectiveSpraying $PoorFlow]
    }

    $ns at 0.01 "plotWindow $stcp_other(0) $cwnd02_file"; ##  Start the probe !!

}


#######################################################  SHORT FLOWS GOING THRU THE CORE SWITCH

#puts "I am alive and kicking!!!! 7"

if { $simpleLeftRightFlows == 1} {
    if { $debugMsgs==1 } {
	puts "simpleLeftRightFlows == 1!...."
    }
    ##puts "Arrival: Poisson with inter-arrival [expr 1/$lambda * 1000] ms"

    ## Question here is whether we should have separate rv_nbytes for each of the sending hosts....??
    set rng2 [new RNG]
    #$rng2 seed [expr 33*$i+4369*$j]
    ##$rng2 seed [expr 33*5+4369*9]
    $rng2 seed [expr 33*($run_i+1)+4369*($run_i+3)]
    set rv_nbytes [new RandomVariable/Empirical];
    $rv_nbytes use-rng $rng2
    $rv_nbytes set interpolation_ 2	
    if { $webSearchCDF == 1 } {
	$rv_nbytes loadCDF "CDF_dctcp.tcl"
	if { $debugMsgs==1 } {
	    puts "FlowSize: Empirical Distribution from CDF_dctcp.tcl (WEB SEARCH) ... Mean Flow Size = $av_fsize !!"
	}
    } else {
	$rv_nbytes loadCDF "CDF.tcl"
	if { $debugMsgs==1 } {
	    puts "FlowSize: Empirical Distribution from CDF.tcl (DATA MINING) ... Mean Flow Size = $av_fsize !!"
	}
    }
    ##set av_inter_arrival [expr (($av_fsize*($pkSize+40)*8.0) / ($mice_load*$btnk_bw_link*1000000))];

    ##set av_inter_arrival [expr (($av_fsize*($pkSize+40)*8.0) / ($mice_load*$btnk_bw*1000000))]; ## 22-June, Reverted to this.... 27-April: Changed from 
    set av_inter_arrival [expr (($av_fsize*1000.0*8.0) / ($mice_load*$btnk_bw*1000000))]; ## July 14, 2016

#    puts "I am alive and kicking!!!! 7-1"

    if { $debugMsgs==1 } {
	puts "SHORT FLOWS THRU CORE: mean inter-arrival time = $av_inter_arrival";
    }
    if {$TOPOLOGY == "LEAFSPINE"} {
	if { $TrafficSourceLeftmostRackOnly != 1 } {
	    set av_inter_arrival [expr $av_inter_arrival / ($numToRs/2)]; # generate for half of the ToRs
	}
    } elseif {$TOPOLOGY == "FATTREE"} {
	set av_inter_arrival [expr $av_inter_arrival / ($K*$K/4)]; # pods on the left (K/2) and each pod has (K/2) ToRs
	##set av_inter_arrival [expr $av_inter_arrival * ($K*$K/4)]; # all pods (K) and each pod has (K/2) ToRs
    } else {
	set av_inter_arrival [expr $av_inter_arrival / ($totalHosts/2)]; # generate traffic according to num of sending hosts
    }
    puts "SHORT FLOWS THRU CORE: mean inter-arrival time (for whole left tree) = $av_inter_arrival";
    # VIP:: x1000000 because btnk_bw is in Mbps .... Div by $totalHosts/2 because we are generating for all left tree hosts in a 
    # iterative way, so we must take into account that this load is being generated collectively for half of the hosts
    # unlike the single-root tree topology, where btnk_bw had a different meaning

    set short_arrival [new RNG]; # This rv is used for generating inter-arrival times
    $short_arrival seed 2
    set s_arrival [new RandomVariable/Exponential]
    $s_arrival set avg_ $av_inter_arrival
    $s_arrival use-rng $short_arrival

#    puts "I am alive and kicking!!!! 7-2"

    #set i $min_flow
    set sink 0
    set short_flow_id 800
    set global_time 0.001

    ## Calculate probabilities for PoorFlow or notPoorFlow
    if { ($FailureCase==1 || $FailureCase==2) && $SelectiveSpraying==1 } {
	if {$TOPOLOGY == "LEAFSPINE"} {
	    set poorLinkCapacity [expr $SPS_PoorPathFlow_Multiplier * ($bw_torAgg/1000.0) / $FailureRatio]
	    set totalCapacity [expr (($numAggs-1)*($bw_torAgg/1000.0)) + (($bw_torAgg/1000.0) / $FailureRatio)]
##	    set totalCapacity [expr (($numAggs-1)*($bw_torAgg/1000.0)) + $poorLinkCapacity]
	    puts "SMI-TEST: poorLinkCapacity=$poorLinkCapacity; totalCapacity=$totalCapacity; "

	    set rng_poorLink [new RNG]; 
	    $rng_poorLink seed [expr 29*($run_i+5)+4351*($run_i+17)]
	    set chanceHashFFL [new RandomVariable/Uniform]
	    set chanceHash2FL [new RandomVariable/Uniform]
	    $chanceHashFFL use-rng $rng_poorLink
	    $chanceHash2FL use-rng $rng_poorLink
	    $chanceHashFFL set min_ -0.5
	    $chanceHash2FL set min_ -0.5
	    $chanceHashFFL set max_ [expr $totalCapacity-1+0.5]
	    $chanceHash2FL set max_ [expr (($numAggs-1)*$totalCapacity)-1+0.5]

	    set chanceHashGLGL [new RandomVariable/Uniform]
	    $chanceHashGLGL use-rng $rng_poorLink
	    $chanceHashGLGL set min_ -0.5
	    $chanceHashGLGL set max_ [expr ($numAggs-1+0.5)]
	}
    }

##    while {$global_time <= [expr $sim_time/40]} {}

#    puts "I am alive and kicking!!!! 7-3"

    while {$global_time <= [expr $sim_time/40]} {
	
	### this makes sure all senders are on the left half of the topology 
	if { $sink == [expr $totalHosts/2] } {
	    set sink 0
	}

	###set transfer_size [expr [$short_tcp value]]
	set transfer_size [expr ceil ([$rv_nbytes value])] ; ## common for all types of flows...

	if { $transfer_size > $fs_threshold } { #  was previously : $fl_size > $fs_threshold
	    set isElephant 1;
	} else {
	    set isElephant 0;
	}

	# Calculate SrcSink value so that it the source sink stays within the leftmost rack
	if { $TrafficSourceLeftmostRackOnly == 1 } {
	    set srcSink [expr $sink % $num_hosts_per_leaf]
	} else {
	    set srcSink $sink;
	}

	# Calculate a DestSink value and make sure the destination is within the leftmost rack of the right subtree
	if { $TrafficDestinationSingleRack == 1 } {
	    set offsetHosts [expr (int(ceil($numToRs/2))) * $num_hosts_per_leaf]
	    set destSink [expr ($sink % $num_hosts_per_leaf) + $offsetHosts]
	} else {
	    set destSink [expr $sink+($totalHosts/2)]
	}

	set PoorFlow 0; ## Default behavior
	set sourceLeaf [expr $srcSink / $num_hosts_per_leaf];
	set destLeaf [expr $destSink / $num_hosts_per_leaf]; ## 11-Mar-2016

	set fromFailedLeaf 0; ## default 11-Mar-16
	set toFailedLeaf 0; ## default 11-Mar-16

	##if { $FailureCase==1  && ($sourceLeaf==$FailedLeaf || $destLeaf==$FailedLeaf) } {}

	if { $FailureCase==0 } {
	    ## Do Nothing??
	} else {
	    ## Either partial (1) or full failure (2) ...
	    if { $sourceLeaf==$FailedLeaf } {
		set fromFailedLeaf 1; # indicates the flow is from a failed leaf
	    } elseif { $destLeaf==$FailedLeaf } {
		set toFailedLeaf 1; # indicates the flow is from a failed leaf
	    } else {
		## Good leaf to good leaf flow...
	    }

	    set failureDetected 0;
	    if { $RealisticFailure==1 } {
		if { $global_time > [expr $FailureStartTime + $FailureDetectionDelay] } {
		    if { $FailureDuration==0 || ($FailureDuration > 0 && $global_time < [expr $FailureStartTime + $FailureDuration + $FailureDetectionDelay]) } {
			set failureDetected 1;
		    }
		}
	    }

	    ## Manage the Poor Flow mapping (use probabilities)
	    if { $SelectiveSpraying==1 && $HealthyPathOnly==0 } {
		if { $RealisticFailure==1 && $failureDetected==1 } {
		    ##if { $DynamicMapping==1 || ($transfer_size < $SPS_Thresh) } { }
		    if { $DynamicMapping==1 || (($toFailedLeaf==1 || $fromFailedLeaf==1) && ($transfer_size < $SPS_Thresh)) || ( ($toFailedLeaf==0 && $fromFailedLeaf==0) && ($transfer_size < $SPS_Thresh_GL2GL) )} {
			if { $toFailedLeaf==1 } {
			    if { $FailureCase==1 } {
				##set randNumber [expr {int(rand()*$totalCapacity*($numAggs-1)) + 1}];
				set hashFlow [expr round([expr [$chanceHash2FL value]])]
				if { $hashFlow < $poorLinkCapacity } {
				    set PoorFlow 1; 
				}
			    }
			} elseif { $fromFailedLeaf==1 } {
			    if { $FailureCase==1 } {
				##set randNumber [expr {int(rand()*$totalCapacity) + 1}];
				set hashFlow [expr round([expr [$chanceHashFFL value]])]
				if { $hashFlow < $poorLinkCapacity } {
				    set PoorFlow 1; 
				}
			    }
			} else {
			    set hashFlow [expr round([expr [$chanceHashGLGL value]])]
			    if { $hashFlow < 1 } { 
				if { $DA_SprayOnly==1 && $DA_HashSome==0 } {
				    ## do nothing...
				} else {
				    set PoorFlow 1; ## GL to GL flows being hashed 24-June-2016 // do same for full failure, then do all this for all-to-all flows scenario as well
				}
			    } 
			}
		    }
		}
	    } 
	}

	if { $debugMsgs==1 } {
	    puts "SMI-TEST: srcSink=$srcSink; destSink=$destSink; sourceLeaf=$sourceLeaf; FailedLeaf=$FailedLeaf; fromFailedLeaf=$fromFailedLeaf;"
	}
	set short_flow_id [expr $short_flow_id + 1]
	set transfer_size [expr $transfer_size * 1000.0]; # changed on 24-Mar-16 // The CDF files should indicate values in KB, not PktSize... July 02, 2016
	##set transfer_size [expr $transfer_size * ($pkSize+40)]; ## Used from 27 June onwards

	##set 
	##stcp build-short-lived 
	##$host($srcSink) $host($destSink) $pkSize $short_flow_id $srcSink $global_time $SRC $SINK $transfer_size $dctcp_enable $isElephant $flowBender $FlowCell 
	##$FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex $fromFailedLeaf $toFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0

	## New parameters 8-Mar-2017
	set DAFlow 0;
	set scndFailedLinkLeaf 0;
	set scndFailedLinkSpine 0;

	set stcp [build-short-lived $host($srcSink) $host($destSink) $pkSize $short_flow_id $srcSink $global_time $SRC $SINK $transfer_size $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLeaf $FailedLinkIndex $fromFailedLeaf $toFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 0 $DAFlow $scndFailedLinkLeaf $scndFailedLinkSpine $sourceLeaf $destLeaf] ;

	## TRACE-OPTIMAL (start-time, sizeBytes, source, destination, N/S_flow)
	if { $flow_trace == 1 } {
	    puts $flow_trace_out "$global_time $transfer_size $srcSink $destSink 0"
	}

	set inter [expr [$s_arrival value]]
	set global_time [expr $global_time + $inter]
	set sink [expr $sink + 1];
    }

    if { $debugMsgs==1 } {
	puts "Done with the creation of short flows for left-to-right scenario!!"
    }
}

##puts "I am alive and kicking!!!! 8"

#puts "I am alive and kicking!!!! 8-1: AlltoAllFlows=$AlltoAllFlows"

#### Short Flows: GOING THRU AGG SWITCHES ONLY

#### All to All short flows

## All to All short flows

if {$AlltoAllFlows == 1} {
    puts "I am alive and kicking!!!! 9"

    set rng2 [new RNG]
    $rng2 seed [expr 33*($run_i+1)+4369*($run_i+3)]
    set rv_nbytes [new RandomVariable/Empirical];
    $rv_nbytes use-rng $rng2
    $rv_nbytes set interpolation_ 2	
    if { $webSearchCDF == 1 } {
	$rv_nbytes loadCDF "CDF_dctcp.tcl"
	if { $debugMsgs==1 } {
	    puts "FlowSize: Empirical Distribution from CDF_dctcp.tcl (WEB SEARCH) ... Mean Flow Size = $av_fsize !!"
	}
    } else {
	$rv_nbytes loadCDF "CDF.tcl"
	if { $debugMsgs==1 } {
	    puts "FlowSize: Empirical Distribution from CDF.tcl (DATA MINING) ... Mean Flow Size = $av_fsize !!"
	}
    }

    if { $debugMsgs==1 } {
	puts "TEST: mice-load = $mice_load;  btnk_bw = $btnk_bw; btnk_bw * 1000000 = [expr $btnk_bw*1000000] "
    }
    
    ##set av_inter_arrival [expr (($av_fsize*($pkSize+40)*8.0) / ($mice_load*$btnk_bw*1000000))]; ## 22-June: Reverted to this now....
    set av_inter_arrival [expr (($av_fsize*1000.0*8.0) / ($mice_load*$btnk_bw*1000000))]; ## July 14th, 2016


    if { $debugMsgs==1 } {
	puts "All-to-All Traffic: mean inter-arrival time = $av_inter_arrival";
    }
    ## set av_inter_arrival [expr (($av_file_size*1024*8) / ($mice_load*$btnk_bw*1000000*2))];  # OLD PASE

    if {$TOPOLOGY == "LEAFSPINE"} {
	set av_inter_arrival [expr $av_inter_arrival / $numToRs]; # generate for all of the ToRs
    } elseif {$TOPOLOGY == "FATTREE"} {
	if { $debugMsgs==1 } {
	    puts "TEST: (K^2)/2 [expr ($K*$K/2)]"
	}
	set av_inter_arrival [expr $av_inter_arrival / ($K*$K/2)]; # all pods (K) and each pod has (K/2) ToRs
	##set av_inter_arrival [expr $av_inter_arrival * ($K*$K/2)]; # all pods (K) and each pod has (K/2) ToRs
    } else {
	set av_inter_arrival [expr $av_inter_arrival / $totalHosts]; # generate traffic according to num of sending hosts
    }

    if { $debugMsgs==1 } {
	puts "SHORT FLOWS THRU CORE: mean inter-arrival time (for whole left tree) = $av_inter_arrival";
    }
    set short_arrival [new RNG]; # This rv is used for generating inter-arrival times
    $short_arrival seed 2
    set s_arrival [new RandomVariable/Exponential]
    $s_arrival set avg_ $av_inter_arrival
    $s_arrival use-rng $short_arrival

    #set i $min_flow
    set sink 0
    set short_flow_id 8000
    set global_time 0.0

    set rng3 [new RNG]; 
    $rng3 seed [expr 22*($run_i+1)+4369*($run_i+5)]
    set index [new RandomVariable/Uniform]
    $index use-rng $rng3
    $index set min_ -0.5
    $index set max_ [expr $totalHosts-1+0.5]

    ### 0==inter-rack (same agg) ; 1==intra-rack ; 2==inter-rack (thru core)
    set ranNumGen4 [new RNG]; 
    $ranNumGen4 seed [expr 23*($run_i+1)+4369*($run_i+7)]
    set flowType [new RandomVariable/Uniform]
    $flowType use-rng $ranNumGen4
    $flowType set min_ -0.5

    if {$TOPOLOGY == "LEAFSPINE"} {
	$flowType set max_ 1.5
    } elseif {$TOPOLOGY == "FATTREE"} {
	$flowType set max_ 2.5
    } else {
	$flowType set max_ 1.5
    }

    puts "I am alive and kicking!!!! 9-1"

    ## Some random numbers are based on the type of topology chosen...

    if {$TOPOLOGY == "LEAFSPINE"} {

	set rng_leaf [new RNG]; 
	$rng_leaf seed [expr 29*($run_i+5)+4351*($run_i+17)]
	set leaf_index [new RandomVariable/Uniform]
	$leaf_index use-rng $rng_leaf
	$leaf_index set min_ -0.5
	$leaf_index set max_ [expr $numToRs - 1 + 0.5]

	set rng_host [new RNG]; 
	$rng_host seed [expr 37*($run_i+7)+4351*($run_i+19)]
	set host_index [new RandomVariable/Uniform]
	$host_index use-rng $rng_host
	$host_index set min_ -0.5
	$host_index set max_ [expr $num_hosts_per_leaf - 1 + 0.5]

    } else {
	if {$TOPOLOGY == "FATTREE"} {
	    set rng_pod [new RNG]; 
	    $rng_pod seed [expr 29*($run_i+1)+4351*($run_i+11)]
	    set pod_index [new RandomVariable/Uniform]
	    $pod_index use-rng $rng_pod
	    $pod_index set min_ -0.5
	    $pod_index set max_ [expr $K-1+0.5]

	    set rng_tor [new RNG]; 
	    $rng_tor seed [expr 19*($run_i+3)+4351*($run_i+13)]
	    set tor_index [new RandomVariable/Uniform]
	    $tor_index use-rng $rng_tor
	    $tor_index set min_ -0.5
	    $tor_index set max_ [expr $num_tors_per_pod - 1 + 0.5]
	}
    }

    set interRackThruAggCount 0
    set interRackThruCoreCount 0
    set intraRackCount 0

    for { set x 0 } { $x < $totalHosts } {incr x } {
	set src_count($x) 0
	set dst_count($x) 0
    }

    ## TODO: Check to see if we are calculating the probabilities correctly for multiple failures
    ## If two failures occur, the specific leaf-leaf pair could be facing:
    ## Case 1: No failures (upto 2 DA spines)
    ## Case 2: 1 failure (upto 1 DA spine)
    ## Case 3: 2 failures (no DA spines possible)

    ## Calculate probabilities for PoorFlow or notPoorFlow
    if { ($FailureCase==1 || $FailureCase==2) && $SelectiveSpraying==1 } {
	if {$TOPOLOGY == "LEAFSPINE"} {
	    set poorLinkCapacity [expr $SPS_PoorPathFlow_Multiplier * ($bw_torAgg/1000.0) / $FailureRatio]
	    
	    set totalCapacity [expr (($numAggs-1)*($bw_torAgg/1000.0)) + (($bw_torAgg/1000.0) / $FailureRatio)]

	    if { $MultipleFailure==1 } {
		set totalCapacityTwoFailures [expr (($numAggs-2)*($bw_torAgg/1000.0)) + (2 * (($bw_torAgg/1000.0) / $FailureRatio))]
	    }

	    ## set totalCapacity [expr (($numAggs-1)*($bw_torAgg/1000.0)) + $poorLinkCapacity]
	    if { $debugMsgs==1 } {
		puts "SMI-TEST: poorLinkCapacity=$poorLinkCapacity; totalCapacity=$totalCapacity; "
	    }

	    set rng_poorLink [new RNG]; 
	    $rng_poorLink seed [expr 29*($run_i+5)+4351*($run_i+17)]
	    set chanceHashFFL [new RandomVariable/Uniform]
	    set chanceHash2FL [new RandomVariable/Uniform]
	    $chanceHashFFL use-rng $rng_poorLink
	    $chanceHash2FL use-rng $rng_poorLink
	    $chanceHashFFL set min_ -0.5
	    $chanceHash2FL set min_ -0.5
	    $chanceHashFFL set max_ [expr $totalCapacity-1+0.5]
	    ##$chanceHash2FL set max_ [expr (($numAggs-1)*$totalCapacity)-1+0.5] ; ## commented 19-Feb-17
	    $chanceHash2FL set max_ [expr (($numToRs-1)*$totalCapacity)-1+0.5] ; ## This should be multiplied by numToRs-1, not numAggs-1, 19-Feb-17

	    ## 19-Feb-17
	    ## handle the probabilities to manage those flows that have 2 failures in its paths....
	    ## furthermore handle the case of 1 failure in paths... and 1 other failure whose effects will be felt....
	    if { $MultipleFailure==1 } {

		## Possibilities for now:
		## 0 direct failures, 2 DA spines ... FFL and 2FL
		## 1 direct failure, 1 DA spine ... ... FFL and 2FL
		## 2 direct failures, no DA spines ... FFL and 2FL --> Probably already managed.

		## set index [expr ($src_leaf_ind * $numToRs) + $dst_leaf_ind]
		## leafPairNumFailures($index);

		## $totalCapacityTwoFailures;
		set chanceSprayWeakLinks_FFL [new RandomVariable/Uniform]
		set chanceSprayWeakLinks_2FL [new RandomVariable/Uniform]
		$chanceSprayWeakLinks_FFL use-rng $rng_poorLink
		$chanceSprayWeakLinks_2FL use-rng $rng_poorLink
		$chanceSprayWeakLinks_FFL set min_ -0.5
		$chanceSprayWeakLinks_2FL set min_ -0.5
		$chanceSprayWeakLinks_FFL set max_ [expr $totalCapacityTwoFailures-1+0.5]
		$chanceSprayWeakLinks_2FL set max_ [expr (($numToRs-1)*$totalCapacityTwoFailures)-1+0.5] ; ## confused... should this be multiplied by (numAggs-1) or (numAggs-2) ??
		
		## Either store the number of failures and number of DA spines for each leaf-leaf combo separately, and use that stored info 
		## accordingly to generate the necessary probabilities...
		## or calculate per flow...

		## instantiate a matrix storing the number of direct failures faced by each srcLeaf-destLeaf pair
		for { set leaf_i 0 } { $leaf_i < $numToRs } { incr leaf_i } {
		    for { set leaf_j 0 } { $leaf_j < $numToRs } { incr leaf_j } {
			set index [expr ($leaf_i * $numToRs) + $leaf_j]
			set leafPairNumFailures($index) 0;
			if { $leaf_i != $leaf_j } {
			    for { set k 0 } { $k < [array size failedLinkLeafs] } { incr k } {
				if { $failedLinkLeafs($k)==$leaf_i || $failedLinkLeafs($k)==$leaf_j } {
				    incr leafPairNumFailures($index);
				}
			    }
			}
		    }
		}
		## at the moment, we don't analyze the number of DA spines, we may just assume that for each LL pair, DA_spines = NumFailures - leafPairNumFailures
	    }

	    set chanceHashGLGL [new RandomVariable/Uniform]
	    $chanceHashGLGL use-rng $rng_poorLink
	    $chanceHashGLGL set min_ -0.5
	    $chanceHashGLGL set max_ [expr ($numAggs-1+0.5)]

	    if { $MultipleFailure==1 } {
		set chanceSprayAffectedSpines_GLGL [new RandomVariable/Uniform]
		$chanceSprayAffectedSpines_GLGL use-rng $rng_poorLink

		$chanceSprayAffectedSpines_GLGL set min_ -0.5
		$chanceSprayAffectedSpines_GLGL set max_ [expr ($numAggs-3+0.5)] ; ## since we expect 2 spines to be DA affected here, so chance is 1/(numAggs-2) .... 

		set chanceHashAffectedSpine [new RandomVariable/Uniform]
		$chanceHashAffectedSpine use-rng $rng_poorLink

		$chanceHashAffectedSpine set min_ -0.5
		$chanceHashAffectedSpine set max_ [expr ($numAggs-2+0.5)] ; ## since we expect 1 DA spine here (and 1 spine is failed otherwise) , so chance is 1/(numAggs-1) .... 
	    }

	} elseif {$TOPOLOGY == "FATTREE"} {
	    ## TODO:
	} else {
	    ## TODO:
	}
    }

    #    puts "I am alive and kicking!!!! 9-2"

    ##while {$global_time <= [expr $sim_time/2]}
    while {$global_time <= [expr $sim_time/40]} {	
	if { $sink == $totalHosts } {
	    set sink 0
	}

	set flowTypeRand [expr round([expr [$flowType value]])]

	if { $flowTypeRand == 0 } {  
	    ## Handle both Fat-Tree & Leaf-Spine ... ## case of inter-rack (same agg)
	    set interRackThruAggCount [expr $interRackThruAggCount + 1]

	    if {$TOPOLOGY == "LEAFSPINE"} {
		## LS: randomly select a src_Leaf (from $numToRs) and a dest_Leaf (from $numToRs-1)
		set src_leaf_ind [expr round([expr [$leaf_index value]])]
		set dst_leaf_ind [expr round([expr [$leaf_index value]])]
		while { $src_leaf_ind == $dst_leaf_ind } {
		    set dst_leaf_ind [expr round([expr [$leaf_index value]])]
		}
		## randomly select src_host under src_Leaf (from $num_hosts_per_leaf)
		set src_host_in_rack [expr round([expr [$host_index value]])]
		set src_ind [expr ($src_leaf_ind*$num_hosts_per_leaf)+$src_host_in_rack]
		## randomly select dest_host under dest_Leaf (from $num_hosts_per_leaf)
		set dst_host_in_rack [expr round([expr [$host_index value]])] ; # no need for == while loop
		set dst_ind	[expr ($dst_leaf_ind*$num_hosts_per_leaf)+$dst_host_in_rack]				
		
	    } elseif {$TOPOLOGY == "FATTREE"} {
		## FT: randomly select an agg (from K*K/2); 
		set pod_ind [expr round([expr [$pod_index value]])]
		## randomly select src_ToR (K/2) and dest_ToR (K/2 - 1) below the selected agg
		set src_tor_in_pod [expr round([expr [$tor_index value]])]
		set dst_tor_in_pod [expr round([expr [$tor_index value]])]
		while { $src_tor_in_pod == $dst_tor_in_pod } {
		    set dst_tor_in_pod [expr round([expr [$tor_index value]])]
		}
		
		set src_host_in_pod [expr round([expr [$tor_index value]])]
		set src_ind [expr ((($pod_ind*$num_nodes_per_pod)+$src_tor_in_pod)*$num_hosts_per_tor)+$src_host_in_pod]
		set dst_host_in_pod [expr round([expr [$tor_index value]])] ; ## no need for == while loop
		set dst_ind	[expr ((($pod_ind*$num_nodes_per_pod)+$dst_tor_in_pod)*$num_hosts_per_tor)+$dst_host_in_pod]

	    } else { 
		## generic topology case
		set src_ind [expr round([expr [$index value]])]
		set dst_ind [expr round([expr [$index value]])]
		while { $src_ind == $dst_ind } {
		    set dst_ind [expr round([expr [$index value]])]
		}
	    }
	} elseif { $flowTypeRand == 1 } { 
	    ## Handle both Fat-Tree & Leaf-Spine ... ## case of intra-rack ...
	    set intraRackCount [expr $intraRackCount + 1]

	    if {$TOPOLOGY == "LEAFSPINE"} {
		## LS: randomly select a leaf (from $numToRs)
		set src_leaf_ind [expr round([expr [$leaf_index value]])]
		set dst_leaf_ind $src_leaf_ind ; ## Added 11-May-2016 
		## randomly select a src_host and a dest_host (from $num_hosts_per_leaf) below the selected leaf
		set src_host_in_rack [expr round([expr [$host_index value]])]
		set dst_host_in_rack [expr round([expr [$host_index value]])] ;
		while { $src_host_in_rack == $dst_host_in_rack } {
		    set dst_host_in_rack [expr round([expr [$host_index value]])]
		}

		set src_ind [expr ($src_leaf_ind*$num_hosts_per_leaf)+$src_host_in_rack]
		set dst_ind	[expr ($src_leaf_ind*$num_hosts_per_leaf)+$dst_host_in_rack]

	    } elseif {$TOPOLOGY == "FATTREE"} {
		## FT: randomly select a pod
		set pod_ind [expr round([expr [$pod_index value]])]
		## randomly select a ToR (K/2) within the selected pod
		set tor_in_pod [expr round([expr [$tor_index value]])]

		## randomly select src_host (from K/2) and dest_host (from K/2 - 1) below the selected ToR
		set src_host_in_pod [expr round([expr [$tor_index value]])]
		set dst_host_in_pod [expr round([expr [$tor_index value]])]
		while { $src_host_in_pod == $dst_host_in_pod } {
		    set dst_host_in_pod [expr round([expr [$tor_index value]])]
		}

		set src_ind [expr ((($pod_ind*$num_nodes_per_pod)+$tor_in_pod)*$num_hosts_per_tor)+$src_host_in_pod]
		set dst_ind	[expr ((($pod_ind*$num_nodes_per_pod)+$tor_in_pod)*$num_hosts_per_tor)+$dst_host_in_pod]			

	    } else {
		## generic topology case
		set src_ind [expr round([expr [$index value]])]
		set dst_ind [expr round([expr [$index value]])]
		while { $src_ind == $dst_ind } {
		    set dst_ind [expr round([expr [$index value]])]
		}
	    }
	} else { 
	    ## certainly case of Fat-Tree ... ## case of inter-rack (thru core) ...
	    set interRackThruCoreCount [expr $interRackThruCoreCount + 1]

	    if {$TOPOLOGY == "FATTREE"} {
		## FT: Randomly select 2 separate pods, src_pod (from K pods) and dest_pod (from K-1 pods)
		set src_pod_ind [expr round([expr [$pod_index value]])]
		set dst_pod_ind [expr round([expr [$pod_index value]])]
		while { $src_pod_ind == $dst_pod_ind } {
		    set dst_pod_ind [expr round([expr [$pod_index value]])]
		}
		
		## randomly select src_ToR (K/2) and dest_ToR (K/2)
		set src_tor_in_pod [expr round([expr [$tor_index value]])]
		set dst_tor_in_pod [expr round([expr [$tor_index value]])]

		set src_host_in_pod [expr round([expr [$tor_index value]])]
		set dst_host_in_pod [expr round([expr [$tor_index value]])] ; ## no need for == while loop
		## randomly select src_host from the src_pod (from K/2 hosts), and dest_host from the dest_pod (from K/2 hosts)
		set src_ind [expr ((($src_pod_ind*$num_nodes_per_pod)+$src_tor_in_pod)*$num_hosts_per_tor)+$src_host_in_pod]
		set dst_ind	[expr ((($dst_pod_ind*$num_nodes_per_pod)+$dst_tor_in_pod)*$num_hosts_per_tor)+$dst_host_in_pod]		
		
	    } else { 
		## generic topology case			
		set src_ind [expr round([expr [$index value]])]
		set dst_ind [expr round([expr [$index value]])]
		while { $src_ind == $dst_ind } {
		    set dst_ind [expr round([expr [$index value]])]
		}
	    }
	} 
	## end of if for flow-type

	set transfer_size [expr ceil ([$rv_nbytes value])] ; ## common for all types of flows...

	if { $transfer_size > $fs_threshold } {
	    set isElephant 1;
	} else {
	    set isElephant 0;
	}

	#	puts "I am alive and kicking!!!! 9-3"

	set PoorFlow 0; ## Default behavior
	set fromFailedLeaf 0; ## default 11-May-16
	set toFailedLeaf 0; ## default 11-May-16
	##set flowFacingMultipleFailures 0; ## 19-Feb-17
	set DAFlow 0; ## set this to true for all DA flows

	if {$TOPOLOGY == "LEAFSPINE"} {

	    ## See if this flow is coming from the failed leaf
	    ##if { $FailureCase==1  && ($src_leaf_ind==$FailedLeaf || $dst_leaf_ind==$FailedLeaf) } {}

	    if { $FailureCase==0 } {
		## Do Nothing??
	    } else {
		## Either partial (1) or full failure (2) ...

		if { $MultipleFailure==1 } {
		    if { $src_leaf_ind==$FailedLeaf || $src_leaf_ind==$SecondFailedLinkLeaf } {
			set fromFailedLeaf 1; # indicates the flow is from a failed leaf
			#if { $src_leaf_ind==$FailedLeaf && $src_leaf_ind==$SecondFailedLinkLeaf } {
			#set flowFacingMultipleFailures 1;
			#}
		    } 

		    if { $dst_leaf_ind==$FailedLeaf || $dst_leaf_ind==$SecondFailedLinkLeaf } {
			set toFailedLeaf 1; # indicates the flow is going to a failed leaf
			#if { $dst_leaf_ind==$FailedLeaf && $dst_leaf_ind==$SecondFailedLinkLeaf } {
			#set flowFacingMultipleFailures 1;
			#}
		    } 

		} else {
		    if { $src_leaf_ind==$FailedLeaf } {
			set fromFailedLeaf 1; # indicates the flow is from a failed leaf
		    } elseif { $dst_leaf_ind==$FailedLeaf } {
			set toFailedLeaf 1; # indicates the flow is going to a failed leaf
		    } else {
			## This is a good leaf to good leaf flow 
		    }
		}

		set failureDetected 0;
		if { $RealisticFailure==1 } {
		    if { $global_time > [expr $FailureStartTime + $FailureDetectionDelay] } {
			if { $FailureDuration==0 || ($FailureDuration > 0 && $global_time < [expr $FailureStartTime + $FailureDuration + $FailureDetectionDelay]) } {
			    set failureDetected 1;
			}
		    }
		}

		## TODO: Use this stuff:
		## leafPairNumFailures([expr ($src_leaf_ind * $numToRs) + $dst_leaf_ind]); ## [array size failedLinkLeafs];

		## Question: Are we doing all of the below for full failure as well (28-March-2017)... this is problematic if we are creating flows
		## for the poor path, which is not possible in the case of full failure (unless the poor path means a DA flow)

		## Manage the Poor Flow mapping (use probabilities)
		if { $SelectiveSpraying==1 && $HealthyPathOnly==0 } {
		    if { $RealisticFailure==1 && $failureDetected==1 } {
			##if { ($DynamicMapping==1 || ($transfer_size < $SPS_Thresh)) && ($flowTypeRand != 1) } { }
			if { $flowTypeRand != 1 } {
			    
			    ## add a clause allowing large flows for the multipleFailure case and when it's either 2FL or FFL, but then inside the if block, make sure we never
			    ## accidentally assign a large flow as a PoorFlow, but only as a DA flow... do this for both 2FL and FFL cases

			    if { $DynamicMapping==1 || (($toFailedLeaf==1 || $fromFailedLeaf==1) && ($transfer_size < $SPS_Thresh)) || ( ($toFailedLeaf==0 && $fromFailedLeaf==0) && ($transfer_size < $SPS_Thresh_GL2GL) )
				 || ( $MultipleFailure==1 && (($toFailedLeaf==1 || $fromFailedLeaf==1) && ($transfer_size < $SPS_Thresh_GL2GL)) ) } {
				if { $toFailedLeaf==1 && $fromFailedLeaf==1 } {
				    if { $MultipleFailure==1 && $transfer_size < $SPS_Thresh } {
					## all cases here, no DA spine flows (either the flow faces 2 failures, or both failures lie on the same overall leaf-2-leaf path
					if { $failedLinkSpines(0) == $failedLinkSpines(1) } {
					    ## we apply 1/31 probability for the weak link, since both failures lie on the same overall leaf-to-leaf path
					    set hashFlow [expr round([expr [$chanceHashFFL value]])]
					    if { $hashFlow < $poorLinkCapacity } {
						set PoorFlow 1; ## no DA spines here
					    }  
					} else {
					    set hashFlow [expr round([expr [$chanceSprayWeakLinks_FFL value]])] ; ## 2 direct paths are affected....
					    if { $hashFlow < [expr 2 * $poorLinkCapacity] } {
						set PoorFlow 1; ## no DA spines here
					    }
					}
				    }
				} elseif { $toFailedLeaf==1 } {
				    if { $FailureCase==1 } {
					if { $MultipleFailure==1 } {
					    ##if { $flowFacingMultipleFailures==1 } { }
					    if { $leafPairNumFailures([expr ($src_leaf_ind * $numToRs) + $dst_leaf_ind])==2 && $transfer_size < $SPS_Thresh } {
						set hashFlow [expr round([expr [$chanceSprayWeakLinks_2FL value]])] ; ## 2 direct paths are affected....
						if { $hashFlow < [expr 2 * $poorLinkCapacity] } {
						    set PoorFlow 1; ## no DA spines here
						}
					    } else {
						set hashFlow [expr round([expr [$chanceHash2FL value]])]
						if { $hashFlow < $poorLinkCapacity && $transfer_size < $SPS_Thresh } {
						    set PoorFlow 1; 
						} else {
						    ## 28-March: Decide whether this is to be sent to the DA spine, or whether it is sprayed over the unaffected & healthy spines only
						    ## 29-March: added the check to see if both failures share the same spine, if so, no need for DA flows
						    if { ($DA_SprayOnly==1 && $DA_HashSome==0) || $failedLinkSpines(0)==$failedLinkSpines(1) } { 
							## do nothing...
						    } else { 
							set hashFlow [expr round([expr [$chanceHashAffectedSpine value]])]
							if { $hashFlow < 1 } {
							    set DAFlow 1; ## added 28-March-17
							}
						    }
						}
					    }
					} else {
					    set hashFlow [expr round([expr [$chanceHash2FL value]])]
					    if { $hashFlow < $poorLinkCapacity } {
						set PoorFlow 1; 
					    }
					}
				    }
				} elseif { $fromFailedLeaf==1 } {
				    if { $FailureCase==1 } {
					if { $MultipleFailure==1 } {
					    ##if { $flowFacingMultipleFailures==1 } {}
					    if { $leafPairNumFailures([expr ($src_leaf_ind * $numToRs) + $dst_leaf_ind])==2 && $transfer_size < $SPS_Thresh } {
						set hashFlow [expr round([expr [$chanceSprayWeakLinks_FFL value]])] ; ## 2 direct paths are affected....
						if { $hashFlow < [expr 2 * $poorLinkCapacity] } {
						    set PoorFlow 1; 
						}
					    } else {
						set hashFlow [expr round([expr [$chanceHashFFL value]])]
						if { $hashFlow < $poorLinkCapacity && $transfer_size < $SPS_Thresh } {
						    set PoorFlow 1; 
						} else {
						    ## 28-March: Decide whether this is to be sent to the DA spine, or whether it is sprayed over the unaffected & healthy spines only
						    ## 29-March: added the check to see if both failures share the same spine, if so, no need for DA flows
						    if { ($DA_SprayOnly==1 && $DA_HashSome==0) || $failedLinkSpines(0)==$failedLinkSpines(1) } { 
							## do nothing...
						    } else { 
							set hashFlow [expr round([expr [$chanceHashAffectedSpine value]])]
							if { $hashFlow < 1 } {
							    set DAFlow 1; ## added 28-March-17
							}
						    }
						}
					    }
					} else {
					    set hashFlow [expr round([expr [$chanceHashFFL value]])]
					    if { $hashFlow < $poorLinkCapacity } {
						set PoorFlow 1; 
					    }
					}
				    }
				} else {
				    ## This block means the flow is neither FFL or 2FL
				    ## Handle the possibility of both non-direct failures having a common spine... in that case, we are hashing to only 1 spine for DA flows (not 2)
				    if { $MultipleFailure==1 && $failedLinkSpines(0)!=$failedLinkSpines(1) } {
					set hashFlow [expr round([expr [$chanceSprayAffectedSpines_GLGL value]])]
					if { $hashFlow < 2 } { 
					    if { $DA_SprayOnly==1 && $DA_HashSome==0 } {
						## do nothing...
					    } else {
						set PoorFlow 1; ## GL to GL flows being hashed 24-June-2016 // do same for full failure, then do all this for all-to-all flows scenario as well
						set DAFlow 1; ## added 28-March-17
					    }
					} 
				    } else {
					set hashFlow [expr round([expr [$chanceHashGLGL value]])]; ## this works for single failure, and mult failure when both indirect failures share same spine
					if { $hashFlow < 1 } { 
					    if { $DA_SprayOnly==1 && $DA_HashSome==0 } {
						## do nothing...
					    } else {
						set PoorFlow 1; ## GL to GL flows being hashed 24-June-2016 // do same for full failure, then do all this for all-to-all flows scenario as well
						set DAFlow 1; ## added 28-March-17
					    }
					} 
				    }
				}
			    }
			}
		    }
		    #		    puts "I am alive and kicking!!!! 9-5"
		} 
		#		puts "I am alive and kicking!!!! 9-6"
	    }

	    #	    puts "I am alive and kicking!!!! 9-7"

	} elseif {$TOPOLOGY == "FATTREE"} {
	    ## TODO
	} else {
	    ## TODO
	}

	#	puts "I am alive and kicking!!!! 9-8"

	set transfer_size [expr $transfer_size * 1000.0];# $transfer_size * 1024.0 (just changed it, 27-Apr-PM) ## July 02, 2016
	##set transfer_size [expr $transfer_size * ($pkSize+40)]; ## Used from 27 June onwards
	set short_flow_id [expr $short_flow_id + 1]
	
	set isIntraRack [expr $flowTypeRand % 2]; ## $flowTypeRand is 1 for intra rack and either 0 or 2 otherwise....

	set stcp [build-short-lived $host($src_ind) $host($dst_ind) $pkSize $short_flow_id $sink $global_time $SRC $SINK $transfer_size $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLeaf $FailedLinkIndex $fromFailedLeaf $toFailedLeaf $SelectiveSpraying $HealthyPathOnly $DA_SprayOnly $PoorFlow 0 $isIntraRack $DAFlow $SecondFailedLinkLeaf $SecondFailedLinkSpine $src_leaf_ind $dst_leaf_ind]


	## TRACE-OPTIMAL (start-time, sizeBytes, source, destination, N/S_flow)
	if { $flow_trace == 1 } {
	    puts $flow_trace_out "$global_time $transfer_size $src_ind $dst_ind 0"
	}

	set src_count($src_ind) [expr $src_count($src_ind) + 1]
	set dst_count($dst_ind) [expr $dst_count($dst_ind) + 1]

	set inter [expr [$s_arrival value]]
	set global_time [expr $global_time + $inter]
	set sink [expr $sink + 1]				
    }	

    set flow_summary [open "flow_summary.out" w]
    puts $flow_summary "Inter-Rack (Thru Core): $interRackThruCoreCount (Thru Agg): $interRackThruAggCount Intra-Rack: $intraRackCount"
    
    for { set x 0 } { $x < $totalHosts } {incr x } {
	puts $flow_summary "$src_count($x) $dst_count($x); " ; ## -nonewline 
    }
    
    close $flow_summary
} 
# All to All ends here #################################################################################################################

#puts "I am alive and kicking!!!! 10"

## NORTH SOUTH CODE PORTION  ###################################################################################################################

if { $NorthSouthTraffic==1 } {
    ## If North-South traffic, create new flows from each endhost to a random remote user after every NorthSouthInterArrival time period

    puts "Inside North-South traffic section!"

    set globalTime 0.001
    set remote_flow_id 0
    set sIndex 0

    set ranNumGen5 [new RNG]; 
    $ranNumGen5 seed [expr 29*($run_i+7)+4369*($run_i+19)]
    set remoteUsr [new RandomVariable/Uniform]
    $remoteUsr use-rng $ranNumGen5
    $remoteUsr set min_ -0.5

    if {$TOPOLOGY=="LEAFSPINE"} {	
	$remoteUsr set max_ [expr $numAggs -1 + 0.5]
    } else {
	$remoteUsr set max_ [expr $num_cores -1 + 0.5]
    }

    set rng02 [new RNG]
    $rng02 seed [expr 39*($run_i+1)+4369*($run_i+5)]
    set rvnbytes [new RandomVariable/Empirical];
    $rvnbytes use-rng $rng02
    $rvnbytes set interpolation_ 2	
    if { $webSearchCDF == 1 } {
	$rvnbytes loadCDF "CDF_dctcp.tcl"
    } else {
	$rvnbytes loadCDF "CDF.tcl"
    }

    ##set avgInterArrival [expr (($av_fsize*($pkSize+40)*8.0) / ($NorthSouthLoad * $btnk_bw_link * 1000000))]; ## 4-June FIXIT
    set avgInterArrival [expr (($av_fsize*1000.0*8.0) / ($NorthSouthLoad * $btnk_bw_link * 1000000))]; ## 4-June FIXIT
   puts "North-South Traffic: mean inter-arrival time = $avgInterArrival";

    if {$TOPOLOGY == "LEAFSPINE"} {
	set avgInterArrival [expr $avgInterArrival / $numAggs]; # generate for all remote users
    } elseif {$TOPOLOGY == "FATTREE"} {
	set avgInterArrival [expr $avgInterArrival / $num_cores]; # generate for all remote users
    } else {
	set avgInterArrival [expr $avgInterArrival / $totalHosts]; # generate traffic according to num of sending hosts
    }

    set shortArrival [new RNG]; # This rv is used for generating inter-arrival times
    $shortArrival seed 2
    set sArrival [new RandomVariable/Exponential]
    $sArrival set avg_ $avgInterArrival
    $sArrival use-rng $shortArrival

    if {$NorthSouthInterArrival == 0} {
	## this means we will use our own Web Search or Data Mining CDF files for traffic gen

	while {$globalTime <= [expr $sim_time/40]} {
	    set remoteUsrRand [expr round([expr [$remoteUsr value]])]
	    set transfer_size [expr ceil ([$rvnbytes value])] ; ## TODO June-3 ## $transfer_size

	    if { $sIndex == $totalHosts } {
		set sIndex 0
	    }

	    if { $transfer_size > $fs_threshold } {
		set isElephant 1;
	    } else {
		set isElephant 0;
	    }
	    set transfer_size [expr $transfer_size * 1000.0]; ## commented on 27 June 2016 ... back to work on July 02, 2016
	    ##set transfer_size [expr $transfer_size * ($pkSize+40)]; ## Used from 27 June onwards

	    ## src dest pktSize fid node_id startTime 
    
	    puts "NorthSouth flow: source-host=$sIndex; remote-user=$remoteUsrRand; pkSize=$pkSize; transfer_size=$transfer_size; globalTime=$globalTime;"
	    set stcp [build-short-lived $host($sIndex) $remoteNode($remoteUsrRand) $pkSize $remote_flow_id $sIndex $globalTime $SRC $SINK $transfer_size $dctcp_enable $isElephant $flowBender $FlowCell $FlowcellSize $RoundRobin $FailureAware $FailureRatio $FailedLinkIndex 0 0 0 0 0 0 1 0] ; ## Set $fromFailedLeaf==0 for now, 4th last parameter ... 2nd last parameter is NorthSouthFlow==1, last is intraRackFlow==0

	    ## TRACE-OPTIMAL (start-time, sizeBytes, source, destination, N/S_flow)
	    if { $flow_trace == 1 } {
		puts $flow_trace_out "$globalTime $transfer_size $sIndex $remoteUsrRand 1"
	    }

	    set inter [expr [$sArrival value]]
	    set globalTime [expr $globalTime + $inter]
	    set sIndex [expr $sIndex + 1]
	    set remote_flow_id [expr $remote_flow_id + 1]
	}

    } else {
	## this means we will use a fixed inter-arrival time for each flow generated at each endhost

	while {$globalTime <= [expr $sim_time/40]} {
	    for { set x 0 } { $x < $totalHosts } {incr x } {

	    }
	    set globalTime [expr $globalTime + $NorthSouthInterArrival]
	}
    }
}

## END OF NORTH SOUTH CODE PORTION  ##########################################################################################################################


## PortionOfCode that will allow us to enable Realistic Failure Mechanism of creating partial failure on the run :: 03-March-2016 SMI  #######################
if { $RealisticFailure==1 } {
    if { $FailureCase==1 } {
	## Partial failure
	$ns at $FailureStartTime "$ns bandwidth $leaf($FailedLeaf) $spine($FailedLinkIndex) [expr $bw_torAgg/$FailureRatio]Mb 'duplex'" ; ##$ns bandwidth $n0 $n2 0.1Mb 'duplex'
	##$ns at [expr $FailureStartTime+$FailureDetectionDelay] "Agent/TCP set failureDetected_ 1" ; ## TODO: Need to check to see if this works in the C++ layer
	puts "Debug: Reset bandwidth at $FailureStartTime to [expr $bw_torAgg/$FailureRatio] Mb!! "

	if { $MultipleFailure==1 } {
	    $ns at $FailureStartTime "$ns bandwidth $leaf($SecondFailedLinkLeaf) $spine($SecondFailedLinkSpine) [expr $bw_torAgg/$FailureRatio]Mb 'duplex'" ; ## 18-Feb-17
	}

	if { $FailureDuration > 0.0 } {
	    $ns at [expr $FailureStartTime+$FailureDuration] "$ns bandwidth $leaf($FailedLeaf) $spine($FailedLinkIndex) [expr $bw_torAgg]Mb 'duplex'"
	    ##$ns at [expr $FailureStartTime+$FailureDuration+$FailureDetectionDelay]  "Agent/TCP set failureDetected_ 0" ; ## TODO: Need to check ....

	    if { $MultipleFailure==1 } {
		$ns at [expr $FailureStartTime+$FailureDuration] "$ns bandwidth $leaf($SecondFailedLinkLeaf) $spine($SecondFailedLinkSpine) [expr $bw_torAgg]Mb 'duplex'" ; ## 18-Feb-17
	    }
	}
    } elseif { $FailureCase==2 } {
	## Full failure
	if { $PutFailedLinkDown==1 } {
	    $ns rtmodel-at $FailureStartTime down $leaf($FailedLeaf) $spine($FailedLinkIndex) ; ## bottleneck link

	    if { $MultipleFailure==1 } {
		$ns rtmodel-at $FailureStartTime down $leaf($SecondFailedLinkLeaf) $spine($SecondFailedLinkSpine) ; ## 18-Feb-17
	    }

	} else {
	    $ns at $FailureStartTime "$ns bandwidth $leaf($FailedLeaf) $spine($FailedLinkIndex) 0.001Kb 'duplex'" ; ##$ns bandwidth $n0 $n2 0.1Mb 'duplex'
	}
	##$ns at [expr $FailureStartTime+$FailureDetectionDelay] "Agent/TCP set failureDetected_ 1" ; ## TODO: Need to check to see if this works in the C++ layer

	if { $FailureDuration > 0.0 } {
	    if { $PutFailedLinkDown==1 } {
		$ns rtmodel-at [expr $FailureStartTime+$FailureDuration] up $leaf($FailedLeaf) $spine($FailedLinkIndex) ; ## bottleneck link

		if { $MultipleFailure==1 } {
		    $ns rtmodel-at [expr $FailureStartTime+$FailureDuration] up $leaf($SecondFailedLinkLeaf) $spine($SecondFailedLinkSpine) ; ## 18-Feb-17
		}

	    } else {
		$ns at [expr $FailureStartTime+$FailureDuration] "$ns bandwidth $leaf($FailedLeaf) $spine($FailedLinkIndex) [expr $bw_torAgg]Mb 'duplex'"
	    }
	    ##$ns at [expr $FailureStartTime+$FailureDuration+$FailureDetectionDelay]  "Agent/TCP set failureDetected_ 0" ; ## TODO: Need to check ....
	}
	## TODO: Do something at TCP level to inform the flows using HPO or SPS that they need to get their act together....!!
    }
}
## End of Code for enabling Realistic Failure Mechanism of creating partial failure on the run :: 03-March-2016 SMI  #######################

# End: agents and sources --------------------------------------

# QUEUE MONITOR CODE

if {$getQmonLinkLevelStats == 1} {
	if {$TOPOLOGY=="LEAFSPINE"} {	
		## for { set i 0 } { $i < $numToRs } { incr i } 
		## for now just settling for left-most leaf (ToR) and all its linked spines
		set futilAll_name "qmon.utilAllLeafSpine"
		set f_utilAll [open $futilAll_name w]

		set futilAllHostLeaf_name "qmon.utilAllHostLeaf"
		set f_utilAllHostLeaf [open $futilAllHostLeaf_name w]

		set fdropAll_LS_name "qmon.dropAll_LS"
		set f_dropAll_LS [open $fdropAll_LS_name w]

		set fdropAll_HL_name "qmon.dropAll_HL"
		set f_dropAll_HL [open $fdropAll_HL_name w]

		if { $simpleLeftRightFlows==1 } {
			set range [expr $numToRs/2]
		} elseif { $AlltoAllFlows==1 } {
			set range $numToRs
		} else {	 
			set range [expr $numToRs/2]; ## default case
		}

		### QMon for Leaf-Spine links....
		for { set i 0 } { $i < $range } { incr i } {
			for { set j 0 } { $j < $numAggs } { incr j } {
				set sIndex [expr $j]
				set qmon_ab($i$sIndex) [$ns monitor-queue $leaf($i) $spine($sIndex) ""]
				set bing_ab($i$sIndex) [$qmon_ab($i$sIndex) get-bytes-integrator];	# bytes integrator 
				set ping_ab($i$sIndex) [$qmon_ab($i$sIndex) get-pkts-integrator];	    # packets integrator

				set fileq($i$sIndex) "qmon.trace"
				set futil_name($i$sIndex) "qmon.util"
				set floss_name($i$sIndex) "qmon.loss"
				set fqueue_name($i$sIndex) "qmon.queue"

				append fileq($i$sIndex) "Leaf$i-Spine$sIndex"
				append futil_name($i$sIndex) "Leaf$i-Spine$sIndex"
				append floss_name($i$sIndex) "Leaf$i-Spine$sIndex"
				append fqueue_name($i$sIndex) "Leaf$i-Spine$sIndex"

				set fq_mon($i$sIndex) [open $fileq($i$sIndex) w]
				set f_util($i$sIndex) [open $futil_name($i$sIndex) w]
				set f_loss($i$sIndex) [open $floss_name($i$sIndex) w]
				set f_queue($i$sIndex) [open $fqueue_name($i$sIndex) w]

				$ns at $STATS_START  "$qmon_ab($i$sIndex) reset"
				$ns at $STATS_START  "$bing_ab($i$sIndex) reset"
				$ns at $STATS_START  "$ping_ab($i$sIndex) reset"
			
				$ns at [expr $STATS_START+$STATS_INTR] "linkDump [$ns link $leaf($i) $spine($sIndex)] $bing_ab($i$sIndex) $ping_ab($i$sIndex) $qmon_ab($i$sIndex) $STATS_INTR A-B $fq_mon($i$sIndex) $f_util($i$sIndex) $f_loss($i$sIndex) $f_queue($i$sIndex) $f_utilAll $f_dropAll_LS [expr ($i*$numAggs)+$sIndex] [expr $range*$numAggs] [expr $qLimit*$pkSize]" ; #last one $buf_bytes
			
			}
		}
		### End of QMon for Leaf-Spine links....

		### QMon for Host-Leaf links....
		for { set i 0 } { $i < $range } { incr i } {
			for { set j 0 } { $j < $num_hosts_per_leaf } { incr j } {
				#set sIndex [expr $j]
				set hIndex [expr ($i*$num_hosts_per_leaf)+$j]

				set qmon_hl($hIndex) [$ns monitor-queue $host($hIndex) $leaf($i) ""]
				set bing_hl($hIndex) [$qmon_hl($hIndex) get-bytes-integrator];	# bytes integrator 
				set ping_hl($hIndex) [$qmon_hl($hIndex) get-pkts-integrator];	    # packets integrator

				$ns at $STATS_START  "$qmon_hl($hIndex) reset"
				$ns at $STATS_START  "$bing_hl($hIndex) reset"
				$ns at $STATS_START  "$ping_hl($hIndex) reset"
		
				$ns at [expr $STATS_START+$STATS_INTR] "linkDump [$ns link $host($hIndex) $leaf($i)] $bing_hl($hIndex) $ping_hl($hIndex) $qmon_hl($hIndex) $STATS_INTR H-T 0 0 0 0 $f_utilAllHostLeaf $f_dropAll_HL $hIndex [expr $range*$num_hosts_per_leaf] [expr $qLimit*$pkSize]" ; #last one $buf_bytes
	
			}
		}		
		### End of QMon for Host-Leaf links....
		
	} elseif {$TOPOLOGY=="FATTREE"} {			

		### QMon for Agg-Core links....
		set futilAll_name "qmon.utilAllAgg"
		set f_utilAll [open $futilAll_name w]

		set fdropAll_AC_name "qmon.dropAll_AC"
		set f_dropAll_AC [open $fdropAll_AC_name w]

		if { $simpleLeftRightFlows==1 } {
			set range [expr $K/2]
		} elseif { $AlltoAllFlows==1 } {
			set range $K
		} else {	 
			set range [expr $K/2]; ## default case
		}
		
		for {set i 0} {$i < $range} {incr i} { #pod i
			for {set j 0} {$j < $num_nodes_per_pod} {incr j} { #aggr switch j in pod i
				set aIndex [expr ($i*$num_nodes_per_pod)+$j]
			  for {set k 0} {$k < [expr $K/2]} {incr k} { #
					set cIndex [expr $j*($K/2)+$k]

					set qmon_ab($aIndex$cIndex) [$ns monitor-queue $aggr($aIndex) $core($cIndex) ""]
					set bing_ab($aIndex$cIndex) [$qmon_ab($aIndex$cIndex) get-bytes-integrator];	# bytes integrator 
					set ping_ab($aIndex$cIndex) [$qmon_ab($aIndex$cIndex) get-pkts-integrator];	    # packets integrator

					set fileq($aIndex$cIndex) "qmon.trace"
					set futil_name($aIndex$cIndex) "qmon.util"
					set floss_name($aIndex$cIndex) "qmon.loss"
					set fqueue_name($aIndex$cIndex) "qmon.queue"

					append fileq($aIndex$cIndex) "Agg$aIndex"
					append fileq($aIndex$cIndex) "Core$cIndex"
					append futil_name($aIndex$cIndex) "Agg$aIndex"
					append futil_name($aIndex$cIndex) "Core$cIndex"
					append floss_name($aIndex$cIndex) "Agg$aIndex"
					append floss_name($aIndex$cIndex) "Core$cIndex"
					append fqueue_name($aIndex$cIndex) "Agg$aIndex"
					append fqueue_name($aIndex$cIndex) "Core$cIndex"

					set fq_mon($aIndex$cIndex) [open $fileq($aIndex$cIndex) w]
					set f_util($aIndex$cIndex) [open $futil_name($aIndex$cIndex) w]
					set f_loss($aIndex$cIndex) [open $floss_name($aIndex$cIndex) w]
					set f_queue($aIndex$cIndex) [open $fqueue_name($aIndex$cIndex) w]

					$ns at $STATS_START  "$qmon_ab($aIndex$cIndex) reset"
					$ns at $STATS_START  "$bing_ab($aIndex$cIndex) reset"
					$ns at $STATS_START  "$ping_ab($aIndex$cIndex) reset"
			
					$ns at [expr $STATS_START+$STATS_INTR] "linkDump [$ns link $aggr($aIndex) $core($cIndex)] $bing_ab($aIndex$cIndex) $ping_ab($aIndex$cIndex) $qmon_ab($aIndex$cIndex) $STATS_INTR A-B $fq_mon($aIndex$cIndex) $f_util($aIndex$cIndex) $f_loss($aIndex$cIndex) $f_queue($aIndex$cIndex) $f_utilAll $f_dropAll_AC [expr ($aIndex*$K/2)+$k] [expr $range*$K*$K/4] [expr $qLimit*$pkSize]" ; #last one $buf_bytes # utilAll sIndex maxLink
					# [expr ($tIndex*$K/2)+$k] [expr $K*$K*$K/4]
			  }
			}
		}		
		### End of QMon for Agg-Core links....

		### QMon for ToR-Agg links....
		set futilAllToRAgg_name "qmon.utilAllToR"
		set f_utilAllToRAgg [open $futilAllToRAgg_name w]

		set fdropAll_TA_name "qmon.dropAll_TA"
		set f_dropAll_TA [open $fdropAll_TA_name w]

		for {set i 0} {$i < $range} {incr i} { # pod i
			for {set j 0} {$j < $num_nodes_per_pod} {incr j} { # agg j of pod i
			 	set aIndex [expr ($i*$num_nodes_per_pod)+$j]
			  for {set k 0} {$k < [expr $K/2]} {incr k} { # tor k of pod i
			  	set tIndex [expr ($i*$num_nodes_per_pod)+$k] 
					#set loopCounter [expr ($aIndex*$num_nodes_per_tor)+$k]

					set qmon_ta($tIndex$aIndex) [$ns monitor-queue $tor($tIndex) $aggr($aIndex) ""]
					set bing_ta($tIndex$aIndex) [$qmon_ta($tIndex$aIndex) get-bytes-integrator];	# bytes integrator 
					set ping_ta($tIndex$aIndex) [$qmon_ta($tIndex$aIndex) get-pkts-integrator];	    # packets integrator

					$ns at $STATS_START  "$qmon_ta($tIndex$aIndex) reset"
					$ns at $STATS_START  "$bing_ta($tIndex$aIndex) reset"
					$ns at $STATS_START  "$ping_ta($tIndex$aIndex) reset"

					$ns at [expr $STATS_START+$STATS_INTR] "linkDump [$ns link $tor($tIndex) $aggr($aIndex)] $bing_ta($tIndex$aIndex) $ping_ta($tIndex$aIndex) $qmon_ta($tIndex$aIndex) $STATS_INTR T-A 0 0 0 0 $f_utilAllToRAgg $f_dropAll_TA [expr ($aIndex*$K/2)+$k] [expr $range*$K*$K/4] [expr $qLimit*$pkSize]" ; #last one $buf_bytes # utilAll sIndex maxLink

			  }
			}
		}
		### End of QMon for ToR-Agg links....

		### QMon for Host-ToR links....
		set futilAllHostToR_name "qmon.utilAllHostToR"
		set f_utilAllHostToR [open $futilAllHostToR_name w]

		set fdropAll_HT_name "qmon.dropAll_HT"
		set f_dropAll_HT [open $fdropAll_HT_name w]

		for {set i 0} {$i < $range} {incr i} { #pod i
			for {set j 0} {$j < $num_nodes_per_pod} {incr j} { #aggr switch j in pod i
				set tIndex [expr ($i*$num_nodes_per_pod)+$j]
				for {set k 0} {$k < $num_hosts_per_tor} {incr k} { #connect host to switch j in pod i
					set hIndex [expr ($tIndex*$num_hosts_per_tor)+$k]

					set qmon_ht($hIndex) [$ns monitor-queue $host($hIndex) $tor($tIndex) ""]
					set bing_ht($hIndex) [$qmon_ht($hIndex) get-bytes-integrator];	# bytes integrator 
					set ping_ht($hIndex) [$qmon_ht($hIndex) get-pkts-integrator];	    # packets integrator

					$ns at $STATS_START  "$qmon_ht($hIndex) reset"
					$ns at $STATS_START  "$bing_ht($hIndex) reset"
					$ns at $STATS_START  "$ping_ht($hIndex) reset"

					$ns at [expr $STATS_START+$STATS_INTR] "linkDump [$ns link $host($hIndex) $tor($tIndex)] $bing_ht($hIndex) $ping_ht($hIndex) $qmon_ht($hIndex) $STATS_INTR H-T 0 0 0 0 $f_utilAllHostToR $f_dropAll_HT $hIndex [expr $range*$K*$K/4] [expr $qLimit*$pkSize]" ; #last one $buf_bytes # utilAll sIndex maxLink
			 	}
	 		} 
		}	
		## end of QMon for host-ToR links
	}
}


# TODO: Need to modify for CONGA and then maybe also for UPNA-LB

for { set i 0 } { $i < $sim_time } { incr i } {
	$ns at $i "curr_time"
}

$ns rtproto DV
#set mproto DM
#set mrthandle [$ns mrtproto $mproto {}]

$ns at $sim_time "finish"

#Run the simulation
$ns run

