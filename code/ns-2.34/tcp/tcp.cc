/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1991-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/tcp/tcp.cc,v 1.180 2008/12/18 05:15:40 sallyfloyd Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "random.h"
#include "basetrace.h"
#include "hdr_qs.h"

#define debug_dctcp 0

#define debug_tcp_smi 0

int hdr_tcp::offset_;

static class TCPHeaderClass : public PacketHeaderClass {
public:
        TCPHeaderClass() : PacketHeaderClass("PacketHeader/TCP",
					     sizeof(hdr_tcp)) {
		bind_offset(&hdr_tcp::offset_);
	}
} class_tcphdr;

static class TcpClass : public TclClass {
public:
	TcpClass() : TclClass("Agent/TCP") {}
	TclObject* create(int , const char*const*) {
		return (new TcpAgent());
	}
} class_tcp;

TcpAgent::TcpAgent() 
	: Agent(PT_TCP), 
	  t_seqno_(0), dupacks_(0), curseq_(0), highest_ack_(0), 
          cwnd_(0), ssthresh_(0), maxseq_(0), count_(0), 
          rtt_active_(0), rtt_seq_(-1), rtt_ts_(0.0), 
          lastreset_(0.0), closed_(0), t_rtt_(0), t_srtt_(0), t_rttvar_(0), 
	  t_backoff_(0), ts_peer_(0), ts_echo_(0), tss(NULL), tss_size_(100), 
	  rtx_timer_(this), delsnd_timer_(this), burstsnd_timer_(this), 
	  first_decrease_(1), fcnt_(0), nrexmit_(0), restart_bugfix_(1), 
          cong_action_(0), ecn_burst_(0), ecn_backoff_(0), ect_(0), 
          use_rtt_(0), qs_requested_(0), qs_approved_(0),
	  qs_window_(0), qs_cwnd_(0), frto_(0), dctcp_maxseq(0), linkID_(0), rfactor(30), 
	  flowcell_(0), flowcellSizePkts_(43), roundRobin_(0), numUplinks(1), totalUplinkWeights(0),
	  failureRatio_(1), failureAware_(0), selectiveSpraying_(0), healthyPathOnly_(0), DA_sprayOnly_(0), poorPathFlow_(0), originallyHashed_(0), 
	  realisticFailure_(0), failureCase_(0), failureDetected_(0), failureStartTime_(0.0), failureDuration_(0.0), failureDetectionDelay_(0.0),
	  fromFailedLeaf_(0), toFailedLeaf_(0), northSouthFlow_(0), intraRackFlow_(0), dynamicMapping_(0), dynamicMappingThreshold_(0), dynamicMappingThresholdGL2GL_(0), flowBender_(0),
	  numFailures_(0), DA_Flow_(0), srcLeaf_(0), destLeaf_(0), toggleWeakSpines_(0), 
	  numDirectPartialFailurePaths(0), numDirectFullFailurePaths(0), numIndirectPartialFailurePaths(0), numIndirectFullFailurePaths(0), numDirectFailures_(0),
	  svtMapping(0), healthyLinkCapacity(0), flowsizeBytes(0), flowBender_T(0.0), flowBender_N(1), markedAckPkts_F(0), totalAckPkts(0), numRTTsCongested_(0), toggleTTL_(0), fl_bndr_timer(this)
{
#ifdef TCP_DELAY_BIND_ALL
        // defined since Dec 1999.
#else /* ! TCP_DELAY_BIND_ALL */
	bind("t_seqno_", &t_seqno_);
	bind("rtt_", &t_rtt_);
	bind("srtt_", &t_srtt_);
	bind("rttvar_", &t_rttvar_);
	bind("backoff_", &t_backoff_);
	bind("dupacks_", &dupacks_);
	bind("seqno_", &curseq_);
	bind("ack_", &highest_ack_);
	bind("cwnd_", &cwnd_);
	bind("ssthresh_", &ssthresh_);
	bind("maxseq_", &maxseq_);
        bind("ndatapack_", &ndatapack_);
        bind("ndatabytes_", &ndatabytes_);
        bind("nackpack_", &nackpack_);
        bind("nrexmit_", &nrexmit_);
        bind("nrexmitpack_", &nrexmitpack_);
        bind("nrexmitbytes_", &nrexmitbytes_);
        bind("necnresponses_", &necnresponses_);
        bind("ncwndcuts_", &ncwndcuts_);
	bind("ncwndcuts1_", &ncwndcuts1_);
	bind("dctcp_", &dctcp_);
	bind("dctcp_alpha_", &dctcp_alpha_);
	bind("temp_alpha_", &temp_alpha_);
	bind("dctcp_g_", &dctcp_g_);
	bind("dupCase1Count_", &dupCase1Count_); /*  SMI: 15-June-2015 */
	bind("dupCase2Count_", &dupCase2Count_); /*  SMI: 15-June-2015 */
	//bind("linkID_", &linkID_); /*  SMI: 12-Dec-2015 */
	bind("flowcell_", &flowcell_); /*  SMI: 12-Dec-2015 */
	bind("flowcellSizePkts_", &flowcellSizePkts_); // SMI: 12-Dec-2015 
	bind("roundRobin_", &roundRobin_); // SMI : 14-Dec-15
	bind("failureRatio_", &failureRatio_); // SMI 23-Dec-15
	//bind("failedLinkIndex_", &failedLinkIndex_); // SMI 23-Dec-15
	bind("failureAware_", &failureAware_); // SMI 23-Dec-15
	bind("fromFailedLeaf_", &fromFailedLeaf_); // SMI 4-Jan-16
	bind("toFailedLeaf_", &toFailedLeaf_); // SMI 13-Mar-16
	bind("selectiveSpraying_", &selectiveSpraying_); // SMI 15-Jan-16
	bind("healthyPathOnly_", &healthyPathOnly_); // 14 July 2016 
	bind("DA_sprayOnly_", &DA_sprayOnly_); // 15 July 2016 .. make sure delay asymmetry flows (G2GL) are only sprayed, and that too across all spines

	bind("poorPathFlow_", &poorPathFlow_); // SMI 15-Jan-16
	bind("originallyHashed_", &originallyHashed_); // Same as above, but does not change after remapping .. 14-jul-16

	bind("dynamicMapping_", &dynamicMapping_); // 16-June
	bind("dynamicMappingThreshold_", &dynamicMappingThreshold_); // 16-June
	bind("dynamicMappingThresholdGL2GL_", &dynamicMappingThresholdGL2GL_); // 13th July

	bind("flowBender_", &flowBender_); // 13-Jan-17
	bind("flowBender_T", &flowBender_T); // 3-Jul-17
	bind("flowBender_N", &flowBender_N); // 3-Jul-17

	bind("realisticFailure_", &realisticFailure_); // SMI 8-Mar
	bind("failureCase_", &failureCase_); // SMI 8-Mar
	//bind("failureDetected_", &failureDetected_); // SMI 8-Mar

	bind("northSouthFlow_", &northSouthFlow_); // SMI 4-June
	bind("intraRackFlow_", &intraRackFlow_); // SMI 22-Jul

	bind("failureStartTime_", &failureStartTime_); /* SMI 9-Mar-2016 */
	bind("failureDuration_", &failureDuration_); // If zero, then assume failure goes on till end ...
	bind("failureDetectionDelay_", &failureDetectionDelay_);
	bind("numUplinks", &numUplinks); // 9-Mar-16

	//bind("failedLinkLeaf_", &failedLinkLeaf_); // 25-Feb-17
	//bind("multipleFailure_", &multipleFailure_); // 18-Feb-17
	bind("numFailures_", &numFailures_); // 20-Feb-17
	// bind("secondFailedLinkLeaf_", &secondFailedLinkLeaf_);  // 18-Feb-17
	// bind("secondFailedLinkSpine_", &secondFailedLinkSpine_);  // 18-Feb-17
	//bind("flowFacingMultipleFailures_", &flowFacingMultipleFailures_);  // 19-Feb-17
	bind("DA_Flow_", &DA_Flow_);  // 19-Feb-17


	bind("srcLeaf_", &srcLeaf_);  // 24-Feb-17
	bind("destLeaf_", &destLeaf_);  // 24-Feb-17

	bind("healthyLinkCapacity", &healthyLinkCapacity);  // 20-June-17
	bind("flowsizeBytes", &flowsizeBytes); // 21-June-17


#endif /* TCP_DELAY_BIND_ALL */

	allFailedLinks = new char[100]; // 14 June 2017

}

void
TcpAgent::delay_bind_init_all()
{
        // Defaults for bound variables should be set in ns-default.tcl.
        delay_bind_init_one("window_");
        delay_bind_init_one("windowInit_");
        delay_bind_init_one("windowInitOption_");

        delay_bind_init_one("syn_");
        delay_bind_init_one("max_connects_");
        delay_bind_init_one("windowOption_");
        delay_bind_init_one("windowConstant_");
        delay_bind_init_one("windowThresh_");
        delay_bind_init_one("delay_growth_");
        delay_bind_init_one("overhead_");
        delay_bind_init_one("tcpTick_");
        delay_bind_init_one("ecn_");
	// DCTCP
	delay_bind_init_one("dctcp_"); 
	delay_bind_init_one("dctcp_alpha_");
	delay_bind_init_one("temp_alpha_");
	delay_bind_init_one("dctcp_g_");

	/* For Weighted Flowcell Spraying  SMI: 15-June-2015 */
	//delay_bind_init_one("linkID_");
	delay_bind_init_one("flowcell_");
	delay_bind_init_one("flowcellSizePkts_");
	delay_bind_init_one("roundRobin_");
	delay_bind_init_one("failureRatio_");
	//delay_bind_init_one("failedLinkIndex_");
	delay_bind_init_one("failureAware_");
	delay_bind_init_one("fromFailedLeaf_");
	delay_bind_init_one("toFailedLeaf_"); // SMI 13-Mar-16

	delay_bind_init_one("selectiveSpraying_");
	delay_bind_init_one("healthyPathOnly_"); // 14 July 2016 
	delay_bind_init_one("DA_sprayOnly_"); // 15 July 2016 

	delay_bind_init_one("poorPathFlow_");
	delay_bind_init_one("originallyHashed_");

	delay_bind_init_one("dynamicMapping_"); // 16-June
	delay_bind_init_one("dynamicMappingThreshold_"); // 16-June
	delay_bind_init_one("dynamicMappingThresholdGL2GL_"); // 13th July

	delay_bind_init_one("flowBender_"); // 13-Jan-17
	delay_bind_init_one("flowBender_T"); // 3-Jul-17
	delay_bind_init_one("flowBender_N"); // 3-Jul-17

	delay_bind_init_one("realisticFailure_"); // SMI 8-Mar
	delay_bind_init_one("failureCase_"); // SMI 8-Mar
	//delay_bind_init_one("failureDetected_"); // SMI 8-Mar

	delay_bind_init_one("northSouthFlow_"); // 4-June
	delay_bind_init_one("intraRackFlow_"); // 22-Jul
	delay_bind_init_one("failureStartTime_"); /* SMI 9-Mar-2016 */
	delay_bind_init_one("failureDuration_"); // If zero, then assume failure goes on till end ...
	delay_bind_init_one("failureDetectionDelay_");
	delay_bind_init_one("numUplinks"); // 9-Mar-16

	//delay_bind_init_one("failedLinkLeaf_"); // 25-Feb-17
	//delay_bind_init_one("multipleFailure_"); // 18-Feb-17
	delay_bind_init_one("numFailures_"); // 20-Feb
	// delay_bind_init_one("secondFailedLinkLeaf_");  // 18-Feb-17
	// delay_bind_init_one("secondFailedLinkSpine_");  // 18-Feb-17
	//delay_bind_init_one("flowFacingMultipleFailures_");  // 18-Feb-17
	delay_bind_init_one("DA_Flow_");  // 27-Mar-17

	delay_bind_init_one("srcLeaf_");  // 24-Feb-17
	delay_bind_init_one("destLeaf_");  // 24-Feb-17

	delay_bind_init_one("healthyLinkCapacity");  // 20-June-17
	delay_bind_init_one("flowsizeBytes"); // 21-June-17

        delay_bind_init_one("SetCWRonRetransmit_");
        delay_bind_init_one("old_ecn_");
        delay_bind_init_one("bugfix_ss_");
        delay_bind_init_one("eln_");
        delay_bind_init_one("eln_rxmit_thresh_");
        delay_bind_init_one("packetSize_");
        delay_bind_init_one("tcpip_base_hdr_size_");
	delay_bind_init_one("ts_option_size_");
        delay_bind_init_one("bugFix_");
	delay_bind_init_one("bugFix_ack_");
	delay_bind_init_one("bugFix_ts_");
	delay_bind_init_one("lessCareful_");
        delay_bind_init_one("slow_start_restart_");
        delay_bind_init_one("restart_bugfix_");
        delay_bind_init_one("timestamps_");
	delay_bind_init_one("ts_resetRTO_");
        delay_bind_init_one("maxburst_");
	delay_bind_init_one("aggressive_maxburst_");
        delay_bind_init_one("maxcwnd_");
	delay_bind_init_one("numdupacks_");
	delay_bind_init_one("numdupacksFrac_");
	delay_bind_init_one("exitFastRetrans_");
        delay_bind_init_one("maxrto_");
	delay_bind_init_one("minrto_");
        delay_bind_init_one("srtt_init_");
        delay_bind_init_one("rttvar_init_");
        delay_bind_init_one("rtxcur_init_");
        delay_bind_init_one("T_SRTT_BITS");
        delay_bind_init_one("T_RTTVAR_BITS");
        delay_bind_init_one("rttvar_exp_");
        delay_bind_init_one("awnd_");
        delay_bind_init_one("decrease_num_");
        delay_bind_init_one("increase_num_");
	delay_bind_init_one("k_parameter_");
	delay_bind_init_one("l_parameter_");
        delay_bind_init_one("trace_all_oneline_");
        delay_bind_init_one("nam_tracevar_");

        delay_bind_init_one("QOption_");
        delay_bind_init_one("EnblRTTCtr_");
        delay_bind_init_one("control_increase_");
	delay_bind_init_one("noFastRetrans_");
	delay_bind_init_one("precisionReduce_");
	delay_bind_init_one("oldCode_");
	delay_bind_init_one("useHeaders_");
	delay_bind_init_one("low_window_");
	delay_bind_init_one("high_window_");
	delay_bind_init_one("high_p_");
	delay_bind_init_one("high_decrease_");
	delay_bind_init_one("max_ssthresh_");
	delay_bind_init_one("cwnd_range_");
	delay_bind_init_one("timerfix_");
	delay_bind_init_one("rfc2988_");
	delay_bind_init_one("singledup_");
	delay_bind_init_one("LimTransmitFix_");
	delay_bind_init_one("rate_request_");
	delay_bind_init_one("qs_enabled_");
	delay_bind_init_one("tcp_qs_recovery_");
	delay_bind_init_one("qs_request_mode_");
	delay_bind_init_one("qs_thresh_");
	delay_bind_init_one("qs_rtt_");
	delay_bind_init_one("print_request_");

	delay_bind_init_one("frto_enabled_");
	delay_bind_init_one("sfrto_enabled_");
	delay_bind_init_one("spurious_response_");

#ifdef TCP_DELAY_BIND_ALL
	// out because delay-bound tracevars aren't yet supported
        delay_bind_init_one("t_seqno_");
        delay_bind_init_one("rtt_");
        delay_bind_init_one("srtt_");
        delay_bind_init_one("rttvar_");
        delay_bind_init_one("backoff_");
        delay_bind_init_one("dupacks_");
        delay_bind_init_one("seqno_");
        delay_bind_init_one("ack_");
        delay_bind_init_one("cwnd_");
        delay_bind_init_one("ssthresh_");
        delay_bind_init_one("maxseq_");
        delay_bind_init_one("ndatapack_");
        delay_bind_init_one("ndatabytes_");
        delay_bind_init_one("nackpack_");
        delay_bind_init_one("nrexmit_");
        delay_bind_init_one("nrexmitpack_");
        delay_bind_init_one("nrexmitbytes_");
        delay_bind_init_one("necnresponses_");
        delay_bind_init_one("ncwndcuts_");
	delay_bind_init_one("ncwndcuts1_");
	delay_bind_init_one("dupCase1Count_"); /*  SMI: 15-June-2015 */
	delay_bind_init_one("dupCase2Count_"); /*  SMI: 15-June-2015 */

#endif /* TCP_DELAY_BIND_ALL */

	Agent::delay_bind_init_all();

        reset();
}

int
TcpAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer)
{
        if (delay_bind(varName, localName, "window_", &wnd_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowInit_", &wnd_init_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowInitOption_", &wnd_init_option_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "syn_", &syn_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "max_connects_", &max_connects_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowOption_", &wnd_option_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowConstant_",  &wnd_const_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowThresh_", &wnd_th_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "delay_growth_", &delay_growth_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "overhead_", &overhead_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "tcpTick_", &tcp_tick_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "ecn_", &ecn_, tracer)) return TCL_OK;

	// SMI 14-Dec-15
        //if (delay_bind(varName, localName, "linkID_", &linkID_, tracer)) return TCL_OK; 
        if (delay_bind(varName, localName, "flowcell_", &flowcell_, tracer)) return TCL_OK; 
        if (delay_bind(varName, localName, "flowcellSizePkts_", &flowcellSizePkts_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "roundRobin_", &roundRobin_, tracer)) return TCL_OK;

	if (delay_bind(varName, localName, "failureRatio_", &failureRatio_, tracer)) return TCL_OK; // SMI 23-Dec-15
	//if (delay_bind(varName, localName, "failedLinkIndex_", &failedLinkIndex_, tracer)) return TCL_OK; // SMI 23-Dec-15
	if (delay_bind(varName, localName, "failureAware_", &failureAware_, tracer)) return TCL_OK; // SMI 23-Dec-15
	if (delay_bind(varName, localName, "fromFailedLeaf_", &fromFailedLeaf_, tracer)) return TCL_OK; // SMI 4-Jan-16
	if (delay_bind(varName, localName, "toFailedLeaf_", &toFailedLeaf_, tracer)) return TCL_OK; // SMI 13-Mar-16

	if (delay_bind(varName, localName, "selectiveSpraying_", &selectiveSpraying_, tracer)) return TCL_OK; // SMI 15-Jan-16
	if (delay_bind(varName, localName, "healthyPathOnly_", &healthyPathOnly_, tracer)) return TCL_OK; // SMI 14-July-16
	if (delay_bind(varName, localName, "DA_sprayOnly_", &DA_sprayOnly_, tracer)) return TCL_OK; // SMI 15-July-16

	if (delay_bind(varName, localName, "poorPathFlow_", &poorPathFlow_, tracer)) return TCL_OK; // SMI 15-Jan-16
	if (delay_bind(varName, localName, "originallyHashed_", &originallyHashed_, tracer)) return TCL_OK; // SMI 14-Jul-16


	if (delay_bind(varName, localName, "dynamicMapping_", &dynamicMapping_, tracer)) return TCL_OK; // SMI 16-June-16
	if (delay_bind(varName, localName, "dynamicMappingThreshold_", &dynamicMappingThreshold_, tracer)) return TCL_OK; // SMI 16-June-16
	if (delay_bind(varName, localName, "dynamicMappingThresholdGL2GL_", &dynamicMappingThresholdGL2GL_, tracer)) return TCL_OK; // SMI 13-Jul-16

	if (delay_bind(varName, localName, "flowBender_", &flowBender_, tracer)) return TCL_OK; // 13-Jan-17 
	if (delay_bind(varName, localName, "flowBender_T", &flowBender_T, tracer)) return TCL_OK; // 3-Jul-17
	if (delay_bind(varName, localName, "flowBender_N", &flowBender_N, tracer)) return TCL_OK; // 3-Jul-17

	if (delay_bind(varName, localName, "realisticFailure_", &realisticFailure_, tracer)) return TCL_OK; // SMI 8-Mar-16
	if (delay_bind(varName, localName, "failureCase_", &failureCase_, tracer)) return TCL_OK; // SMI 8-Mar-16
	//if (delay_bind(varName, localName, "failureDetected_", &failureDetected_, tracer)) return TCL_OK; // SMI 8-Mar-16

	if (delay_bind(varName, localName, "northSouthFlow_", &northSouthFlow_, tracer)) return TCL_OK;  // 4-June
	if (delay_bind(varName, localName, "intraRackFlow_", &intraRackFlow_, tracer)) return TCL_OK;  // 22-Jul

	if (delay_bind(varName, localName, "failureStartTime_", &failureStartTime_, tracer)) return TCL_OK;  /* SMI 9-Mar-2016 */
	if (delay_bind(varName, localName, "failureDuration_", &failureDuration_, tracer)) return TCL_OK; // If zero, then assume failure goes on till end ...
	if (delay_bind(varName, localName, "failureDetectionDelay_", &failureDetectionDelay_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "numUplinks", &numUplinks, tracer)) return TCL_OK; // 9-Mar-16

	//if (delay_bind(varName, localName, "failedLinkLeaf_", &failedLinkLeaf_, tracer)) return TCL_OK; // 25-Feb-17
	//if (delay_bind(varName, localName, "multipleFailure_", &multipleFailure_, tracer)) return TCL_OK; // 18-Feb-17
	if (delay_bind(varName, localName, "numFailures_", &numFailures_, tracer)) return TCL_OK; // 20-Feb-17

	// if (delay_bind(varName, localName, "secondFailedLinkLeaf_", &secondFailedLinkLeaf_, tracer)) return TCL_OK; // 18-Feb-17
	// if (delay_bind(varName, localName, "secondFailedLinkSpine_", &secondFailedLinkSpine_, tracer)) return TCL_OK; // 18-Feb-17
	//if (delay_bind(varName, localName, "flowFacingMultipleFailures_", &flowFacingMultipleFailures_, tracer)) return TCL_OK; // 19-Feb-17
	if (delay_bind(varName, localName, "DA_Flow_", &DA_Flow_, tracer)) return TCL_OK; // 27-Mar-17


	if (delay_bind(varName, localName, "srcLeaf_", &srcLeaf_, tracer)) return TCL_OK; // 24-Feb-17
	if (delay_bind(varName, localName, "destLeaf_", &destLeaf_, tracer)) return TCL_OK; // 24-Feb-17

	if (delay_bind(varName, localName, "healthyLinkCapacity", &healthyLinkCapacity, tracer)) return TCL_OK; // 20-June-17
	if (delay_bind(varName, localName, "flowsizeBytes", &flowsizeBytes, tracer)) return TCL_OK; // 21-June-17

	// Mohammad
        if (delay_bind_bool(varName, localName, "dctcp_", &dctcp_, tracer)) return TCL_OK; 
	if (delay_bind(varName, localName, "dctcp_alpha_", &dctcp_alpha_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "temp_alpha_", &temp_alpha_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "dctcp_g_", &dctcp_g_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "SetCWRonRetransmit_", &SetCWRonRetransmit_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "old_ecn_", &old_ecn_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "bugfix_ss_", &bugfix_ss_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "eln_", &eln_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "eln_rxmit_thresh_", &eln_rxmit_thresh_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "packetSize_", &size_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "tcpip_base_hdr_size_", &tcpip_base_hdr_size_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "ts_option_size_", &ts_option_size_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "bugFix_", &bug_fix_ , tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "bugFix_ack_", &bugfix_ack_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "bugFix_ts_", &bugfix_ts_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "lessCareful_", &less_careful_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "timestamps_", &ts_option_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "ts_resetRTO_", &ts_resetRTO_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "slow_start_restart_", &slow_start_restart_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "restart_bugfix_", &restart_bugfix_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxburst_", &maxburst_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "aggressive_maxburst_", &aggressive_maxburst_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxcwnd_", &maxcwnd_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "numdupacks_", &numdupacks_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "numdupacksFrac_", &numdupacksFrac_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "exitFastRetrans_", &exitFastRetrans_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxrto_", &maxrto_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "minrto_", &minrto_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "srtt_init_", &srtt_init_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rttvar_init_", &rttvar_init_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rtxcur_init_", &rtxcur_init_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "T_SRTT_BITS", &T_SRTT_BITS , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "T_RTTVAR_BITS", &T_RTTVAR_BITS , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rttvar_exp_", &rttvar_exp_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "awnd_", &awnd_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "decrease_num_", &decrease_num_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "increase_num_", &increase_num_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "k_parameter_", &k_parameter_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "l_parameter_", &l_parameter_, tracer)) return TCL_OK;


        if (delay_bind_bool(varName, localName, "trace_all_oneline_", &trace_all_oneline_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "nam_tracevar_", &nam_tracevar_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "QOption_", &QOption_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "EnblRTTCtr_", &EnblRTTCtr_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "control_increase_", &control_increase_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "noFastRetrans_", &noFastRetrans_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "precisionReduce_", &precision_reduce_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "oldCode_", &oldCode_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "useHeaders_", &useHeaders_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "low_window_", &low_window_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "high_window_", &high_window_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "high_p_", &high_p_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "high_decrease_", &high_decrease_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "max_ssthresh_", &max_ssthresh_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "cwnd_range_", &cwnd_range_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "timerfix_", &timerfix_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "rfc2988_", &rfc2988_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "singledup_", &singledup_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "LimTransmitFix_", &LimTransmitFix_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rate_request_", &rate_request_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "qs_enabled_", &qs_enabled_ , tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "tcp_qs_recovery_", &tcp_qs_recovery_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "qs_request_mode_", &qs_request_mode_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "qs_thresh_", &qs_thresh_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "qs_rtt_", &qs_rtt_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "print_request_", &print_request_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "frto_enabled_", &frto_enabled_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "sfrto_enabled_", &sfrto_enabled_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "spurious_response_", &spurious_response_, tracer)) return TCL_OK;

#ifdef TCP_DELAY_BIND_ALL
	// not if (delay-bound delay-bound tracevars aren't yet supported
        if (delay_bind(varName, localName, "t_seqno_", &t_seqno_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rtt_", &t_rtt_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "srtt_", &t_srtt_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rttvar_", &t_rttvar_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "backoff_", &t_backoff_ , tracer)) return TCL_OK;

        if (delay_bind(varName, localName, "dupacks_", &dupacks_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "seqno_", &curseq_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ack_", &highest_ack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "cwnd_", &cwnd_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ssthresh_", &ssthresh_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxseq_", &maxseq_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ndatapack_", &ndatapack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ndatabytes_", &ndatabytes_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nackpack_", &nackpack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nrexmit_", &nrexmit_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nrexmitpack_", &nrexmitpack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nrexmitbytes_", &nrexmitbytes_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "necnresponses_", &necnresponses_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ncwndcuts_", &ncwndcuts_ , tracer)) return TCL_OK;
 	if (delay_bind(varName, localName, "ncwndcuts1_", &ncwndcuts1_ , tracer)) return TCL_OK;

 	if (delay_bind(varName, localName, "dupCase1Count_", &dupCase1Count_ , tracer)) return TCL_OK; /* SMI: 15-June-2015 */
 	if (delay_bind(varName, localName, "dupCase2Count_", &dupCase2Count_ , tracer)) return TCL_OK; /* SMI: 15-June-2015 */

#endif

        return Agent::delay_bind_dispatch(varName, localName, tracer);
}

#define TCP_WRK_SIZE		512
/* Print out all the traced variables whenever any one is changed */
void
TcpAgent::traceAll() {
	if (!channel_)
		return;

	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[TCP_WRK_SIZE];

	curtime = &s ? s.clock() : 0;
	snprintf(wrk, TCP_WRK_SIZE,
		 "time: %-8.5f saddr: %-2d sport: %-2d daddr: %-2d dport:"
		 " %-2d maxseq: %-4d hiack: %-4d seqno: %-4d cwnd: %-6.3f"
		 " ssthresh: %-3d dupacks: %-2d rtt: %-6.3f srtt: %-6.3f"
		 " rttvar: %-6.3f bkoff: %-d\n", curtime, addr(), port(),
		 daddr(), dport(), int(maxseq_), int(highest_ack_),
		 int(t_seqno_), double(cwnd_), int(ssthresh_),
		 int(dupacks_), int(t_rtt_)*tcp_tick_, 
		 (int(t_srtt_) >> T_SRTT_BITS)*tcp_tick_, 
		 int(t_rttvar_)*tcp_tick_/4.0, int(t_backoff_)); 
	(void)Tcl_Write(channel_, wrk, -1);
}

/* Print out just the variable that is modified */
void
TcpAgent::traceVar(TracedVar* v) 
{
	if (!channel_)
		return;

	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[TCP_WRK_SIZE];

	curtime = &s ? s.clock() : 0;

	// XXX comparing addresses is faster than comparing names
	if (v == &cwnd_)
		snprintf(wrk, TCP_WRK_SIZE,
			 "%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f\n",
			 curtime, addr(), port(), daddr(), dport(),
			 v->name(), double(*((TracedDouble*) v))); 
 	else if (v == &t_rtt_)
		snprintf(wrk, TCP_WRK_SIZE,
			 "%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f\n",
			 curtime, addr(), port(), daddr(), dport(),
			 v->name(), int(*((TracedInt*) v))*tcp_tick_); 
	else if (v == &t_srtt_)
		snprintf(wrk, TCP_WRK_SIZE,
			 "%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f\n",
			 curtime, addr(), port(), daddr(), dport(),
			 v->name(), 
			 (int(*((TracedInt*) v)) >> T_SRTT_BITS)*tcp_tick_); 
	else if (v == &t_rttvar_)
		snprintf(wrk, TCP_WRK_SIZE,
			 "%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f\n",
			 curtime, addr(), port(), daddr(), dport(),
			 v->name(), 
			 int(*((TracedInt*) v))*tcp_tick_/4.0); 
	else
		snprintf(wrk, TCP_WRK_SIZE,
			 "%-8.5f %-2d %-2d %-2d %-2d %s %d\n",
			 curtime, addr(), port(), daddr(), dport(),
			 v->name(), int(*((TracedInt*) v))); 

	(void)Tcl_Write(channel_, wrk, -1);
}

void
TcpAgent::trace(TracedVar* v) 
{
	if (nam_tracevar_) {
		Agent::trace(v);
	} else if (trace_all_oneline_)
		traceAll();
	else 
		traceVar(v);
}

//
// in 1-way TCP, syn_ indicates we are modeling
// a SYN exchange at the beginning.  If this is true
// and we are delaying growth, then use an initial
// window of one.  If not, we do whatever initial_window()
// says to do.
//

void
TcpAgent::set_initial_window()
{
	if (syn_ && delay_growth_) {
		cwnd_ = 1.0; 
		syn_connects_ = 0;
	} else
		cwnd_ = initial_window();
}

void
TcpAgent::reset_qoption()
{
	int now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);

	T_start = now ; 
	RTT_count = 0 ; 
	RTT_prev = 0 ; 
	RTT_goodcount = 1 ; 
	F_counting = 0 ; 
	W_timed = -1 ; 
	F_full = 0 ;
	Backoffs = 0 ; 
}

void
TcpAgent::reset()
{
	rtt_init();
	rtt_seq_ = -1;
	/*XXX lookup variables */
	dupacks_ = 0;
	curseq_ = 0;
	set_initial_window();

	t_seqno_ = 0;
	maxseq_ = -1;
	last_ack_ = -1;
	highest_ack_ = -1;
	//highest_ack_ = 1;
	ssthresh_ = int(wnd_);
	if (max_ssthresh_ > 0 && max_ssthresh_ < ssthresh_) 
		ssthresh_ = max_ssthresh_;
	wnd_restart_ = 1.;
	awnd_ = wnd_init_ / 2.0;
	recover_ = 0;
	closed_ = 0;
	last_cwnd_action_ = 0;
	boot_time_ = Random::uniform(tcp_tick_);
	first_decrease_ = 1;
	/* W.N.: for removing packets from previous incarnations */
	lastreset_ = Scheduler::instance().clock();

	/* Now these variables will be reset 
	   - Debojyoti Dutta 12th Oct'2000 */
 
	ndatapack_ = 0;
	ndatabytes_ = 0;
	nackpack_ = 0;
	nrexmitbytes_ = 0;
	nrexmit_ = 0;
	nrexmitpack_ = 0;
	necnresponses_ = 0;
	ncwndcuts_ = 0;
	ncwndcuts1_ = 0;

	dupCase1Count_ = 0; /*  SMI: 15-June-2015 */
	dupCase2Count_ = 0; /*  SMI: 15-June-2015 */

        cancel_timers();      // suggested by P. Anelli.

	if (control_increase_) {
		prev_highest_ack_ = highest_ack_ ; 
	}

	if (wnd_option_ == 8) {
		// HighSpeed TCP
		hstcp_.low_p = 1.5/(low_window_*low_window_);
		double highLowWin = log(high_window_)-log(low_window_);
		double highLowP = log(high_p_) - log(hstcp_.low_p);
		hstcp_.dec1 = 
		   0.5 - log(low_window_) * (high_decrease_ - 0.5)/highLowWin;
		hstcp_.dec2 = (high_decrease_ - 0.5)/highLowWin;
        	hstcp_.p1 = 
		  log(hstcp_.low_p) - log(low_window_) * highLowP/highLowWin;
		hstcp_.p2 = highLowP/highLowWin;
	}

	if (QOption_) {
		int now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);
		T_last = now ; 
		T_prev = now ; 
		W_used = 0 ;
		if (EnblRTTCtr_) {
			reset_qoption();
		}
	}
}

/*
 * Initialize variables for the retransmit timer.
 */
void TcpAgent::rtt_init()
{
	t_rtt_ = 0;
	t_srtt_ = int(srtt_init_ / tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_ / tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;
	t_backoff_ = 1;
}

double TcpAgent::rtt_timeout()
{
	double timeout;
	if (rfc2988_) {
	// Correction from Tom Kelly to be RFC2988-compliant, by
	// clamping minrto_ before applying t_backoff_.
		if (t_rtxcur_ < minrto_ && !use_rtt_)
			timeout = minrto_ * t_backoff_;
		else
			timeout = t_rtxcur_ * t_backoff_;
	} else {
		// only of interest for backwards compatibility
		timeout = t_rtxcur_ * t_backoff_;
		if (timeout < minrto_)
			timeout = minrto_;
	}

	if (timeout > maxrto_)
		timeout = maxrto_;

        if (timeout < 2.0 * tcp_tick_) {
		if (timeout < 0) {
			fprintf(stderr, "TcpAgent: negative RTO!  (%f)\n",
				timeout);
			exit(1);
		} else if (use_rtt_ && timeout < tcp_tick_)
			timeout = tcp_tick_;
		else
			timeout = 2.0 * tcp_tick_;
	}
	use_rtt_ = 0;
	return (timeout);
}


/* This has been modified to use the tahoe code. */
void TcpAgent::rtt_update(double tao)
{
	double now = Scheduler::instance().clock();
	if (ts_option_)
		t_rtt_ = int(tao /tcp_tick_ + 0.5);
	else {
		double sendtime = now - tao;
		sendtime += boot_time_;
		double tickoff = fmod(sendtime, tcp_tick_);
		t_rtt_ = int((tao + tickoff) / tcp_tick_);
	}
	if (t_rtt_ < 1)
		t_rtt_ = 1;
	//
	// t_srtt_ has 3 bits to the right of the binary point
	// t_rttvar_ has 2
        // Thus "t_srtt_ >> T_SRTT_BITS" is the actual srtt, 
  	//   and "t_srtt_" is 8*srtt.
	// Similarly, "t_rttvar_ >> T_RTTVAR_BITS" is the actual rttvar,
	//   and "t_rttvar_" is 4*rttvar.
	//
        if (t_srtt_ != 0) {
		register short delta;
		delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);	// d = (m - a0)
		if ((t_srtt_ += delta) <= 0)	// a1 = 7/8 a0 + 1/8 m
			t_srtt_ = 1;
		if (delta < 0)
			delta = -delta;
		delta -= (t_rttvar_ >> T_RTTVAR_BITS);
		if ((t_rttvar_ += delta) <= 0)	// var1 = 3/4 var0 + 1/4 |d|
			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;		// srtt = rtt
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	// rttvar = rtt / 2
	}
	//
	// Current retransmit value is 
	//    (unscaled) smoothed round trip estimate
	//    plus 2^rttvar_exp_ times (unscaled) rttvar. 
	//
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) +
		t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;

	return;
}

void TcpAgent::rtt_backoff()
{
	if (t_backoff_ < 64 || (rfc2988_ && rtt_timeout() < maxrto_))	
	//this was a patch from Michele Weigle.... applied by SMI on 7-apr-2015 
	//if (t_backoff_ < 64 || rfc2988_)  
        	t_backoff_ <<= 1;
        // RFC2988 allows a maximum for the backed-off RTO of 60 seconds.
        // This is applied by maxrto_.

	if (t_backoff_ > 8) {
		/*
		 * If backed off this far, clobber the srtt
		 * value, storing it in the mean deviation
		 * instead.
		 */
		t_rttvar_ += (t_srtt_ >> T_SRTT_BITS);
		t_srtt_ = 0;
	}
}

/*
 * headersize:
 *      how big is an IP+TCP header in bytes; include options such as ts
 * this function should be virtual so others (e.g. SACK) can override
 */
int TcpAgent::headersize()
{
        int total = tcpip_base_hdr_size_;
	if (total < 1) {
		fprintf(stderr,
		  "TcpAgent(%s): warning: tcpip hdr size is only %d bytes\n",
		  name(), tcpip_base_hdr_size_);
	}
	if (ts_option_)
		total += ts_option_size_;
        return (total);
}

void TcpAgent::output(int seqno, int reason)
{
	int force_set_rtx_timer = 0;
	Packet* p = allocpkt();
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_flags* hf = hdr_flags::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	int databytes = hdr_cmn::access(p)->size();
	tcph->seqno() = seqno;

	// printf("DEBUG: tcp:output() flowcell_=%d flowcellSizePkts_=%d ndatapack_=%d \n", flowcell_, flowcellSizePkts_, (int) ndatapack_);
	// TODO: We still do not cater for the case of how to deal with flowcells for when weight for failed link is not 1

	failureDetected_ = 0;
	double time_now = (double) Scheduler::instance().clock();

	if(realisticFailure_ && (failureCase_!=0)) {
		if(time_now > (failureStartTime_ + failureDetectionDelay_)) {
			if(failureDuration_==0.0 || ((failureDuration_ > 0.0) && (time_now < (failureStartTime_ + failureDuration_ + failureDetectionDelay_)))) {
				failureDetected_ = 1;
				hf->failureDetected_ = 1;
			}
		}
	}

	if(flowcell_) {
		if(ndatapack_ == 0) { // case of first packet in the flow, so do some initializations

			if(debug_tcp_smi) {
				printf("DEBUG-0: 1st PKT: failureRatio_=%d, failureAware_=%d numUplinks=%d ", failureRatio_, failureAware_, numUplinks);
				printf(" numFailures_=%d \n", numFailures_);
			}
			uplinkWeights = new int[numUplinks];
			linkID_ = numUplinks - 1;
			rfactor = 0;

			if(numFailures_ > 0) {

				failedLinks = new failedLink[numFailures_];

				// populate the list for no failure SVT
				svt_NoFailure = new int[numUplinks];
				svt_partialFailure = new int[numUplinks];
				fullFailureSpines = new int[numUplinks];

				svt_DA_partialFailure = new int[numUplinks]; // spines involved in an indirect partial failure
				svt_DA_fullFailure = new int[numUplinks]; // spines involved in an indirect full failure
				svt_DA_NoFailure = new int[numUplinks]; // spines that face no direct failure and not related to any indirect failure either

				for(int j=0; j < numUplinks; j++) {
					svt_NoFailure[j] = 1;
					svt_DA_NoFailure[j] = 1;

					svt_partialFailure[j] = -1;
					fullFailureSpines[j] = -1;

					svt_DA_partialFailure[j] = -1;
					svt_DA_fullFailure[j] = -1;
				}

				char* all_f_links = new char[100];
				strcpy(all_f_links, allFailedLinks);
				char* token = strtok(all_f_links, ",;");

				int failed_link_fields[numFailures_ * 3];
				int flink = 0;

				while(token != NULL) {
					failed_link_fields[flink] = atoi(token);
					token = strtok(NULL, ",;");
					flink++;
				}

				for(flink = 0; flink < numFailures_; flink++) {
					failedLinks[flink].leaf = failed_link_fields[(flink*3)];
					failedLinks[flink].spine = failed_link_fields[(flink*3)+1];
					failedLinks[flink].failureRatio = failed_link_fields[(flink*3)+2];
				}

				// DEBUG
				if(debug_tcp_smi) {
					for(int k = 0; k < numFailures_; k++) {
						printf("failed link %d: leaf=%d spine=%d f_ratio=%d \n", k, failedLinks[k].leaf, failedLinks[k].spine, failedLinks[k].failureRatio);
					}
				}
			}

			// for future, we need to make number of failures completely flexible, maybe 3 or 4 or more
			// 20-Feb-17
			if(numFailures_ > 1) { // removed AND with multipleFailure_

				// comment out for later ....	
				//////////////////////////////////////////
				// failedLinkLeafs = new int[numFailures_];
				// failedLinkSpines = new int[numFailures_];

				// failedLinkSpines[0] = failedLinkIndex_;
				// failedLinkSpines[1] = secondFailedLinkSpine_;
				// failedLinkLeafs[0] = failedLinkLeaf_;
				// failedLinkLeafs[1] = secondFailedLinkLeaf_;
				//////////////////////////////////////////

				//printf("Calculating num of failures faced by (srcLeaf_=%d, destLeaf_=%d) pair \n", srcLeaf_, destLeaf_);

				// calculate the number of direct failures faced by this srcLeaf+destLeaf pair
				for(int j=0; j<numFailures_; j++) {
					if(failedLinks[j].leaf==srcLeaf_ || failedLinks[j].leaf==destLeaf_) {
						// check for duplicates, then populate the relevant SVT, and then add to the relevant counter
						if(svt_NoFailure[failedLinks[j].spine]==1) {
							svt_NoFailure[failedLinks[j].spine] = -1;
							svt_DA_NoFailure[failedLinks[j].spine] = -1;
							numDirectFailures_++;
							if(failedLinks[j].failureRatio==100) {
								fullFailureSpines[failedLinks[j].spine] = 1;
								numDirectFullFailurePaths++;
							} else {
								svt_partialFailure[failedLinks[j].spine] = 1;
								numDirectPartialFailurePaths++;
							}
							// If this relevant spine was considered among the DA_spines, we should now remove it from there....
							if(svt_DA_partialFailure[failedLinks[j].spine]==1) {
								svt_DA_partialFailure[failedLinks[j].spine] = -1;
								numIndirectPartialFailurePaths--;
							} else if(svt_DA_fullFailure[failedLinks[j].spine]==1) {
								svt_DA_fullFailure[failedLinks[j].spine] = -1;
								numIndirectFullFailurePaths--;
							}
						} else {
							// this may be the case where 2 failures share the same spine
							if(failedLinks[j].failureRatio==100) {
								if(fullFailureSpines[failedLinks[j].spine]!=1) {
									fullFailureSpines[failedLinks[j].spine] = 1;
									numDirectFullFailurePaths++;
									if(svt_partialFailure[failedLinks[j].spine]==1) {
										svt_partialFailure[failedLinks[j].spine] = -1;
										numDirectPartialFailurePaths--;
									}
								}
							} // nothing to be done if new failure is partial, since it does not change anything if it shares the spine with another failure
						}						
						/* With svt_NoFailure, an entry=-1 means this spine is not in this SVT
						   With svt_partialFailure, an entry=1 means this spine is in the SVT
						   With fullFailureSpines, an entry=1 means this spine is fully failed
						*/
					} else {
						// an indirect failure (maybe full or partial)

						if(svt_NoFailure[failedLinks[j].spine] == -1) {
							// if this spine is already removed from svt_NoFailure, this means it is involved in a direct failure, therefore it will not
							// be considered as a DA_spine

						} else {
							if(svt_DA_NoFailure[failedLinks[j].spine] == -1) {
								// this spine is already known as a DA Spine
								// TODO :: if this failure is a DA Indirect Full Failure, do we need to check if the original decision making this 
								// a DA Spine was due to a full or partial failure??

								// if(svt_DA_partialFailure[failedLinks[j].spine]==1) {
								// 	svt_DA_partialFailure[failedLinks[j].spine] = -1;
								// 	numIndirectPartialFailurePaths--;
								// } else if(svt_DA_fullFailure[failedLinks[j].spine]==1) {
								// 	svt_DA_fullFailure[failedLinks[j].spine] = -1;
								// 	numIndirectFullFailurePaths--;
								// }
							} else {
								// otherwise, we can look to see whether we need to add this to the svt_DA_partialFailure or the svt_DA_fullFailure
								if(failedLinks[j].failureRatio==100) {									
									svt_DA_fullFailure[failedLinks[j].spine] = 1; // add to svt_DA_fullFailure
									numIndirectFullFailurePaths++;									
								} else {
									svt_DA_partialFailure[failedLinks[j].spine] = 1; // add to svt_DA_partialFailure
									numIndirectPartialFailurePaths++;									
								}
								svt_DA_NoFailure[failedLinks[j].spine] = -1; // remove from svt_DA_NoFailure
							}
						}
					}
				}

				/* TODO: In the code above and below, where we prepare the different SVTs and we calculate probs and assign the flow to a particular SVT,
				   have we taken care of the issue of (1) flow size and (2) dynamic mapping? Do we need to worry about the matter of toFailedLeaf and fromFailedLeaf
				   since we have not considered it here???
				 */ 

				/** This is where we should calculate probabilities to decide whether this flow should map to: */

				int partialFailurePaths_BW = numDirectPartialFailurePaths*(healthyLinkCapacity/failureRatio_);
				int healthyPaths_BW = (numUplinks-numDirectPartialFailurePaths-numDirectFullFailurePaths)*healthyLinkCapacity;
				int total_BW = partialFailurePaths_BW + healthyPaths_BW;
				int indirectFailurePaths_BW = (numIndirectFullFailurePaths + numIndirectPartialFailurePaths) * healthyLinkCapacity;

				// TEMP: temporarily commented this for debugging purposes -- 21-June-2017
				if(toFailedLeaf_) {
					total_BW = total_BW * (numUplinks-1); // extra cautious for to-failed-leaf case, as before
				}

				int randNum_totalBW = Random::integer(total_BW);
				if(debug_tcp_smi) {
					printf("\n srcLeaf_=%d destLeaf_=%d flowsizeBytes=%d healthyLinkCapacity=%d " , srcLeaf_, destLeaf_, flowsizeBytes, healthyLinkCapacity);
					printf("dynamicMapping_=%d healthyPathOnly_=%d DA_sprayOnly_=%d \n", dynamicMapping_, healthyPathOnly_, DA_sprayOnly_);
					printf("numIndirectFullFailurePaths=%d, numIndirectPartialFailurePaths=%d ", numIndirectFullFailurePaths, numIndirectPartialFailurePaths);
					printf("numDirectPartialFailurePaths=%d, numDirectFullFailurePaths=%d ", numDirectPartialFailurePaths, numDirectFullFailurePaths);
					printf("total_BW=%d healthyPaths_BW=%d partialFailurePaths_BW=%d \n", total_BW, healthyPaths_BW, partialFailurePaths_BW);
					printf("indirectFailurePaths_BW=%d dynamicMappingThreshold_=%d dynamicMappingThresholdGL2GL_=%d \n", indirectFailurePaths_BW, dynamicMappingThreshold_, dynamicMappingThresholdGL2GL_);
					printf("randNum_totalBW=%d \n\n", randNum_totalBW);
				}

				// handle fromFailedLeaf and toFailedLeaf -- this should only affect the probabilities for svt_partialFailure flows

				// Remapping perhaps if DA-causing traffic exists or ceases to exist TODO
				Tcl& tcl = Tcl::instance();
				char buff[64];
				sprintf (buff, "[Simulator instance] get-incoming-traffic %d %d", srcLeaf_, destLeaf_); 
				tcl.evalc(buff);
				const char* incTraffic = tcl.result();
				if(debug_tcp_smi) {
					printf("DEBUG: incomingTrafficToDestLeaf=%s for srcleaf=%d, destleaf=%d \n", incTraffic, srcLeaf_, destLeaf_);
				}
				int incomingTrafficToDestLeaf = atoi(incTraffic);

				// if(incomingTrafficToDestLeaf>0) { // choose from 10, 11 or 12
				// } else { // choose from 0 or 1 }
				
				//if(((flowsizeBytes < dynamicMappingThreshold_) || dynamicMapping_==1) && healthyPathOnly_==0 && DA_sprayOnly_==0 && (randNum_totalBW < partialFailurePaths_BW)) {
				if(((flowsizeBytes < dynamicMappingThreshold_) || dynamicMapping_==1) && healthyPathOnly_==0 && (randNum_totalBW < partialFailurePaths_BW)) {
					svtMapping = 1; // svt_partialFailure chosen
				} else {
					if((healthyPathOnly_==0 && DA_sprayOnly_==0) && ((numIndirectFullFailurePaths+numIndirectPartialFailurePaths) > 0) && incomingTrafficToDestLeaf ) { 
						int randNum_healthyBW = Random::integer(healthyPaths_BW);
						//printf("randNum_healthyBW=%d \n ", randNum_healthyBW);
						if(((flowsizeBytes < dynamicMappingThresholdGL2GL_) || dynamicMapping_==1) && (randNum_healthyBW < indirectFailurePaths_BW)) {
							// select from one of the indirect failure SVTs
							//printf("inside here \n");
							int randNum_DA_indirectFailureBW = Random::integer(indirectFailurePaths_BW);
							if( randNum_DA_indirectFailureBW < (numIndirectPartialFailurePaths * healthyLinkCapacity)) {
								svtMapping = 11; // svt_DA_partialFailure
							} else {
								svtMapping = 12; //  svt_DA_fullFailure
							}
						} else {
							svtMapping = 10; // svt_DA_NoFailure
						}
					} else { // case of SAPS-DAU or HPO, or just zero indirect failures
						svtMapping = 0; // svt_NoFailure 
					}
				}
			}

			printf("DEBUG: tcp:output() ndatapack_=%d svtMapping=%d \n", (int) ndatapack_, svtMapping);

			if(debug_tcp_smi) {
				printf("DEBUG-0: 1st PKT of FLOW: numDirectFailures_=%d \n", numDirectFailures_);
			}

	/* CHALLENGE: 28-March

	  Another case: perhaps already managed, wherein, we have a flow that is both FFL and 2FL, and this means either
	  (a) both failures share the same spine, thus there are no DA flows with this scenario, and 1 specific path contains both failures on its way
	  (b) don't share the same spine, so 1 particular leaf-leaf combo faces 2 direct failures, and that combo has no DA flows

	  Also, have to check for correctness of WFCS code, it is not working at the moment - 28-March-17
	 */

	/*	
	  For weighted schemes, if multipleFailure_ is true, make sure we do the relevant weighting for both spines (and of course, it may be only 1 spine counted twice) --- check if the weights
	  are being correctly used, also see if our weights calculation is done rightly as well

	  for SPPS, we have to make sure if multipleFailure_ is true, that (1) we do not spray healthy spine flows onto failure affected spines; (2) spray poor link flows across both affected spines (check
	  to see if indeed it is two separate spines or not); (3) check if we are managing goodleaf-to-goodleaf flows correctly; for them we also need to do the same, i.e. points (1) and (2)

	  for HPO, we must also not use the multiple affected spines....

	 */
			/*
			  Also, enable all left flows to send to the rightmost rack, and of course replicate changes made to all-to-all part of traffic generation to the left-to-right piece of code, making sure
			  that new expected variables for build-short-lived are also passed properly.
			*/
			if(failureAware_ && (fromFailedLeaf_ || toFailedLeaf_)) { // Weighted LB Scheme Initialization
				if(roundRobin_) {
					if(selectiveSpraying_) { // HPO or SPS or SPPS
						/* 29-March-17: This section is where there is a possibility of DA flows, where numDirectFailures_ is 1. When it is 2, no DA flows */
						if(poorPathFlow_) {
							if(numFailures_ > 1) { // commented if(multipleFailure_) on 9-Jul-17 SMI
								// for(int i=0; i < numFailures_; i++) {
								// 	if(failedLinkLeafs[i]==srcLeaf_ || failedLinkLeafs[i]==destLeaf_) {
								// 		linkID_ = failedLinkSpines[i]; 
								// 		break;
								// 	}
								// }
							}
							else {
								linkID_ = failedLinks[0].spine; //linkID_ = failedLinkIndex_; Commented on 9-Jul-17
							}
							originallyHashed_ = 1; // 14 July 2016
						} else if(DA_Flow_ && numFailures_ > 0) { /* DA spine flow, we have to set it to that spine which is an indirect failure for this src-leaf/dest-leaf combo */
							// for(int failure=0; failure<numFailures_; failure++) {
							// 	if(failedLinkLeafs[failure]!=srcLeaf_ && failedLinkLeafs[failure]!=destLeaf_) {
							// 		linkID_ = failedLinkSpines[failure];
							// 		break;
							// 	}
							// }
						} else {
							/* if multiple failures, find first spine that is not involved in any of the failures (previously it was any failure involving this src-leaf/dest-leaf pair */
							if(numFailures_ > 1) {
								// for(int i=0; i < numUplinks; i++) {
								// 	int spineIsFine=1;
								// 	for(int j=0; j<numFailures_; j++) {
								// 		if(i==failedLinkSpines[j]) {
								// 			// TODO: Add this clause (failedLinkLeafs[j]==srcLeaf_ || failedLinkLeafs[j]==destLeaf_) when we know the scheme is SPPS-DASO
								// 			spineIsFine=0;
								// 			break;
								// 		}
								// 	}
								// 	if(spineIsFine) {
								// 		linkID_ = i;
								// 		break;
								// 	}
								// }
							}
							else { // do we need to initialize anything here??? maybe play with rfactor value if want to use it for something else ?? 
								// linkID_ = (failedLinkIndex_ + 1) % numUplinks; // SMI JUl 29
								linkID_ = (failedLinks[0].spine + 1) % numUplinks;
							}
						}
					} else { // WPS, WFCS && ??						
						if(numFailures_ > 1) { // if(multipleFailure_) Commented on 9-Jul-17
							rfactor = (numUplinks - numDirectFailures_) * failureRatio_; /* here we should minus the numOfFailures with this srcLeaf_ & destLeaf_  */
						} else {
							rfactor = (numUplinks - 1) * failureRatio_;
						}

						if(debug_tcp_smi) {
							printf("DEBUG-0.1: 1st PKT of FLOW: WFCS rfactor=%d \n", rfactor);
						}

					}
				} else { // WPS-P, WFCS-P ??
					for(int i = 0; i < numUplinks; i++) {
						//if(numFailures_ > 0) {
						int uplinkFailed=0;
						int uplinkFailureRatio=1;
						for(int j=0; j<numFailures_;j++) {
							if(i==failedLinks[j].spine && (failedLinks[j].leaf==srcLeaf_ || failedLinks[j].leaf==destLeaf_)) {
								//if(i==failedLinkSpines[j] && (failedLinkLeafs[j]==srcLeaf_ || failedLinkLeafs[j]==destLeaf_)) {
								uplinkFailureRatio=failedLinks[j].failureRatio;
								uplinkFailed=1; // make sure this link is actually failed...
								break;
							}
						}
						if(uplinkFailureRatio==100) { // 100 indicates full failure // 9-Jul-17
							uplinkWeights[i] = 0;
						} else {
							uplinkWeights[i] = 10/uplinkFailureRatio;
						}
						// } else {
						// 	if(i==failedLinkIndex_) {
						// 		uplinkWeights[i] = 10/failureRatio_;
						// 	} else {
						// 		uplinkWeights[i] = 10; //rfactor += 10;
						// 	} 
						//}
						totalUplinkWeights += uplinkWeights[i];
					}
				}
			} else { // either not failure aware of not either from-failed-leaf or to-failed-leaf
				if(failureAware_) {
					if(roundRobin_==0) { // WPS-P, WFCS-P : Good Leaf to Good Leaf flow
						for(int i = 0; i < numUplinks; i++) {
							uplinkWeights[i] = 10;
							totalUplinkWeights += uplinkWeights[i];
						}					
					} else { /* 29-March-17: This section is where the numDirectFailures_ is 0 (thus there are 2 DA spines) */

						if(selectiveSpraying_) { 
							if(poorPathFlow_) {  /* this indicates that this flow is directed to the spine affected by delay asymmetry */
								if(debug_tcp_smi) {
									printf("In TCP.cc : GL to GL hashed flow.... COOL!!!\n");
								}
								if(numFailures_ > 1 ) { // if(multipleFailure_) Commented 9-Jul-17
									// for(int i=0; i < numFailures_ ; i++) {
									// 	if(failedLinkLeafs[i]!=srcLeaf_ && failedLinkLeafs[i]!=destLeaf_) {
									// 		linkID_ = failedLinkSpines[i];
									// 		break;
									// 	}
									// }
								} else {
									//linkID_ = failedLinkIndex_; // case of SPS, some flows are hashed onto the "less-used-path"
									linkID_ = failedLinks[0].spine; 
								}
								originallyHashed_ = 1; // 14 July 2016
								if(debug_tcp_smi) {
									printf("This is a Hashed Flow with linkID_=%d !!\n", linkID_);
								}
							} else { // good-leaf-2-good-leaf and not going to delay asymmetry affected spine....
								if(numFailures_ > 1) { // if(multipleFailure_) Commented 9-Jul-17
									// // We need that spine that is (1) not affected by any delay asymmetry
									// for(int i=0; i < numUplinks; i++) {
									// 	int spineIsFine=1;
									// 	for(int j=0; j<numFailures_; j++) {
									// 		if(i==failedLinkSpines[j]) {
									// 			spineIsFine=0;
									// 			break;
									// 		}
									// 	}
									// 	if(spineIsFine) {
									// 		linkID_ = i;
									// 		break;
									// 	}
									// }
								} else {									
									// linkID_ = (failedLinkIndex_ + 1) % numUplinks; // SMI JUl 29
									linkID_ = (failedLinks[0].spine + 1) % numUplinks;
								}
							}
						} else { // 17-April -- for WFCS ?? no direct failures on this (src leaf - dest leaf) combination
							failureRatio_ = 1; // we do not incorporate any failure in our algo in this case
							rfactor = numUplinks - 1; // the turn of the failed link comes after rfactor flowcells...
						}
					}
				} else { // failure_unaware
					failureRatio_ = 1; // we do not incorporate any failure in our algo in this case
					rfactor = numUplinks - 1; // the turn of the failed link comes after rfactor flowcells...					
				}
			}
			flowCellMod_ = flowcellSizePkts_ * rfactor;
			if(debug_tcp_smi) {
				printf("DEBUG-1 FailureStartTime=%f; FailureDuration=%f; FailureDetectionTime=%f \n ", failureStartTime_, failureDuration_, failureDetectionDelay_);
				printf("DEBUG-2: numUplinks=%d; totalUplinkWeights=%d; flowCellMod_ = %d ; rfactor=%d ", numUplinks, totalUplinkWeights, flowCellMod_, rfactor);
				printf(" selectiveSpraying_=%d; poorPathFlow_=%d \n", selectiveSpraying_, poorPathFlow_);
				printf("DEBUG-3: realisticFailure_=%d failureCase_=%d  failureDetected_=%d \n", realisticFailure_, failureCase_,  failureDetected_);
				printf("DEBUG-4: NorthSouth=%d; now=%f \n", northSouthFlow_, time_now);

				// svtMapping = 1; // svt_partialFailure chosen
				// svtMapping = 0; // svt_NoFailure
				// svtMapping = 12; //  svt_DA_fullFailure
				// svtMapping = 11; // svt_DA_partialFailure
				// svtMapping = 10; // svt_DA_NoFailure

				if(selectiveSpraying_) {
					//int fID_ = int(fid_);
					//if(fID_==1)
					printf("SVT 0='no-failure; 1='partial-failure'; 10='no-DA-no-failure'; 11='DA-partial-failure'; 12='DA-full-failure'; \n");
					printf("SVT Chosen=%d! Src-Leaf=%d Dest-Leaf=%d \n", svtMapping, srcLeaf_, destLeaf_);  
					printf("SVT Partial Failure:\t");  
					for(int j=0; j < numUplinks; j++) {
						printf("%d \t", svt_partialFailure[j]);
					}
					printf("\nSVT No Failure:\t");  
					for(int j=0; j < numUplinks; j++) {
						printf("%d \t", svt_NoFailure[j]);
					}
					printf("\nSVT DA Full Failure:\t");  
					for(int j=0; j < numUplinks; j++) {
						printf("%d \t", svt_DA_fullFailure[j]);
					}
					printf("\nSVT DA Partial Failure:\t");  
					for(int j=0; j < numUplinks; j++) {
						printf("%d \t", svt_DA_partialFailure[j]);
					}
					printf("\nSVT No-DA No-Failure:\t");  
					for(int j=0; j < numUplinks; j++) {
						printf("%d \t", svt_DA_NoFailure[j]);
					}
					printf("\n");

					// for(int j=0; j < numUplinks; j++) {
					// 	fullFailureSpines[j] = -1;
					// }
				}
			}
		} // end of IF for the first packet...

		/*
		  Important things left for multiple failure scenarios (30-March-2017):
		  (1) Fix WFCS at 3 places: size_aware.tcl initialization, above these lines at 1st packet, and below these lines as well.
		  --> Have to ask why we allow all this code below to run again for the 1st packet? Do we really need it to be this way???
		  --> check to see if the large flows that are now allowed to be DA flows, etc... are also being catered for (when they are either FFL or 2FL)
		 */		


		/**

		   Challenge for 21-June-2017::
 
		   1) Comment out code for multipleFailure_==1 for the selectiveSpraying_ cases
		   2) Replace with a standalone block that simply does the following:
		   - reads the relevant svtMapping value and does the necessary when it the packet constitutes a new flowcell
		   - if dynamicMapping_ is true, we have to check ndatabytes_ for when it crosses either the dynamicMappingThresholdGL2GL_ (svt_DA_fullFailure or svt_DA_partialFailure) 
		   or dynamicMappingThreshold_ (if svt_partialFailure)
		   3) This block should no longer consider the redundant variables such as failureCase_ and DA_Flow_ (perhaps we should set relevant values for DA_Flow_ and poorPathFlow_
		   given the value we have for svtMapping
		   4) Ponder over the initial value of linkID_, will it have any undesirable impact on the performance of SAPS?
		   5) Handle for failureDetected_ properly

		   Review code from lines 1273 to 1421

		 **/

		if(failureAware_ == 0) { // Unweighted Flowcell Spraying (UFS) or Unweighted Packet Spraying (UPS)
			if (ndatapack_ % flowcellSizePkts_ == 0){
				linkID_ = (linkID_ + 1) % numUplinks; //linkID_++; //if (linkID_ > failedLink_) linkID_ = 0;
			}
		} else { // Failure Aware (WFCS or WPS, or SPPS/SPS/HPO/SPPS-DASO)
			if(realisticFailure_) {
				if(roundRobin_) {
					if(failureDetected_) {
						if(selectiveSpraying_ && (numFailures_ > 1) && (ndatapack_ % flowcellSizePkts_ == 0)) {
							//if(selectiveSpraying_ && multipleFailure_ && (ndatapack_ % flowcellSizePkts_ == 0)) {
							// Have to cater to SAPS, SAPS-DAU (DA_sprayOnly_==1), and HPO (healthyPathOnly_==1)

							// TODO: Remapping if we have new incoming DA-causing traffic, or if it ceases to exist

							// if a flow is mapped to a direct or indirect(DA) failure link, if it has crossed the relevant threshold, remap it
							if(dynamicMapping_ && (svtMapping==1 || svtMapping==11 || svtMapping==12)) {
								if(svtMapping==1 && (ndatabytes_ > dynamicMappingThreshold_)) {
									if(DA_sprayOnly_==1) { // SAPS-DAU, remap from 1 to 0
										svtMapping = 0;
									} else { // SAPS, remap from 1 to 10
										svtMapping = 10;
									}									
								} else {
									if((svtMapping==11 || svtMapping==12) && (ndatabytes_ > dynamicMappingThresholdGL2GL_)) {
										svtMapping = 10; // this can only be SAPS, not SAPS-DAU
									}
								}

							}							
							nextValidLink(); // toggle to the next valid link in the respective svt

						} else {

							if(failureCase_==1) { // partial failure
								if(selectiveSpraying_) { // HPO, SPS or SPPS
									if(DA_Flow_ && (numFailures_ > 1)) { /* The case of a DA spine flow which is also FFL or 2FL, or perhaps not from either */
									//if(DA_Flow_ && multipleFailure_) {
										// // I don't think we need to handle DASO here, since DASO will not have flows assigned to DA spine
										// if(dynamicMapping_ && (ndatabytes_ > dynamicMappingThresholdGL2GL_)) {
										// 	DA_Flow_ = 0; // this is no longer a DA flow, now make it a healthy links flow
										// 	linkID_ = (linkID_ + 1) % numUplinks; // go to the next port

										// 	/* For later: We may need to incorporate more variables (threshold-type) to decide whether DA spines are actually DA at all? */
										// 	for(int i=linkID_; i < (linkID_ + numUplinks); i++) {
										// 		int spineIsFine=1;
										// 		int port = i % numUplinks;
										// 		for(int j=0; j<numFailures_; j++) {
										// 			//if((failedLinkLeafs[j]==srcLeaf_ || failedLinkLeafs[j]==destLeaf_) && i==failedLinkSpines[j]) {
										// 			if(port==failedLinkSpines[j]) {
										// 				spineIsFine=0;
										// 				break;
										// 			}
										// 		}
										// 		if(spineIsFine) {
										// 			linkID_ = port;
										// 			break;
										// 		}
										// 	}
										// } else { // DA Flow, either not dynamic mapping, or dynamic but still within the threshold
										// 	if (ndatapack_ % flowcellSizePkts_ == 0 && ndatapack_ > 0) {
										// 		if((numFailures_ - numDirectFailures_)!=1) { // no need to change the linkID_ value if this value is 1
										// 			// loop by starting at the next linkID_ across all uplinks, and search for the next spine that is not directly failed
										// 			int port = (linkID_ + 1) % numUplinks;
										// 			int foundNextDASpine = 0;
										// 			for(int i = port; i < (port + numUplinks); i++) {
										// 				for(int j=0; j < numFailures_ ; j++) {
										// 					if((i % numUplinks)==failedLinkSpines[j]) {
										// 						if(failedLinkLeafs[j]!=srcLeaf_ && failedLinkLeafs[j]!=destLeaf_) {
										// 							linkID_ = i % numUplinks;
										// 							foundNextDASpine = 1;
										// 							break;
										// 						}
										// 					}
										// 				}
										// 				if(foundNextDASpine) {
										// 					break;
										// 				}
										// 			}
										// 		}
										// 	}
										// }
									} else if(poorPathFlow_) { // SPS or SPPS (Poor or Less-Used_Path) flow
										if(dynamicMapping_) {
											int thresh = 0;
											if(fromFailedLeaf_ || toFailedLeaf_) {
												thresh = dynamicMappingThreshold_;
											} else {
												thresh = dynamicMappingThresholdGL2GL_;
											}
											if(ndatabytes_ > thresh) {
												//if( ((fromFailedLeaf_ || toFailedLeaf_) && (ndatabytes_ > dynamicMappingThreshold_)) || ((!fromFailedLeaf_ && !toFailedLeaf_) && (ndatabytes_ > (100*dynamicMappingThreshold_)))) {
												poorPathFlow_ = 0; // remap poor link flows towards flows that are sprayed

												linkID_ = (linkID_ + 1) % numUplinks; // this executes for both multi (as default case) and single failure
												if(numFailures_ > 1) {
													//if(multipleFailure_) { commented 9-Jul-17
													// /* We may need to incorporate more variables (threshold-type) to decide whether DA spines are actually DA at all? */
													// for(int i=0; i < numUplinks; i++) {
													// 	int spineIsFine=1;
													// 	for(int j=0; j<numFailures_; j++) {
													// 		//if((failedLinkLeafs[j]==srcLeaf_ || failedLinkLeafs[j]==destLeaf_) && i==failedLinkSpines[j]) {
													// 		if(i==failedLinkSpines[j]) {
													// 			spineIsFine=0;
													// 			break;
													// 		}
													// 	}
													// 	if(spineIsFine) {
													// 		linkID_ = i;
													// 		break;
													// 	}
													// }
												} else {												
													if(linkID_ == failedLinks[0].spine) {
														linkID_ = (linkID_ + 1) % numUplinks;
													}
												}
											} else { // this is the poor path flow, continuing on its path
												if(numFailures_ > 1) {
													//if(multipleFailure_) { // This should work, if no new spine found, it will use old linkID_ value as before
													// if (ndatapack_ % flowcellSizePkts_ == 0 && ndatapack_ > 0) { // toggle between weak spines...
													// 	for(int j=0; j<numFailures_; j++) { // toggleWeakSpines_ = (toggleWeakSpines_ + 1) % numFailures_;
													// 		if((failedLinkLeafs[j]==srcLeaf_ || failedLinkLeafs[j]==destLeaf_) && failedLinkSpines[j] != linkID_) {
													// 			linkID_ = failedLinkSpines[j];
													// 			break;
													// 		}
													// 	}
													// }
												} else {
													linkID_ = failedLinks[0].spine;
													//linkID_ = failedLinkIndex_; // works for both multi and single failure... Commented 9-Jul-17
												}
											}
										} else { // not dynamic mapping (but still PoorPath chosen)
											if(numFailures_ > 1) {
												//if(multipleFailure_) { // This should work, if no new spine found, it will use old linkID_ value as before
												// if (ndatapack_ % flowcellSizePkts_ == 0 && ndatapack_ > 0) { // toggle between weak spines...
												// 	for(int j=0; j<numFailures_; j++) { // toggleWeakSpines_ = (toggleWeakSpines_ + 1) % numFailures_;
												// 		if((failedLinkLeafs[j]==srcLeaf_ || failedLinkLeafs[j]==destLeaf_) && failedLinkSpines[j] != linkID_) {
												// 			linkID_ = failedLinkSpines[j];
												// 			break;
												// 		}
												// 	}
												// }
											} else {
												linkID_ = failedLinks[0].spine;
												// linkID_ = failedLinkIndex_; Commented 9-Jul-17
											}
										}
									} else { // not a Poor (or Less-Used) Path Flow :: SPS, SPPS, SPPS-DASO or maybe HPO
										if (ndatapack_ % flowcellSizePkts_ == 0 && ndatapack_ > 0) {
											linkID_ = (linkID_ + 1) % numUplinks;
											if(numFailures_ > 1) {
												// if(multipleFailure_) { // Have to toggle onto next healthy spine ..  // Commented 9-Jul-17
												// for(int i = linkID_; i < (numUplinks + linkID_); i++) {
												// 	int spineIsFine=1;
												// 	int spine = i % numUplinks;
												// 	for(int j=0; j<numFailures_; j++) {
												// 		if(spine==failedLinkSpines[j]) { 
												// 			if(healthyPathOnly_==0 && DA_sprayOnly_==0) { 
												// 				spineIsFine=0; // Plain SPPS: stricter, not allow spray over DA-affected spines
												// 				break;
												// 			} else if(failedLinkLeafs[j]==srcLeaf_ || failedLinkLeafs[j]==destLeaf_){
												// 				spineIsFine=0; // case of either SPPS-DASO or HPO .. less scrutiny
												// 				break;
												// 			} else { /* nothing here*/ }
												// 		}
												// 	}
												// 	if(spineIsFine) {
												// 		linkID_ = spine;
												// 		break;
												// 	}
												// }
											} else {	
												if(linkID_ == failedLinks[0].spine) {
													//if(linkID_ == failedLinkIndex_) { Commented 9-Jul-17
													if((healthyPathOnly_==0 && DA_sprayOnly_==0) || (fromFailedLeaf_ || toFailedLeaf_)) {
														//if((fromFailedLeaf_ || toFailedLeaf_)) { // commented on 24-June-16, since now we r doing this for all flows
														linkID_ = (linkID_ + 1) % numUplinks; // Should not run for HPO GL to GL flows
														//}
													}
												}
											}
										}
									}
								} else { // WPS/WFS
									// #CheckingWFCS working code here
									if(ndatapack_ % flowcellSizePkts_ == 0) { // should we add the clause ndatapack_ > 0 ?? asking on 30-March
										if(numFailures_ > 1) {
											//if(multipleFailure_) { // Have to toggle onto next spine .. if healthy, OK, if not, check if time has come...
											if(debug_tcp_smi) {
												printf("DEBUG-Multiple Failures: Inside FailureCase==1!! numUplinks=%d flowCellMod_=%d \t", numUplinks, flowCellMod_);
												printf("toggle-Weak-Spines=%d, numDirectFailures_=%d ndatapack_=%d \n", toggleWeakSpines_, numDirectFailures_, (int) ndatapack_);
											}
											//// NEW LOGIC
											linkID_ = (linkID_ + 1) % numUplinks; // go to next link...

											if(numDirectFailures_ > 0) {
												int allChecksCleared = 0;
												int i = 0;
												while(allChecksCleared==0 && i < numUplinks) {
													int is_SpineFailed = isSpineFailed(linkID_); // (0 for not failed, 1 for partial, 2 for full failure	
													if(debug_tcp_smi) {
														printf("inside While Loop for all-checks-cleared ! \t");
														printf("for spine i=%d is_SpineFailed=%d \n", i, is_SpineFailed);
													}
													if(is_SpineFailed==2) {
														linkID_ = (linkID_ + 1) % numUplinks; // check next spine, this one is fully failed
													} else if(is_SpineFailed==1) { // partial failure spine ...
														if((ndatapack_ % flowCellMod_ == 0 && ndatapack_ > 0) || toggleWeakSpines_) {
															if(++toggleWeakSpines_ >= numDirectPartialFailurePaths) {
																// reset toggleWeakSpines_ and resize flowCellMod_
																toggleWeakSpines_ = 0;
																int pktsSentWeakPaths = flowcellSizePkts_ * numDirectPartialFailurePaths;
																int pktsSentHealthyPaths = flowcellSizePkts_ * rfactor;
																flowCellMod_ +=  pktsSentWeakPaths + pktsSentHealthyPaths;
															}
															allChecksCleared = 1; // if time has come for weak spine, then allow it
														} else {
															linkID_ = (linkID_ + 1) % numUplinks; // check next spine, since it's the turn of healthy spines
														}
													} else { // healthy spine ... 
														if((ndatapack_ % flowCellMod_ == 0 && ndatapack_ > 0) || toggleWeakSpines_) {
															linkID_ = (linkID_ + 1) % numUplinks; // turn of weak(er) spines
														} else {
															allChecksCleared = 1; // all OK
														}
													}
													i++;
												}
											}

											// if(numDirectFailures_ > 0) {
											// 	for(int i = linkID_; i < (numUplinks + linkID_); i++) {
											// 		if(numDirectPartialFailurePaths > 0) {

											// 			// PROBLEM NEEDS FIXING -- 12-jul-2017
											// 			if((ndatapack_ % flowCellMod_ == 0) || toggleWeakSpines_) {
											// 				if(spineIsFine==0) { // assign first weak spine now
											// 					linkID_ = spine;
											// 					if(++toggleWeakSpines_ >= numDirectPartialFailurePaths) {
											// 						//if(++toggleWeakSpines_ == numDirectFailures_) {
											// 						toggleWeakSpines_ = 0;
											// 						int pktsSentWeakPaths = flowcellSizePkts_ * numDirectPartialFailurePaths;
											// 						int pktsSentHealthyPaths = flowcellSizePkts_ * rfactor;
											// 						flowCellMod_ +=  pktsSentWeakPaths + pktsSentHealthyPaths;
											// 						//flowCellMod_ = flowCellMod_ + (flowcellSizePkts_ * numDirectFailures_) + (flowcellSizePkts_ * rfactor);
											// 					}
											// 					break;
											// 				} // if spineIsFine==-1, its a full failure, if ==1, its a good spine... iterate to next uplink
											// 			} else {
											// 				if(spineIsFine==1) {
											// 					linkID_ = spine;
											// 					break;
											// 				} // if spineIsFine==-1, its a full failure, if its ==0, its a partial failure, iterate to next uplink
											// 			}


											// 		} else {
											// 			if(spineIsFine==1) {
											// 				linkID_ = spine;
											// 				break;
											// 			} // if spineIsFine==-1, its a full failure, if its ==0, its a partial failure, iterate to next uplink
											// 		}


											// 	}
											// }
										} else { // single failure case
											linkID_ = (linkID_ + 1) % numUplinks; //
											if(linkID_ == failedLinks[0].spine && (fromFailedLeaf_ || toFailedLeaf_)) {
												//if(linkID_ == failedLinkIndex_ && (fromFailedLeaf_ || toFailedLeaf_)) {
												if((ndatapack_ % flowCellMod_ != 0) || failedLinks[0].failureRatio==100) {
													linkID_ = (linkID_ + 1) % numUplinks; //									
												} else {
													flowCellMod_ = flowCellMod_ + flowcellSizePkts_ + (flowcellSizePkts_ * rfactor);
												}
											}
										}
									}
								}
							} else if(failureCase_==2) { // full failure

								// KAKA: Currently multiple failures not implemented for full link failure case...
								if(debug_tcp_smi) {
									printf("DEBUG-0: Inside FailureCase==2!! \n");
								}
								if(selectiveSpraying_) { // HPO or SPS
									if(poorPathFlow_) { // this can only be a GL to GL flow, 
										linkID_ = failedLinks[0].spine;
										//linkID_ = failedLinkIndex_;

										if(dynamicMapping_ && (ndatabytes_ > dynamicMappingThresholdGL2GL_)) { // Symmetric Path with Dynamic Mapping
											// TODO: Might need to remap long flows, but at a larger threshold perhaps (June-24-2016) SMI
											poorPathFlow_ = 0; // remap towards flows that are sprayed
											linkID_ = (linkID_ + 1) % numUplinks;
											if(linkID_ == failedLinks[0].spine) {
												//if(linkID_ == failedLinkIndex_) {
												linkID_ = (linkID_ + 1) % numUplinks;
											}
										}
									} else {
										if(debug_tcp_smi) {
											printf("ndatapack_=%d; flowcellSizePkts_=%d  \t", (int) ndatapack_, flowcellSizePkts_);
											printf("fromFL=%d; toFL=%d; HPO=%d; DASO=%d \n", fromFailedLeaf_, toFailedLeaf_, healthyPathOnly_, DA_sprayOnly_);
										}
										if (ndatapack_ % flowcellSizePkts_ == 0) {
											linkID_ = (linkID_ + 1) % numUplinks; //
											if(linkID_ == failedLinks[0].spine) {
												//if(linkID_ == failedLinkIndex_) {
												if((healthyPathOnly_==0 && DA_sprayOnly_==0) || (fromFailedLeaf_ || toFailedLeaf_)) {
													linkID_ = (linkID_ + 1) % numUplinks; // This line should not execute for HPO GL-to-GL flows
												}
											}
										}
									}
								} else { // WPS, WFCS ??
									if (ndatapack_ % flowcellSizePkts_ == 0) {
										linkID_ = (linkID_ + 1) % numUplinks; //
										if(linkID_ == failedLinks[0].spine) {
											//if(linkID_ == failedLinkIndex_) {
											if(fromFailedLeaf_ || toFailedLeaf_) {
												linkID_ = (linkID_ + 1) % numUplinks; //
											}
										}
									}
								}						
							} else {
								// no failure TODO:SMI 24-June
								if(! selectiveSpraying_) { // handle the case for no failure and scheme is either WFCS or WPS (29 July 2016)
									if (ndatapack_ % flowcellSizePkts_ == 0){ // indicates the start of a new flowcell
										linkID_ = (linkID_ + 1) % numUplinks; // treat WPS/WFS like UPS/UFS
									}
								}
							}
						}
					} else { // dynamic failure currently not detected
						if (ndatapack_ % flowcellSizePkts_ == 0){ // indicates the start of a new flowcell
							linkID_ = (linkID_ + 1) % numUplinks; // treat WPS/WFS like UPS/UFS
						}

						if(debug_tcp_smi) {
							printf("DEBUG-TCP: Dynamic Failure Currently Not Detected at Time=%f \t ", time_now);
							if(! selectiveSpraying_) {
								printf("Treating WFCS like UFCS!! flowcellSizePkts_=%d linkID_=%d \n", flowcellSizePkts_, linkID_);
							}
						}
					}
				} else { // Probabilistics Schemes ... WPS-P, WFCS-P
					if(failureDetected_) {
						if (ndatapack_ % flowcellSizePkts_ == 0){ // indicates the start of a new flowcell
							int randomNumber = Random::integer(totalUplinkWeights);
							if( debug_tcp_smi) {
								printf("SMI-TEST: Probabilistic spraying -> randomNumber = %d \n", randomNumber);
							}
							int counter = 0, weightsAdded = 0;
							linkID_ = -1; 

							do {
								weightsAdded = weightsAdded + uplinkWeights[counter];
								if(randomNumber < weightsAdded) {
									linkID_ = counter;
								} else {
									counter++;
								}
							} while(linkID_ < 0 && counter < numUplinks);
			
							if(linkID_ < 0) { // Just a check to make sure we have chosen a valid port
								linkID_ = Random::integer(numUplinks);
							}
							if(debug_tcp_smi) {
								printf("SMI-TEST: linkID_=%d flowID=%d flowcellNum=%d ndatapack_=%d \n",linkID_, int(fid_), (int) ndatapack_ / flowcellSizePkts_ , (int)ndatapack_);
							}
						}
					} else {
						if (ndatapack_ % flowcellSizePkts_ == 0){
							linkID_ = (linkID_ + 1) % numUplinks; //linkID_++; //if (linkID_ > failedLink_) linkID_ = 0;
						}
					}
				}
			} else { // case of not being realistic failure
				if(roundRobin_) { // Deterministic Spraying
					if(selectiveSpraying_ && poorPathFlow_ && (fromFailedLeaf_ || toFailedLeaf_)) {
						// perhaps we don't need to do anything as linkID_ is already set...
					} else {
						if (ndatapack_ % flowcellSizePkts_ == 0){ // indicates the start of a new flowcell
							linkID_ = (linkID_ + 1) % numUplinks;
				
							if(linkID_ == failedLinks[0].spine) {
								//if(linkID_ == failedLinkIndex_) {
								if(debug_tcp_smi) {
									printf("DEBUG: New flowcell, on failed link! ndatapack=%d flowCellMod=%d rfactor=%d ! \n", (int)ndatapack_, flowCellMod_, rfactor);
								}
								if ((selectiveSpraying_ && (fromFailedLeaf_ || toFailedLeaf_) && poorPathFlow_==0) || (ndatapack_ % flowCellMod_ != 0)){
									linkID_ = (linkID_ + 1) % numUplinks;
								} else {
									if(debug_tcp_smi) {
										printf("DEBUG: Before: flowCellMod_ = % d ", flowCellMod_);
									}
									flowCellMod_ = flowCellMod_ + flowcellSizePkts_ + (flowcellSizePkts_ * rfactor);
									if(debug_tcp_smi) {
										printf("After: flowCellMod_ = % d \n", flowCellMod_);
									}
								}
							}
							if(debug_tcp_smi) {
								printf("Debug: flowID = %d Link = %d ndatapack_ = %d\n",int(fid_), linkID_, (int)ndatapack_);
							}
						}
					}
				} else { // Probabilistic Spraying
					if (ndatapack_ % flowcellSizePkts_ == 0){ // indicates the start of a new flowcell
						int randomNumber = Random::integer(totalUplinkWeights);
						if(debug_tcp_smi) {
							printf("SMI-TEST: Probabilistic spraying -> randomNumber = %d \n", randomNumber);
						}
						int counter = 0, weightsAdded = 0;
						linkID_ = -1; 

						do {
							weightsAdded = weightsAdded + uplinkWeights[counter];
							if(randomNumber < weightsAdded) {
								linkID_ = counter;
							} else {
								counter++;
							}
						} while(linkID_ < 0 && counter < numUplinks);
			
						if(linkID_ < 0) { // Just a check to make sure we have chosen a valid port
							linkID_ = Random::integer(numUplinks);
						}
						if(debug_tcp_smi) {
							printf("SMI-TEST: linkID_=%d flowID=%d flowcellNum=%d ndatapack_=%d \n",linkID_, int(fid_), (int) ndatapack_ / flowcellSizePkts_ , (int)ndatapack_);
						}
					}
				}
			}
		}
		tcph->linkID = linkID_; //Mohsin
		hf->flowcellUplink_ = linkID_ ; // LB decision for this flowcell : SMI [15-Dec-2015]
		hf->flowcellSeq_ = ndatapack_ / flowcellSizePkts_; // Flowcell number in flow.

		if(debug_tcp_smi) {
			printf("tcph->linkID=%d Flow-ID=%d flowcellNum=%d ndatapack_=%d \n", tcph->linkID, (int) fid_, (int) ndatapack_ / flowcellSizePkts_ , (int)ndatapack_);
		}

	} else {
		if(debug_tcp_smi) {
			printf("Either ECMP or FlowBender! \n");
		}

		// if(flowBender_) {

		// 	/*  When RTO occurs --> simply do:
		// 	  (1) toggleTTL_++;
		// 	  (2) perhaps we need to restart the flowbender timer and reset the (a) markedAckPkts_F , (b) totalAckPkts and perhaps the numRTTsCongested_
		// 	 */

		// 	// if(ndatapack_ % 50 == 0) {
		// 	// 	printf("Toggle TTL value for FlowBender iph->ttl()=%d; toggleTTL_=%d rtt_timeout()=%f \n", iph->ttl(), toggleTTL_, rtt_timeout() * tcp_tick_);
		// 	// 	toggleTTL_++;
		// 	// }

		// 	iph->ttl() = iph->ttl() + toggleTTL_;
		// 	//#ifdef debug_tcp_smi
		// 	printf("tcp:output() FLOWBENDER: Flow-ID=%d ndatapack_=%d iph->ttl()=%d \n", (int) fid_, (int)ndatapack_, iph->ttl());
		// 	//#endif
		// }
	}

	/*
	  task for today: 9-july-2017
	  failedLinks[flink].leaf; failedLinks[flink].spine; failedLinks[flink].failureRatio;
	  USE VARS ABOVE EVERYWHERE, INSTEAD OF:->
	  failedLinkLeafs[i], failedLinkSpines[i];
	  failedLinkIndex_, failedLinkLeaf_;
	  secondFailedLinkLeaf_, secondFailedLinkSpine_;
	*/

	tcph->ts() = Scheduler::instance().clock();
	int is_retransmit = (seqno < maxseq_);
 
	// Mark packet for diagnosis purposes if we are in Quick-Start Phase
	if (qs_approved_) {
		hf->qs() = 1;
	}
 
        // store timestamps, with bugfix_ts_.  From Andrei Gurtov. 
	// (A real TCP would use scoreboard for this.)
        if (bugfix_ts_ && tss==NULL) {
                tss = (double*) calloc(tss_size_, sizeof(double));
                if (tss==NULL) exit(1);
        }
        //dynamically grow the timestamp array if it's getting full
        if (bugfix_ts_ && ((seqno - highest_ack_) > tss_size_* 0.9)) {
                double *ntss;
                ntss = (double*) calloc(tss_size_*2, sizeof(double));
                printf("%p resizing timestamp table\n", this);
                if (ntss == NULL) exit(1);
                for (int i=0; i<tss_size_; i++)
                        ntss[(highest_ack_ + i) % (tss_size_ * 2)] =
                                tss[(highest_ack_ + i) % tss_size_];
                free(tss);
                tss_size_ *= 2;
                tss = ntss;
        }
 
        if (tss!=NULL)
                tss[seqno % tss_size_] = tcph->ts(); 

	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;
	tcph->last_rtt() = int(int(t_rtt_)*tcp_tick_*1000);

	if (ecn_) {
		hf->ect() = 1;	// ECN-capable transport
	}
	if (cong_action_ && (!is_retransmit || SetCWRonRetransmit_)) {
		hf->cong_action() = TRUE;  // Congestion action.
		cong_action_ = FALSE;
        }
	/* Check if this is the initial SYN packet. */
	if (seqno == 0) {
		if (syn_) {
			/* SMI:: For FlowBender, we have to start the timer for checking congestion per RTT, and rerouting if N consecutive RTTs F exceeds T -- 6-Jul-17 */
			if(flowBender_) {
				// flowBender_N += Random::integer(2);
				fl_bndr_timer.resched(rtt_timeout()); // * tcp_tick_ // SMI-Commented-jul25
			}
			databytes = 0;
			if (maxseq_ == -1) { // Added by SMI: DCTCP?
				curseq_ += 1; /*increment only on initial SYN*/
			}
			hdr_cmn::access(p)->size() = tcpip_base_hdr_size_;
			++syn_connects_;
			//fprintf(stderr, "TCPAgent: syn_connects_ %d max_connects_ %d\n",
			//	syn_connects_, max_connects_);
			if (max_connects_ > 0 &&
                               syn_connects_ > max_connects_) {
			      // Abort the connection.	
			      // What is the best way to abort the connection?	
			      curseq_ = 0;
	                      rtx_timer_.resched(10000);
                              return;
                        }
		}
		if (ecn_) {
			hf->ecnecho() = 1;
//			hf->cong_action() = 1;
			hf->ect() = 0;
		/* ali munir DCTCP */
		if (dctcp_)
			hf->ect() = 1;
		}
		if (qs_enabled_) {
			hdr_qs *qsh = hdr_qs::access(p);

			// dataout is kilobytes queued for sending
			int dataout = (curseq_ - maxseq_ - 1) * (size_ + headersize()) / 1024;
			int qs_rr = rate_request_;
			if (qs_request_mode_ == 1 && qs_rtt_ > 0) {
				// PS: Avoid making unnecessary QS requests
				// use a rough estimation of RTT in qs_rtt_
				// to calculate the desired rate from dataout.
				// printf("dataout %d qs_rr %d qs_rtt_ %d\n",
				//	dataout, qs_rr, qs_rtt_);
				if (dataout * 1000 / qs_rtt_ < qs_rr) {
					qs_rr = dataout * 1000 / qs_rtt_;
				}
				// printf("request %d\n", qs_rr);
				// qs_thresh_ is minimum number of unsent
				// segments needed to activate QS request
				// printf("curseq_ %d maxseq_ %d qs_thresh_ %d\n",
				//	 int(curseq_), int(maxseq_), qs_thresh_);
				if ((curseq_ - maxseq_ - 1) < qs_thresh_) {
					qs_rr = 0;
				}
			} 

		    	if (qs_rr > 0) {
				if (print_request_) 
					printf("QS request (before encoding): %d KBps\n", qs_rr);
				// QuickStart code from Srikanth Sundarrajan.
				qsh->flag() = QS_REQUEST;
				qsh->ttl() = Random::integer(256);
				ttl_diff_ = (iph->ttl() - qsh->ttl()) % 256;
				qsh->rate() = hdr_qs::Bps_to_rate(qs_rr * 1024);
				qs_requested_ = 1;
		    	} else {
				qsh->flag() = QS_DISABLE;
			}
		}
	}
	else if (useHeaders_ == true) {
		hdr_cmn::access(p)->size() += headersize();
	}
        hdr_cmn::access(p)->size();

	/* if no outstanding data, be sure to set rtx timer again */
	if (highest_ack_ == maxseq_)
		force_set_rtx_timer = 1;
	/* call helper function to fill in additional fields */
	output_helper(p);

        ++ndatapack_;
        ndatabytes_ += databytes;
	send(p, 0);
	if (seqno == curseq_ && seqno > maxseq_)
		idle();  // Tell application I have sent everything so far

	/* ali munir: begins DCTCP */
	if (dctcp_)
	if (seqno > dctcp_maxseq) 
		dctcp_maxseq = seqno;

	

	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_) {
				rtt_seq_ = seqno;
				rtt_ts_ = Scheduler::instance().clock();
			}
					
		}
	} else {
        	++nrexmitpack_;
		nrexmitbytes_ += databytes;
	}
	if (!(rtx_timer_.status() == TIMER_PENDING) || force_set_rtx_timer)
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

/*
 * Must convert bytes into packets for one-way TCPs.
 * If nbytes == -1, this corresponds to infinite send.  We approximate
 * infinite by a very large number (TCP_MAXSEQ).
 */
void TcpAgent::sendmsg(int nbytes, const char* /*flags*/)
{
	if (nbytes == -1 && curseq_ <= TCP_MAXSEQ)
		curseq_ = TCP_MAXSEQ; 
	else
		curseq_ += (nbytes/size_ + (nbytes%size_ ? 1 : 0));
	send_much(0, 0, maxburst_);
}

void TcpAgent::advanceby(int delta)
{
  curseq_ += delta;
	if (delta > 0)
		closed_ = 0;
	send_much(0, 0, maxburst_); 
}


/*
  This function is a helper function for deterministic WPS and WFCS, it helps in deciding whether the current chosen spine is failed (full/partial) or OK
 */
int TcpAgent::isSpineFailed(int spine) {
	int spineFailed = 0; // 0 for not failed, 1 for partial, 2 for full failure

	// if(fullFailureSpines[spine]==1) {
	// 	spineFailed = 2;
	// } else {
	for(int j=0; j<numFailures_; j++) {
		if((failedLinks[j].leaf==srcLeaf_ || failedLinks[j].leaf==destLeaf_) && spine==failedLinks[j].spine) {
			if(failedLinks[j].failureRatio==100) {
				spineFailed = 2;
			} else {
				spineFailed = 1;
			}
			break;
		}
	}
	// }

	return spineFailed;
}

/**
    The aim of this function is to toggle to the next valid link within the SVT this flow has been mapped to
 **/
void TcpAgent::nextValidLink() {
	linkID_ = (linkID_ + 1) % numUplinks;
	if(svtMapping==0) {
		while(svt_NoFailure[linkID_]==-1) {
			linkID_ = (linkID_ + 1) % numUplinks;
		}
	} else if(svtMapping==1) { 
		while(svt_partialFailure[linkID_]==-1) {
			linkID_ = (linkID_ + 1) % numUplinks;
		}
	} else if(svtMapping==10) {
		while(svt_DA_NoFailure[linkID_]==-1) {
			linkID_ = (linkID_ + 1) % numUplinks;
		}
	} else if(svtMapping==11) {
		while(svt_DA_partialFailure[linkID_]==-1) {
			linkID_ = (linkID_ + 1) % numUplinks;
		}
	} else if(svtMapping==12) {
		while(svt_DA_fullFailure[linkID_]==-1) {
			linkID_ = (linkID_ + 1) % numUplinks;
		}
	} else {
		// nothing else for now...
	}

}

int TcpAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "failedlinks") == 0) {
			//char* failedLinkSet = new char[100];
			//Tcl_GetString(argv[2]);
			//strcpy(failedLinkSet, argv[2]);
			strcpy(allFailedLinks, argv[2]);
			//printf("Failed Link Set = %s \n", allFailedLinks);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advance") == 0) {
			int newseq = atoi(argv[2]);
			if (newseq > maxseq_)
				advanceby(newseq - curseq_);
			else
				advanceby(maxseq_ - curseq_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advanceby") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "eventtrace") == 0) {
			et_ = (EventTrace *)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		/*
		 * Curtis Villamizar's trick to transfer tcp connection
		 * parameters to emulate http persistent connections.
		 *
		 * Another way to do the same thing is to open one tcp
		 * object and use start/stop/maxpkts_ or advanceby to control
		 * how much data is sent in each burst.
		 * With a single connection, slow_start_restart_
		 * should be configured as desired.
		 *
		 * This implementation (persist) may not correctly
		 * emulate pure-BSD-based systems which close cwnd
		 * after the connection goes idle (slow-start
		 * restart).  See appendix C in
		 * Jacobson and Karels ``Congestion
		 * Avoidance and Control'' at
		 * <ftp://ftp.ee.lbl.gov/papers/congavoid.ps.Z>
		 * (*not* the original
		 * '88 paper) for why BSD does this.  See
		 * ``Performance Interactions Between P-HTTP and TCP
		 * Implementations'' in CCR 27(2) for descriptions of
		 * what other systems do the same.
		 *
		 */
		if (strcmp(argv[1], "persist") == 0) {
			TcpAgent *other
			  = (TcpAgent*)TclObject::lookup(argv[2]);
			cwnd_ = other->cwnd_;
			awnd_ = other->awnd_;
			ssthresh_ = other->ssthresh_;
			t_rtt_ = other->t_rtt_;
			t_srtt_ = other->t_srtt_;
			t_rttvar_ = other->t_rttvar_;
			t_backoff_ = other->t_backoff_;
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

/*
 * Returns the window size adjusted to allow <num> segments past recovery
 * point to be transmitted on next ack.
 */
int TcpAgent::force_wnd(int num)
{
	return recover_ + num - (int)highest_ack_;
}

int TcpAgent::window()
{
        /*
         * If F-RTO is enabled and first ack has come in, temporarily open
         * window for sending two segments.
	 * The F-RTO code is from Pasi Sarolahti.  F-RTO is an algorithm
	 * for detecting spurious retransmission timeouts.
         */
        if (frto_ == 2) {
                return (force_wnd(2) < wnd_ ?
                        force_wnd(2) : (int)wnd_);
        } else {
		return (cwnd_ < wnd_ ? (int)cwnd_ : (int)wnd_);
        }
}

double TcpAgent::windowd()
{
	return (cwnd_ < wnd_ ? (double)cwnd_ : (double)wnd_);
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void TcpAgent::send_much(int force, int reason, int maxburst)
{
	send_idle_helper();
	int win = window();
	int npackets = 0;

	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	/* Save time when first packet was sent, for newreno  --Allman */
	if (t_seqno_ == 0)
		firstsent_ = Scheduler::instance().clock();

	if (burstsnd_timer_.status() == TIMER_PENDING)
		return;
	while (t_seqno_ <= highest_ack_ + win && t_seqno_ < curseq_) {
		if (overhead_ == 0 || force || qs_approved_) {
			output(t_seqno_, reason);
			npackets++;
			if (QOption_)
				process_qoption_after_send () ; 
			t_seqno_ ++ ;
			if (qs_approved_ == 1) {
				// delay = effective RTT / window
				double delay = (double) t_rtt_ * tcp_tick_ / win;
				if (overhead_) { 
					delsnd_timer_.resched(delay + Random::uniform(overhead_));
				} else {
					delsnd_timer_.resched(delay);
				}
				return;
			}
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		win = window();
		if (maxburst && npackets == maxburst)
			break;
	}
	/* call helper function */
	send_helper(maxburst);
}

/*
 * We got a timeout or too many duplicate acks.  Clear the retransmit timer.  
 * Resume the sequence one past the last packet acked.  
 * "mild" is 0 for timeouts and Tahoe dup acks, 1 for Reno dup acks.
 * "backoff" is 1 if the timer should be backed off, 0 otherwise.
 */
void TcpAgent::reset_rtx_timer(int mild, int backoff)
{
	if (backoff)
		rtt_backoff();
	set_rtx_timer();
	if (!mild)
		t_seqno_ = highest_ack_ + 1;
	rtt_active_ = 0;
}

/*
 * Set retransmit timer using current rtt estimate.  By calling resched(), 
 * it does not matter whether the timer was already running.
 */
void TcpAgent::set_rtx_timer()
{
	rtx_timer_.resched(rtt_timeout());
}

/*
 * Set new retransmission timer if not all outstanding
 * or available data acked, or if we are unable to send because 
 * cwnd is less than one (as when the ECN bit is set when cwnd was 1).
 * Otherwise, if a timer is still outstanding, cancel it.
 */
void TcpAgent::newtimer(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	/*
	 * t_seqno_, the next packet to send, is reset (decreased) 
	 *   to highest_ack_ + 1 after a timeout,
	 *   so we also have to check maxseq_, the highest seqno sent.
	 * In addition, if the packet sent after the timeout has
	 *   the ECN bit set, then the returning ACK caused cwnd_ to
	 *   be decreased to less than one, and we can't send another
	 *   packet until the retransmit timer again expires.
	 *   So we have to check for "cwnd_ < 1" as well.
	 */
	if (t_seqno_ > tcph->seqno() || tcph->seqno() < maxseq_ || cwnd_ < 1) 
		set_rtx_timer();
	else
		cancel_rtx_timer();
}

/*
 * for experimental, high-speed TCP
 */
double TcpAgent::linear(double x, double x_1, double y_1, double x_2, double y_2)
{
	// The y coordinate factor ranges from y_1 to y_2
	//  as the x coordinate ranges from x_1 to x_2.
	double y = y_1 + ((y_2 - y_1) * ((x - x_1)/(x_2-x_1)));
	return y;
}

/*
 * Limited Slow-Start for large congestion windows.
 * This should only be called when max_ssthresh_ is non-zero.
 */
double TcpAgent::limited_slow_start(double cwnd, int max_ssthresh, double increment)
{
        if (max_ssthresh <= 0) {
	  	return increment;
	} else {
                double increment1 = 0.0;
		int round = int(cwnd / (double(max_ssthresh)/2.0));
		if (round > 0) {
		  	increment1 = 1.0/double(round); 
		} 
		if (increment < increment1) {
		  	return increment1;
		} else {
		  	return increment;
		}
	}
}

/*
 * For retrieving numdupacks_.
 */
int TcpAgent::numdupacks(double cwnd)
{
        int cwndfraction = (int) cwnd/numdupacksFrac_;
	if (numdupacks_ > cwndfraction) {
	  	return numdupacks_;
        } else {
	  	return cwndfraction;
	}
}

/*
 * Calculating the decrease parameter for highspeed TCP.
 */
double TcpAgent::decrease_param()
{
	double decrease;
	// OLD:
	// decrease = linear(log(cwnd_), log(low_window_), 0.5, log(high_window_), high_decrease_);
	// NEW (but equivalent):
        decrease = hstcp_.dec1 + log(cwnd_) * hstcp_.dec2;  
	return decrease;
}

/*
 * Calculating the increase parameter for highspeed TCP.
 */
double TcpAgent::increase_param()
{
	double increase, decrease, p, answer;
	/* extending the slow-start for high-speed TCP */

	/* for highspeed TCP -- from Sylvia Ratnasamy, */
	/* modifications by Sally Floyd and Evandro de Souza */
 	// p ranges from 1.5/W^2 at congestion window low_window_, to
	//    high_p_ at congestion window high_window_, on a log-log scale.
        // The decrease factor ranges from 0.5 to high_decrease
	//  as the window ranges from low_window to high_window, 
	//  as the log of the window. 
	// For an efficient implementation, this would just be looked up
	//   in a table, with the increase and decrease being a function of the
	//   congestion window.

       if (cwnd_ <= low_window_) { 
		answer = 1 / cwnd_;
       		return answer; 
       } else if (cwnd_ >= hstcp_.cwnd_last_ && 
	      cwnd_ < hstcp_.cwnd_last_ + cwnd_range_) {
	      // cwnd_range_ can be set to 0 to be disabled,
	      //  or can be set from 1 to 100 
       		answer = hstcp_.increase_last_ / cwnd_;
              	return answer;
       } else { 
		// OLD:
 		// p = exp(linear(log(cwnd_), log(low_window_), log(hstcp_.low_p), log(high_window_), log(high_p_)));
		// NEW, but equivalent:
        	p = exp(hstcp_.p1 + log(cwnd_) * hstcp_.p2);  
        	decrease = decrease_param();
		// OLD:
		// increase = cwnd_*cwnd_*p *(2.0*decrease)/(2.0 - decrease); 
		// NEW, but equivalent:
		increase = cwnd_ * cwnd_ * p /(1/decrease - 0.5);
		//	if (increase > max_increase) { 
		//		increase = max_increase;
		//	} 
		answer = increase / cwnd_;
		hstcp_.cwnd_last_ = cwnd_;
		hstcp_.increase_last_ = increase;
       		return answer;
	}       
}

/*
 * open up the congestion window
 */
void TcpAgent::opencwnd()
{
	double increment;
	if (cwnd_ < ssthresh_) {
		/* slow-start (exponential) */
		cwnd_ += 1;
	} else {
		/* linear */
		double f;
		switch (wnd_option_) {
		case 0:
			if (++count_ >= cwnd_) {
				count_ = 0;
				++cwnd_;
			}
			break;

		case 1:
			/* This is the standard algorithm. */
			increment = increase_num_ / cwnd_;
			if ((last_cwnd_action_ == 0 ||
			  last_cwnd_action_ == CWND_ACTION_TIMEOUT) 
			  && max_ssthresh_ > 0) {
				increment = limited_slow_start(cwnd_,
				  max_ssthresh_, increment);
			}
			cwnd_ += increment;
			break;

		case 2:
			/* These are window increase algorithms
			 * for experimental purposes only. */
			/* This is the Constant-Rate increase algorithm 
                         *  from the 1991 paper by S. Floyd on "Connections  
			 *  with Multiple Congested Gateways". 
			 *  The window is increased by roughly 
			 *  wnd_const_*RTT^2 packets per round-trip time.  */
			f = (t_srtt_ >> T_SRTT_BITS) * tcp_tick_;
			f *= f;
			f *= wnd_const_;
			/* f = wnd_const_ * RTT^2 */
			f += fcnt_;
			if (f > cwnd_) {
				fcnt_ = 0;
				++cwnd_;
			} else
				fcnt_ = f;
			break;

		case 3:
			/* The window is increased by roughly 
			 *  awnd_^2 * wnd_const_ packets per RTT,
			 *  for awnd_ the average congestion window. */
			f = awnd_;
			f *= f;
			f *= wnd_const_;
			f += fcnt_;
			if (f > cwnd_) {
				fcnt_ = 0;
				++cwnd_;
			} else
				fcnt_ = f;
			break;

                case 4:
			/* The window is increased by roughly 
			 *  awnd_ * wnd_const_ packets per RTT,
			 *  for awnd_ the average congestion window. */
                        f = awnd_;
                        f *= wnd_const_;
                        f += fcnt_;
                        if (f > cwnd_) {
                                fcnt_ = 0;
                                ++cwnd_;
                        } else
                                fcnt_ = f;
                        break;
		case 5:
			/* The window is increased by roughly wnd_const_*RTT 
			 *  packets per round-trip time, as discussed in
			 *  the 1992 paper by S. Floyd on "On Traffic 
			 *  Phase Effects in Packet-Switched Gateways". */
                        f = (t_srtt_ >> T_SRTT_BITS) * tcp_tick_;
                        f *= wnd_const_;
                        f += fcnt_;
                        if (f > cwnd_) {
                                fcnt_ = 0;
                                ++cwnd_;
                        } else
                                fcnt_ = f;
                        break;
                case 6:
                        /* binomial controls */ 
                        cwnd_ += increase_num_ / (cwnd_*pow(cwnd_,k_parameter_));                
                        break; 
 		case 8: 
			/* high-speed TCP, RFC 3649 */
			increment = increase_param();
			if ((last_cwnd_action_ == 0 ||
			  last_cwnd_action_ == CWND_ACTION_TIMEOUT) 
			  && max_ssthresh_ > 0) {
				increment = limited_slow_start(cwnd_,
				  max_ssthresh_, increment);
			}
			cwnd_ += increment;
                        break;
		default:
#ifdef notdef
			/*XXX*/
			error("illegal window option %d", wnd_option_);
#endif
			abort();
		}
	}
	// if maxcwnd_ is set (nonzero), make it the cwnd limit
	if (maxcwnd_ && (int(cwnd_) > maxcwnd_))
		cwnd_ = maxcwnd_;

	return;
}

void
TcpAgent::slowdown(int how)
{
	double decrease;  /* added for highspeed - sylvia */
	double win, halfwin, decreasewin;
	int slowstart = 0;
	++ncwndcuts_;
	if (!(how & TCP_IDLE) && !(how & NO_OUTSTANDING_DATA)){
		++ncwndcuts1_; 
	}
	// we are in slowstart for sure if cwnd < ssthresh
	if (cwnd_ < ssthresh_) 
		slowstart = 1;
        if (precision_reduce_) {
		halfwin = windowd() / 2;
                if (wnd_option_ == 6) {         
                        /* binomial controls */
                        decreasewin = windowd() - (1.0-decrease_num_)*pow(windowd(),l_parameter_);
                } else if (wnd_option_ == 8 && (cwnd_ > low_window_)) { 
                        /* experimental highspeed TCP */
			decrease = decrease_param();
			//if (decrease < 0.1) 
			//	decrease = 0.1;
			decrease_num_ = decrease;
                        decreasewin = windowd() - (decrease * windowd());
                } else {
	 		decreasewin = decrease_num_ * windowd();
		}
		win = windowd();
	} else  {
		int temp;
		temp = (int)(window() / 2);
		halfwin = (double) temp;
                if (wnd_option_ == 6) {
                        /* binomial controls */
                        temp = (int)(window() - (1.0-decrease_num_)*pow(window(),l_parameter_));
                } else if ((wnd_option_ == 8) && (cwnd_ > low_window_)) { 
                        /* experimental highspeed TCP */
			decrease = decrease_param();
			//if (decrease < 0.1)
                        //       decrease = 0.1;		
			decrease_num_ = decrease;
                        temp = (int)(windowd() - (decrease * windowd()));
                } else {
 			temp = (int)(decrease_num_ * window());
		}
		decreasewin = (double) temp;
		win = (double) window();
	}
	if (how & CLOSE_SSTHRESH_HALF)
		// For the first decrease, decrease by half
		// even for non-standard values of decrease_num_.
		if (first_decrease_ == 1 || slowstart ||
			last_cwnd_action_ == CWND_ACTION_TIMEOUT) {
			// Do we really want halfwin instead of decreasewin
		// after a timeout?
			ssthresh_ = (int) halfwin;
		} else {
			ssthresh_ = (int) decreasewin;
		}
	else if (how & CLOSE_SSTHRESH_DCTCP) { // DCTCP
		ssthresh_ = (int) ((1 - dctcp_alpha_/2.0) * windowd());
	if(debug_dctcp)
		fprintf(stdout,"DCTCP backoff (ssthresh) applied (alpha=%f) at time= %f\n", (double)dctcp_alpha_, (double)Scheduler::instance().clock());
	}
        else if (how & THREE_QUARTER_SSTHRESH)
		if (ssthresh_ < 3*cwnd_/4)
			ssthresh_  = (int)(3*cwnd_/4);
	if (how & CLOSE_CWND_HALF)
		// For the first decrease, decrease by half
		// even for non-standard values of decrease_num_.
		if (first_decrease_ == 1 || slowstart || decrease_num_ == 0.5) {
			cwnd_ = halfwin;
		} else cwnd_ = decreasewin;
	else if (how & CLOSE_CWND_DCTCP){ // DCTCP
	if(debug_dctcp)
		fprintf(stdout,"DCTCP backoff (cwnd down) applied (alpha=%f) at time= %f\n", (double)dctcp_alpha_, (double)Scheduler::instance().clock());	
		cwnd_ = (1 - dctcp_alpha_/2.0) * windowd();
	}
        else if (how & CWND_HALF_WITH_MIN) {
		// We have not thought about how non-standard TCPs, with
		// non-standard values of decrease_num_, should respond
		// after quiescent periods.
                cwnd_ = decreasewin;
                if (cwnd_ < 1)
                        cwnd_ = 1;
	}
	else if (how & CLOSE_CWND_RESTART) 
		cwnd_ = int(wnd_restart_);
	else if (how & CLOSE_CWND_INIT)
		cwnd_ = int(wnd_init_);
	else if (how & CLOSE_CWND_ONE)
		cwnd_ = 1;
	else if (how & CLOSE_CWND_HALF_WAY) {
		// cwnd_ = win - (win - W_used)/2 ;
		cwnd_ = W_used + decrease_num_ * (win - W_used);
                if (cwnd_ < 1)
                        cwnd_ = 1;
	}
	if (ssthresh_ < 2)
		ssthresh_ = 2;
	if (cwnd_ < 1)
		cwnd_ = 1; // DCTCP
	if (how & (CLOSE_CWND_HALF|CLOSE_CWND_RESTART|CLOSE_CWND_INIT|CLOSE_CWND_ONE|CLOSE_CWND_DCTCP))
		cong_action_ = TRUE;

	fcnt_ = count_ = 0;
	if (first_decrease_ == 1)
		first_decrease_ = 0;
	// for event tracing slow start
	if (cwnd_ == 1 || slowstart) 
		// Not sure if this is best way to capture slow_start
		// This is probably tracing a superset of slowdowns of
		// which all may not be slow_start's --Padma, 07/'01.
		trace_event("SLOW_START");
	


	
}

/*
 * Process a packet that acks previously unacknowleged data.
 */
void TcpAgent::newack(Packet* pkt)
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	/* 
	 * Wouldn't it be better to set the timer *after*
	 * updating the RTT, instead of *before*? 
	 */
	if (!timerfix_) newtimer(pkt);
	dupacks_ = 0;
	last_ack_ = tcph->seqno();
	prev_highest_ack_ = highest_ack_ ;
	highest_ack_ = last_ack_;

	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	/* 
	 * Update RTT only if it's OK to do so from info in the flags header.
	 * This is needed for protocols in which intermediate agents
	 * in the network intersperse acks (e.g., ack-reconstructors) for
	 * various reasons (without violating e2e semantics).
	 */	
	hdr_flags *fh = hdr_flags::access(pkt);
	if (!fh->no_ts_) {
		if (ts_option_) {
			ts_echo_=tcph->ts_echo();
			rtt_update(now - tcph->ts_echo());
			if (ts_resetRTO_ && (!ect_ || !ecn_backoff_ ||
			    !hdr_flags::access(pkt)->ecnecho())) { 
				// From Andrei Gurtov
				/* 
				 * Don't end backoff if still in ECN-Echo with
			 	 * a congestion window of 1 packet. 
				 */
				t_backoff_ = 1;
				ecn_backoff_ = 0;
			}
		}
		if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
			if (!ect_ || !ecn_backoff_ || 
				!hdr_flags::access(pkt)->ecnecho()) {
				/* 
				 * Don't end backoff if still in ECN-Echo with
			 	 * a congestion window of 1 packet. 
				 */
				t_backoff_ = 1;
				ecn_backoff_ = 0;
			}
			rtt_active_ = 0;
			if (!ts_option_)
				rtt_update(now - rtt_ts_);
		}
	}
	if (timerfix_) newtimer(pkt);
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd_;
}


/*
 * Respond either to a source quench or to a congestion indication bit.
 * This is done at most once a roundtrip time;  after a source quench,
 * another one will not be done until the last packet transmitted before
 * the previous source quench has been ACKed.
 *
 * Note that this procedure is called before "highest_ack_" is
 * updated to reflect the current ACK packet.  
 */
void TcpAgent::ecn(int seqno)
{
	if (seqno > recover_ || 
	      last_cwnd_action_ == CWND_ACTION_TIMEOUT) {
		recover_ =  maxseq_;
		last_cwnd_action_ = CWND_ACTION_ECN;
		if (cwnd_ <= 1.0) {
			if (ecn_backoff_) 
				rtt_backoff();
			else ecn_backoff_ = 1;
		} else ecn_backoff_ = 0;
		if (dctcp_)  
			slowdown(CLOSE_CWND_DCTCP|CLOSE_SSTHRESH_DCTCP); // DCTCP
		else
		slowdown(CLOSE_CWND_HALF|CLOSE_SSTHRESH_HALF);
		++necnresponses_ ;
		// added by sylvia to count number of ecn responses 
	}
}

/*
 *  Is the connection limited by the network (instead of by a lack
 *    of data from the application?
 */
int TcpAgent::network_limited() {
	int win = window () ;
	if (t_seqno_ > (prev_highest_ack_ + win))
		return 1;
	else
		return 0;
}

void TcpAgent::recv_newack_helper(Packet *pkt) {
	//hdr_tcp *tcph = hdr_tcp::access(pkt);
	newack(pkt);
        if (qs_window_ && highest_ack_ >= qs_window_) {
                // All segments in the QS window have been acknowledged.
                // We can exit the Quick-Start phase.
                qs_window_ = 0;
        }
	if (!ect_ || !hdr_flags::access(pkt)->ecnecho() ||
		(old_ecn_ && ecn_burst_)) {
		/* If "old_ecn", this is not the first ACK carrying ECN-Echo
		 * after a period of ACKs without ECN-Echo.
		 * Therefore, open the congestion window. */
		/* if control option is set, and the sender is not
			 window limited, then do not increase the window size */
		
		if (!control_increase_ || 
		   (control_increase_ && (network_limited() == 1))) 
	      		opencwnd();
	}
	if (ect_) {
		if (!hdr_flags::access(pkt)->ecnecho())
			ecn_backoff_ = 0;
		if (!ecn_burst_ && hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = TRUE;
		else if (ecn_burst_ && ! hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = FALSE;
	}
	if (!ect_ && hdr_flags::access(pkt)->ecnecho() &&
		!hdr_flags::access(pkt)->cong_action())
		ect_ = 1;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
	if (QOption_ && curseq_ == highest_ack_ +1) {
		cancel_rtx_timer();
	}
	if (frto_ == 1) {
		/*
		 * New ack after RTO. If F-RTO is enabled, try to transmit new
		 * previously unsent segments.
		 * If there are no new data or receiver window limits the
		 * transmission, revert to traditional recovery.
		 */
		if (recover_ + 1 >= highest_ack_ + wnd_ ||
		    recover_ + 1 >= curseq_) {
			frto_ = 0;
 		} else if (highest_ack_ == recover_) {
 			/*
 			 * F-RTO step 2a) RTO retransmission fixes whole
			 * window => cancel F-RTO
 			 */
 			frto_ = 0;
		} else {
			t_seqno_ = recover_ + 1;
			frto_ = 2;
		}
	} else if (frto_ == 2) {
		/*
		 * Second new ack after RTO. If F-RTO is enabled, RTO can be
		 * declared spurious
		 */
		spurious_timeout();
	}
}

/*
 * Set the initial window. 
 */
double
TcpAgent::initial_window()
{
        // If Quick-Start Request was approved, use that as a basis for
        // initial window
        if (qs_cwnd_) {
                return (qs_cwnd_);
        }
	//
	// init_option = 1: static iw of wnd_init_
	//
	if (wnd_init_option_ == 1) {
		return (wnd_init_);
	}
        else if (wnd_init_option_ == 2) {
		// do iw according to Internet draft
 		if (size_ <= 1095) {
			return (4.0);
	 	} else if (size_ < 2190) {
			return (3.0);
		} else {
			return (2.0);
		}
	}
	// XXX what should we return here???
	fprintf(stderr, "Wrong number of wnd_init_option_ %d\n", 
		wnd_init_option_);
	abort();
	return (2.0); // XXX make msvc happy.
}

/*
 * Dupack-action: what to do on a DUP ACK.  After the initial check
 * of 'recover' below, this function implements the following truth
 * table:
 *
 *	bugfix	ecn	last-cwnd == ecn	action
 *
 *	0	0	0			tahoe_action
 *	0	0	1			tahoe_action	[impossible]
 *	0	1	0			tahoe_action
 *	0	1	1			slow-start, return
 *	1	0	0			nothing
 *	1	0	1			nothing		[impossible]
 *	1	1	0			nothing
 *	1	1	1			slow-start, return
 */

/* 
 * A first or second duplicate acknowledgement has arrived, and
 * singledup_ is enabled.
 * If the receiver's advertised window permits, and we are exceeding our
 * congestion window by less than numdupacks_, then send a new packet.
 */
void
TcpAgent::send_one()
{
	if (t_seqno_ <= highest_ack_ + wnd_ && t_seqno_ < curseq_ &&
		t_seqno_ <= highest_ack_ + cwnd_ + dupacks_ ) {
		output(t_seqno_, 0);
		if (QOption_)
			process_qoption_after_send () ;
		t_seqno_ ++ ;
		// send_helper(); ??
	}
	return;
}

void
TcpAgent::dupack_action()
{
	int recovered = (highest_ack_ > recover_);
	if (recovered || (!bug_fix_ && !ecn_) || 
		(bugfix_ss_ && highest_ack_ == 0)) {
		// (highest_ack_ == 0) added to allow Fast Retransmit
		//  when the first data packet is dropped.
		//  Bug report from Mark Allman.
		goto tahoe_action;
	}

	if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		last_cwnd_action_ = CWND_ACTION_DUPACK;
		slowdown(CLOSE_CWND_ONE);
		reset_rtx_timer(0,0);
		return;
	}

	if (bug_fix_) {
		/*
		 * The line below, for "bug_fix_" true, avoids
		 * problems with multiple fast retransmits in one
		 * window of data. 
		 */
		return;
	}

tahoe_action:
        recover_ = maxseq_;
        if (!lossQuickStart()) {
		// we are now going to fast-retransmit and willtrace that event
		trace_event("FAST_RETX");
		last_cwnd_action_ = CWND_ACTION_DUPACK;
		slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_ONE);
	}
	reset_rtx_timer(0,0);
	return;
}

/*
 * When exiting QuickStart, reduce the congestion window to the
 *   size that was actually used.
 */
void TcpAgent::endQuickStart()
{
	qs_approved_ = 0;
        qs_cwnd_ = 0;
        qs_window_ = maxseq_;
	int new_cwnd = maxseq_ - last_ack_;
	if (new_cwnd > 1 && new_cwnd < cwnd_) {
	 	cwnd_ = new_cwnd;
		if (cwnd_ < initial_window()) 
			cwnd_ = initial_window();
	}
}

void TcpAgent::processQuickStart(Packet *pkt)
{
	// QuickStart code from Srikanth Sundarrajan.
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	hdr_qs *qsh = hdr_qs::access(pkt);
	double now = Scheduler::instance().clock();
	int app_rate;

        // printf("flag: %d ttl: %d ttl_diff: %d rate: %d\n", qsh->flag(),
	//     qsh->ttl(), ttl_diff_, qsh->rate());
	qs_requested_ = 0;
	qs_approved_ = 0;
	if (qsh->flag() == QS_RESPONSE && qsh->ttl() == ttl_diff_ && 
            qsh->rate() > 0) {
                app_rate = (int) ((hdr_qs::rate_to_Bps(qsh->rate()) *
                      (now - tcph->ts_echo())) / (size_ + headersize()));
		if (print_request_) {
		  double num1 = hdr_qs::rate_to_Bps(qsh->rate());
		  double time = now - tcph->ts_echo();
		  int size = size_ + headersize();
		  printf("Quick Start request, rate: %g Bps, encoded rate: %d\n", 
		     num1, qsh->rate());
		  printf("Quick Start request, window %d rtt: %4.2f pktsize: %d\n",
		     app_rate, time, size);
		}
                if (app_rate > initial_window()) {
			qs_cwnd_ = app_rate;
                        qs_approved_ = 1;
                }
        } else { // Quick Start rejected
#ifdef QS_DEBUG
                printf("Quick Start rejected\n");
#endif
        }

}



/*
 * ACK has been received, hook from recv()
 */
void TcpAgent::recv_frto_helper(Packet *pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	if (tcph->seqno() == last_ack_ && frto_ != 0) {
		/*
		 * Duplicate ACK while in F-RTO indicates that the
		 * timeout was valid. Go to slow start retransmissions.
		 */
		t_seqno_ = highest_ack_ + 1;
		cwnd_ = frto_;
		frto_ = 0;

		// Must zero dupacks (in order to trigger send_much at recv)
		// dupacks is increased in recv after exiting this function
		dupacks_ = -1;
	}
}


/*
 * A spurious timeout has been detected. Do appropriate actions.
 */
void TcpAgent::spurious_timeout()
{
	frto_ = 0;

	if(debug_tcp_smi) {
		printf("Inside spurious_timeout()\n");
	}


	switch (spurious_response_) {
	case 1:
	default:
		/*
		 * Full revert of congestion window
		 * (FlightSize before last acknowledgment)
		 */
		cwnd_ = t_seqno_ - prev_highest_ack_;
		break;
 
	case 2:
		/*
		 * cwnd = reduced ssthresh (approx. half of the earlier pipe)
		 */
		cwnd_ = ssthresh_; break;
	case 3:
		/*
		 * slow start, but without retransmissions
		 */
		cwnd_ = 1; break;
	}

	/*
	 * Revert ssthresh to size before retransmission timeout
	 */
	ssthresh_ = pipe_prev_;

	/* If timeout was spurious, bugfix is not needed */
	recover_ = highest_ack_ - 1;
}


/*
 * Loss occurred in Quick-Start window.
 * If Quick-Start is enabled, packet loss in the QS phase, during slow-start,
 * should trigger slow start instead of the regular fast retransmit.
 * We use variable tcp_qs_recovery_ to toggle this behaviour on and off.
 * If tcp_qs_recovery_ is true, initiate slow start to probe for
 * a correct window size.
 *
 * Return value: non-zero if Quick-Start specific loss recovery took place
 */
int TcpAgent::lossQuickStart()
{
       if (qs_window_ && tcp_qs_recovery_) {
                //recover_ = maxseq_;
                //reset_rtx_timer(1,0);
                slowdown(CLOSE_CWND_INIT);
		// reset ssthresh to half of W-D/2?
                qs_window_ = 0;
                output(last_ack_ + 1, TCP_REASON_DUPACK);
                return 1;
       }
       return 0;
}




/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void TcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	int valid_ack = 0;
	if (qs_approved_ == 1 && tcph->seqno() > last_ack_) 
		endQuickStart();
	if (qs_requested_ == 1)
		processQuickStart(pkt);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif
	/* W.N.: check if this is from a previous incarnation */
	if (tcph->ts() < lastreset_) {
		// Remove packet and do nothing
		Packet::free(pkt);
		return;
	}

	++nackpack_;
	ts_peer_ = tcph->ts();
	int ecnecho = hdr_flags::access(pkt)->ecnecho();
	if (ecnecho && ecn_)
		ecn(tcph->seqno());
	recv_helper(pkt);
	recv_frto_helper(pkt);

	/* grow cwnd and check if the connection is done */ 
	if (tcph->seqno() > last_ack_) {
		recv_newack_helper(pkt);
		if (last_ack_ == 0 && delay_growth_) { 
			cwnd_ = initial_window();
		}
	} else if (tcph->seqno() == last_ack_) {
                if (hdr_flags::access(pkt)->eln_ && eln_) {
                        tcp_eln(pkt);
                        return;
                }
		if (++dupacks_ == numdupacks_ && !noFastRetrans_) {
			dupack_action();
		} else if (dupacks_ < numdupacks_ && singledup_ ) {
			send_one();
		}
	}

	if (QOption_ && EnblRTTCtr_)
		process_qoption_after_ack (tcph->seqno());

	if (tcph->seqno() >= last_ack_)  
		// Check if ACK is valid.  Suggestion by Mark Allman. 
		valid_ack = 1;
	Packet::free(pkt);
	/*
	 * Try to send more data.
	 */
	if (valid_ack || aggressive_maxburst_)
		send_much(0, 0, maxburst_);
}

/*
 * Process timeout events other than rtx timeout. Having this as a separate 
 * function allows derived classes to make alterations/enhancements (e.g.,
 * response to new types of timeout events).
 */ 
void TcpAgent::timeout_nonrtx(int tno) 
{
	if (tno == TCP_TIMER_DELSND)  {
	 /*
	 	* delayed-send timer, with random overhead
	 	* to avoid phase effects
	 	*/
		send_much(1, TCP_REASON_TIMEOUT, maxburst_);
	}
}
	
void TcpAgent::timeout(int tno)
{
	/* retransmit timer */
	if (tno == TCP_TIMER_RTX) {

		// There has been a timeout - will trace this event
		trace_event("TIMEOUT");

		frto_ = 0;
		// Set pipe_prev as per Eifel Response
		pipe_prev_ = (window() > ssthresh_) ?
			window() : (int)ssthresh_;

	        if (cwnd_ < 1) cwnd_ = 1;
		if (qs_approved_ == 1) qs_approved_ = 0;
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.  
			 */
			 // Should this return be here?
			 // What if CWND_ACTION_ECN and cwnd < 1?
			 // return;
		} else {
			recover_ = maxseq_;
			if (highest_ack_ == -1 && wnd_init_option_ == 2)
				/* 
				 * First packet dropped, so don't use larger
				 * initial windows. 
				 */
				wnd_init_option_ = 1;
                        else if ((highest_ack_ == -1) &&
                                (wnd_init_option_ == 1) && (wnd_init_ > 1)
				&& bugfix_ss_)
                                /*
                                 * First packet dropped, so don't use larger
                                 * initial windows.  Bugfix from Mark Allman.
                                 */
                                wnd_init_ = 1;
			if (highest_ack_ == maxseq_ && restart_bugfix_)
			       /* 
				* if there is no outstanding data, don't cut 
				* down ssthresh_.
				*/
				slowdown(CLOSE_CWND_ONE|NO_OUTSTANDING_DATA);
			else if (highest_ack_ < recover_ &&
			  last_cwnd_action_ == CWND_ACTION_ECN) {
			       /*
				* if we are in recovery from a recent ECN,
				* don't cut down ssthresh_.
				*/
				slowdown(CLOSE_CWND_ONE);
				if (frto_enabled_ || sfrto_enabled_) {
					frto_ = 1;
				}
			}
			else {
				++nrexmit_;
				last_cwnd_action_ = CWND_ACTION_TIMEOUT;
				slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);
				if (frto_enabled_ || sfrto_enabled_) {
					frto_ = 1;
				}
			}
		}
		/* if there is no outstanding data, don't back off rtx timer */
		if (highest_ack_ == maxseq_ && restart_bugfix_) {
			reset_rtx_timer(0,0);
		}
		else {
			reset_rtx_timer(0,1);
		}
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
		send_much(0, TCP_REASON_TIMEOUT, maxburst_);
	} 
	else {
		timeout_nonrtx(tno);
	}
}

/* 
 * Check if the packet (ack) has the ELN bit set, and if it does, and if the
 * last ELN-rxmitted packet is smaller than this one, then retransmit the
 * packet.  Do not adjust the cwnd when this happens.
 */
void TcpAgent::tcp_eln(Packet *pkt)
{
        //int eln_rxmit;
        hdr_tcp *tcph = hdr_tcp::access(pkt);
        int ack = tcph->seqno();

        if (++dupacks_ == eln_rxmit_thresh_ && ack > eln_last_rxmit_) {
                /* Retransmit this packet */
                output(last_ack_ + 1, TCP_REASON_DUPACK);
                eln_last_rxmit_ = last_ack_+1;
        } else
                send_much(0, 0, maxburst_);

        Packet::free(pkt);
        return;
}

/*
 * This function is invoked when the connection is done. It in turn
 * invokes the Tcl finish procedure that was registered with TCP.
 */
void TcpAgent::finish()
{
	Tcl::instance().evalf("%s done", this->name());
}

void FlowBenderTimer::expire(Event*)
{
	a_->timeout(TCP_FB_TIMER);
	//a_->checkCongestionFB();
}

void RtxTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_RTX);
}

void DelSndTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_DELSND);
}

void BurstSndTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_BURSTSND);
}

/*
 * THE FOLLOWING FUNCTIONS ARE OBSOLETE, but REMAIN HERE
 * DUE TO OTHER PEOPLE's TCPs THAT MIGHT USE THEM
 *
 * These functions are now replaced by ecn() and slowdown(),
 * respectively.
 */

/*
 * Respond either to a source quench or to a congestion indication bit.
 * This is done at most once a roundtrip time;  after a source quench,
 * another one will not be done until the last packet transmitted before
 * the previous source quench has been ACKed.
 */
void TcpAgent::quench(int how)
{
	if (highest_ack_ >= recover_) {
		recover_ =  maxseq_;
		last_cwnd_action_ = CWND_ACTION_ECN;
		closecwnd(how);
	}
}

/*
 * close down the congestion window
 */
void TcpAgent::closecwnd(int how)
{   
	static int first_time = 1;
	if (first_time == 1) {
		fprintf(stderr, "the TcpAgent::closecwnd() function is now deprecated, please use the function slowdown() instead\n");
	}
	switch (how) {
	case 0:
		/* timeouts */
		ssthresh_ = int( window() / 2 );
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		cwnd_ = int(wnd_restart_);
		break;

	case 1:
		/* Reno dup acks, or after a recent congestion indication. */
		// cwnd_ = window()/2;
		cwnd_ = decrease_num_ * window();
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;		
		break;

	case 2:
		/* Tahoe dup acks  		
		 * after a recent congestion indication */
		cwnd_ = wnd_init_;
		break;

	case 3:
		/* Retransmit timeout, but no outstanding data. */ 
		cwnd_ = int(wnd_init_);
		break;
	case 4:
		/* Tahoe dup acks */
		ssthresh_ = int( window() / 2 );
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		cwnd_ = 1;
		break;

	default:
		abort();
	}
	fcnt_ = 0.;
	count_ = 0;
}

/*
 * Check if the sender has been idle or application-limited for more
 * than an RTO, and if so, reduce the congestion window.
 */
void TcpAgent::process_qoption_after_send ()
{
	int tcp_now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);
	int rto = (int)(t_rtxcur_/tcp_tick_) ; 
	/*double ct = Scheduler::instance().clock();*/

	if (!EnblRTTCtr_) {
		if (tcp_now - T_last >= rto) {
			// The sender has been idle.
		 	slowdown(THREE_QUARTER_SSTHRESH|TCP_IDLE) ;
			for (int i = 0 ; i < (tcp_now - T_last)/rto; i ++) {
				slowdown(CWND_HALF_WITH_MIN|TCP_IDLE);
			}
			T_prev = tcp_now ;
			W_used = 0 ;
		}
		T_last = tcp_now ;
		if (t_seqno_ == highest_ack_+ window()) {
			T_prev = tcp_now ; 
			W_used = 0 ; 
		}
		else if (t_seqno_ == curseq_-1) {
			// The sender has no more data to send.
			int tmp = t_seqno_ - highest_ack_ ;
			if (tmp > W_used)
				W_used = tmp ;
			if (tcp_now - T_prev >= rto) {
				// The sender has been application-limited.
				slowdown(THREE_QUARTER_SSTHRESH|TCP_IDLE);
				slowdown(CLOSE_CWND_HALF_WAY|TCP_IDLE);
				T_prev = tcp_now ;
				W_used = 0 ;
			}
		}
	} else {
		rtt_counting();
	}
}

/*
 * Check if the sender has been idle or application-limited for more
 * than an RTO, and if so, reduce the congestion window, for a TCP sender
 * that "counts RTTs" by estimating the number of RTTs that fit into
 * a single clock tick.
 */
void
TcpAgent::rtt_counting()
{
        int tcp_now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);
	int rtt = (int(t_srtt_) >> T_SRTT_BITS) ;

	if (rtt < 1) 
		rtt = 1 ;
	if (tcp_now - T_last >= 2*rtt) {
		// The sender has been idle.
		int RTTs ; 
		RTTs = (tcp_now -T_last)*RTT_goodcount/(rtt*2) ; 
		RTTs = RTTs - Backoffs ; 
		Backoffs = 0 ; 
		if (RTTs > 0) {
			slowdown(THREE_QUARTER_SSTHRESH|TCP_IDLE) ;
			for (int i = 0 ; i < RTTs ; i ++) {
				slowdown(CWND_HALF_WITH_MIN|TCP_IDLE);
				RTT_prev = RTT_count ; 
				W_used = 0 ;
			}
		}
	}
	T_last = tcp_now ;
	if (tcp_now - T_start >= 2*rtt) {
		if ((RTT_count > RTT_goodcount) || (F_full == 1)) {
			RTT_goodcount = RTT_count ; 
			if (RTT_goodcount < 1) RTT_goodcount = 1 ; 
		}
		RTT_prev = RTT_prev - RTT_count ;
		RTT_count = 0 ; 
		T_start  = tcp_now ;
		F_full = 0;
	}
	if (t_seqno_ == highest_ack_ + window()) {
		W_used = 0 ; 
		F_full = 1 ; 
		RTT_prev = RTT_count ;
	}
	else if (t_seqno_ == curseq_-1) {
		// The sender has no more data to send.
		int tmp = t_seqno_ - highest_ack_ ;
		if (tmp > W_used)
			W_used = tmp ;
		if (RTT_count - RTT_prev >= 2) {
			// The sender has been application-limited.
			slowdown(THREE_QUARTER_SSTHRESH|TCP_IDLE) ;
			slowdown(CLOSE_CWND_HALF_WAY|TCP_IDLE);
			RTT_prev = RTT_count ; 
			Backoffs ++ ; 
			W_used = 0;
		}
	}
	if (F_counting == 0) {
		W_timed = t_seqno_  ;
		F_counting = 1 ;
	}
}

void TcpAgent::process_qoption_after_ack (int seqno)
{
	if (F_counting == 1) {
		if (seqno >= W_timed) {
			RTT_count ++ ; 
			F_counting = 0 ; 
		}
		else {
			if (dupacks_ == numdupacks_)
				RTT_count ++ ;
		}
	}
}

void TcpAgent::trace_event(char *eventtype)
{
	if (et_ == NULL) return;
	int seqno = t_seqno_;
	char *wrk = et_->buffer();
	char *nwrk = et_->nbuffer();
	if (wrk != 0)
		sprintf(wrk,
			"E "TIME_FORMAT" %d %d TCP %s %d %d %d",
			et_->round(Scheduler::instance().clock()),   // time
			addr(),                       // owner (src) node id
			daddr(),                      // dst node id
			eventtype,                    // event type
			fid_,                         // flow-id
			seqno,                        // current seqno
			int(cwnd_)                         //cong. window
			);
	
	if (nwrk != 0)
		sprintf(nwrk,
			"E -t "TIME_FORMAT" -o TCP -e %s -s %d.%d -d %d.%d",
			et_->round(Scheduler::instance().clock()),   // time
			eventtype,                    // event type
			addr(),                       // owner (src) node id
			port(),                       // owner (src) port id
			daddr(),                      // dst node id
			dport()                       // dst port id
			);
	et_->trace();
}
