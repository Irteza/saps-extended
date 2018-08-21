/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990, 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* 8/02 Tom Kelly - Made scoreboard a general interface to allow
 *                  easy swapping of scoreboard algorithms.  
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "scoreboard.h"
#include "scoreboard-rq.h"
#include "random.h"

#define TRUE    1
#define FALSE   0
#define RECOVER_DUPACK  1
#define RECOVER_TIMEOUT 2
#define RECOVER_QUENCH  3

#define debug_dctcp 0

class Sack1TcpAgent : public TcpAgent {
 public:
	Sack1TcpAgent();
	virtual ~Sack1TcpAgent();
	virtual void recv(Packet *pkt, Handler*);
	int is_sacked(hdr_tcp *tcph, int seqlo, int seqhi);
	void reset();
	virtual void timeout(int tno);
	virtual void dupack_action();
	virtual void partial_ack_action();
	void plot();
	virtual void send_much(int force, int reason, int maxburst);
 protected:
	u_char timeout_;	/* boolean: sent pkt from timeout? */
	u_char fastrecov_;	/* boolean: doing fast recovery? */
	/* variables for DCTCP */
	int dctcp_total;
	int dctcp_marked;
	int dctcp_alpha_update_seq;
//	int dctcp_maxseq;
	int ce_transition;

	int pipe_;		/* estimate of pipe size (fast recovery) */ 
	int partial_ack_;	/* Set to "true" to ensure sending */
				/*  a packet on a partial ACK.     */
	int next_pkt_;		/* Next packet to transmit during Fast */
				/*  Retransmit as a result of a partial ack. */
	int firstpartial_;	/* First of a series of partial acks. */
	ScoreBoard* scb_;
	static const int SBSIZE=64; /* Initial scoreboard size */
	void update_dctcp_alpha(Packet *); /* DCTCP alpha update */
	void checkCongestionFB(); // FlowBender -- check the congestion level of the used path after each RTT
};

static class Sack1TcpClass : public TclClass {
public:
	Sack1TcpClass() : TclClass("Agent/TCP/Sack1") {}
	TclObject* create(int, const char*const*) {		
		return (new Sack1TcpAgent());
	}
} class_sack;

Sack1TcpAgent::Sack1TcpAgent() : fastrecov_(FALSE), pipe_(-1), next_pkt_(0), firstpartial_(0),
	        dctcp_total(0), dctcp_marked(0), dctcp_alpha_update_seq(0), ce_transition(0)
{
	bind_bool("partial_ack_", &partial_ack_);
	/* Use the Reassembly Queue based scoreboard as
	 * ScoreBoard is O(cwnd) which is bad for HSTCP
	 * scb_ = new ScoreBoard(new ScoreBoardNode[SBSIZE],SBSIZE);
	 */
	scb_ = new ScoreBoardRQ();
}

Sack1TcpAgent::~Sack1TcpAgent(){
	delete scb_;
}

void Sack1TcpAgent::reset ()
{
	scb_->ClearScoreBoard();
	TcpAgent::reset ();
}


void Sack1TcpAgent::recv(Packet *pkt, Handler*)
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

	dupCase1Count_ = hdr_flags::access(pkt)->dupCase1Count(); /* SMI: 15-June-2015 */
	dupCase2Count_ = hdr_flags::access(pkt)->dupCase2Count(); /* SMI: 15-June-2015 */

	/* FlowBender Stats Update Code -- SMI 6-Jul-2017 */
	// if(flowBender_) {
	// 	printf("Servicing ACK pkt: totalAckPkts=%d nackpack_=%d \n", totalAckPkts, (int) nackpack_);
	// 	totalAckPkts++;
	// }
	
	/* ali munir */
	if (dctcp_) 
		update_dctcp_alpha(pkt);

	int ecnecho = hdr_flags::access(pkt)->ecnecho();
	if (ecnecho && ecn_) {
		ecn(tcph->seqno());
		//if(flowBender_) markedAckPkts_F++; // added by SMI 6-Jul-17 for FlowBender
	}
        if (tcph->seqno() >= last_ack_)
		// Check if ACK is valid.  Suggestion by Mark Allman.
		valid_ack = 1;

	/* begin: ali munir 
	hdr_flags *fh = hdr_flags::access(pkt);
  	    	if (ecn_) {
 			if (dctcp_) { // DCTCP	       
 				if (fh->ce() && fh->ect()) {
 					// no CWR from peer yet... arrange to
 					// keep sending ECNECHO
 					if (recent_ce_ == FALSE) {
 						ce_transition = 1;
 						recent_ce_ = TRUE;
 					} else {
 						ce_transition = 0;
 					}
 				} else if (datalen > 0 && !fh->ce() && fh->ect()){
 					if (recent_ce_ == TRUE) {
 						ce_transition = 1;
 						recent_ce_ = FALSE;
 					} else {
 						ce_transition = 0;
 					}
 				}  
 			}

	/* END: ali munir */


	/*
	 * If DSACK is being used, check for DSACK blocks here.
	 * Possibilities:  Check for unnecessary Fast Retransmits.
	 */
	if (!fastrecov_) {
		/* normal... not fast recovery */
		if ((int)tcph->seqno() > last_ack_) {
			/*
			 * regular ACK not in fast recovery... normal
			 */
			firstpartial_ = 0;
			recv_newack_helper(pkt);
			timeout_ = FALSE;
			scb_->ClearScoreBoard();
			if (last_ack_ == 0 && delay_growth_) {
				cwnd_ = initial_window();
			}
		} else if ((int)tcph->seqno() < last_ack_) {
			/*NOTHING*/
		} else if (timeout_ == FALSE) {
			if (tcph->seqno() != last_ack_) {
				fprintf(stderr, "pkt seq %d should be %d\n" ,
					tcph->seqno(), last_ack_);
				abort();
			}
			scb_->UpdateScoreBoard (highest_ack_, tcph);
			/*
		 	 * Check for a duplicate ACK.
			 * Check that the SACK block actually
			 *  acknowledges new data.
 			 */

		/* Begin: Ali Munir: Is it at the right place  */
			/* DCTCP receiver state machine 
		        if (dctcp_ && ce_transition && ((rcv_nxt_ - last_ack_) > 0)) {
				// Must send an immediate ACK with with previous ECN state 
				// before transitioning to new state
				flags_ |= 1;
				recent_ce_ = !recent_ce_;		
				send_much(1, REASON_NORMAL, maxburst_);
				recent_ce_ = !recent_ce_;			  
			}
		/* End: Ali Munir */


 		        if(scb_->CheckUpdate()) {
 			 	if (++dupacks_ == numdupacks(cwnd_)) {
 					/*
 					 * Assume we dropped just one packet.
 					 * Retransmit last ack + 1
 					 * and try to resume the sequence.
 					 */
 				   	dupack_action();
 				} else if (dupacks_ < numdupacks(cwnd_) && singledup_ ) {
					send_one();
 				}
			}
			if (sfrto_enabled_ && frto_ == 2) {
				/*
				 * SACK-based F-RTO: If SACK only acknowledges
				 * data that was transmitted before RTO and
				 * not acknowledged earlier,
				 * the timeout was spurious.
				 */
				if (scb_->IsChanged() &&
				    !is_sacked(tcph, recover_, maxseq_)) {
					spurious_timeout();
				} else {
					t_seqno_ = highest_ack_ + 1;
					cwnd_ = frto_;
					frto_ = 0;
					dupacks_ = 0;
				}
			}
		}
        	if (valid_ack || aggressive_maxburst_)
			if (dupacks_ == 0)
				send_much(FALSE, 0, maxburst_);
	} else {
		/* we are in fast recovery */
		--pipe_;
		if ((int)tcph->seqno() >= recover_) {
			/* ACK indicates fast recovery is over */
			recover_ = 0;
			fastrecov_ = FALSE;
			newack(pkt);
			/* if the connection is done, call finish() */
			if ((highest_ack_ >= curseq_-1) && !closed_) {
				closed_ = 1;
				finish();
			}
			timeout_ = FALSE;
			scb_->ClearScoreBoard();

			/* New window: W/2 - K or W/2? */
		} else if ((int)tcph->seqno() > highest_ack_) {
			/* Not out of fast recovery yet.
			 * Update highest_ack_, but not last_ack_. */
			highest_ack_ = (int)tcph->seqno();
			scb_->UpdateScoreBoard (highest_ack_, tcph);
			if (partial_ack_) {
			  /* partial_ack_ is needed to guarantee that */
			  /*  a new packet is sent in response to a   */
			  /*  partial ack.                            */
				partial_ack_action();
				++pipe_;
				if (firstpartial_ == 0) {
					newtimer(pkt);
					t_backoff_ = 1;
					firstpartial_ = 1;
				}
			} else {
				--pipe_;
				newtimer(pkt);
				t_backoff_ = 1;
 			 /* If this partial ACK is from a retransmitted pkt,*/
 			 /* then we decrement pipe_ again, so that we never */
 			 /* do worse than slow-start.  If this partial ACK  */
 			 /* was instead from the original packet, reordered,*/
 			 /* then this might be too aggressive. */
			}
		} else if (timeout_ == FALSE) {
			/* got another dup ack */
			scb_->UpdateScoreBoard (highest_ack_, tcph);
 		        if(scb_->CheckUpdate()) {
 				if (dupacks_ > 0)
 			        	dupacks_++;
 			}
		}
        	if (valid_ack || aggressive_maxburst_)
			send_much(FALSE, 0, maxburst_);
	}

	Packet::free(pkt);
#ifdef notyet
	if (trace_)
		plot();
#endif
}


/*
 * Returns TRUE if any of the SACK blocks in the current packet (tcph)
 * cover sequence numbers between seqlo and seqhi
 */
int Sack1TcpAgent::is_sacked(hdr_tcp *tcph, int seqlo, int seqhi)
{
	int i, sleft, sright;
	for (i=0; i < tcph->sa_length(); i++) {
		sleft = tcph->sa_left(i);
		sright = tcph->sa_right(i);

		if ((sright > seqlo && sright <= seqhi) ||
		    (sleft >= seqlo && sleft < seqhi) ||
		    (sleft < seqlo && sright > seqhi)) {
			return TRUE;
		}
	}
	return FALSE;
}


void
Sack1TcpAgent::dupack_action()
{
	int recovered = (highest_ack_ > recover_);
	if (recovered || (!bug_fix_ && !ecn_) || 
		(bugfix_ss_ && highest_ack_ == 0)) {
                // (highest_ack_ == 0) added to allow Fast Retransmit
                //  when the first data packet is dropped.
                //  Bug report from Mark Allman.
		goto sack_action;
	}
 
	if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		last_cwnd_action_ = CWND_ACTION_DUPACK;

		/* ali  munir: cut window by half when we have 3 dup ack */
		if (dctcp_) 
			slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF); 


		/*
		 * What if there is a DUPACK action followed closely by ECN
		 * followed closely by a DUPACK action?
		 * The optimal thing to do would be to remember all
		 * congestion actions from the most recent window
		 * of data.  Otherwise "bugfix" might not prevent
		 * all unnecessary Fast Retransmits.
		 */
		reset_rtx_timer(1,0);
		/*
		 * There are three possibilities: 
		 * (1) pipe_ = int(cwnd_) - numdupacks_;
		 * (2) pipe_ = window() - numdupacks_;
		 * (3) pipe_ = maxseq_ - highest_ack_ - numdupacks_;
		 * equation (2) takes into account the receiver's
		 * advertised window, and equation (3) takes into
		 * account a data-limited sender.
		 */
		if (singledup_ && LimTransmitFix_) {
		  pipe_ = maxseq_ - highest_ack_ - 1;
		}
		else {
		  // numdupacks(cwnd_) packets have left the pipe
		  pipe_ = maxseq_ - highest_ack_ - numdupacks(cwnd_);
		}
		fastrecov_ = TRUE;
		scb_->MarkRetran(highest_ack_+1);
		output(last_ack_ + 1, TCP_REASON_DUPACK);
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

sack_action:
	recover_ = maxseq_;
	if (oldCode_) {
	 	pipe_ = int(cwnd_) - numdupacks(cwnd_);
	} else if (singledup_ && LimTransmitFix_) {
		  pipe_ = maxseq_ - highest_ack_ - 1;
	}
	else {
		  // numdupacks(cwnd_) packets have left the pipe
		  pipe_ = maxseq_ - highest_ack_ - numdupacks(cwnd_);
	}
	reset_rtx_timer(1,0);
	fastrecov_ = TRUE;
	scb_->MarkRetran(highest_ack_+1);
        if (!lossQuickStart()) {
                // we are now going into fast_recovery and will trace that event
                trace_event("FAST_RECOVERY");
                last_cwnd_action_ = CWND_ACTION_DUPACK;
                slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
		output(last_ack_ + 1, TCP_REASON_DUPACK);	// from top
		/*
		 * If dynamically adjusting numdupacks_, record information
		 *  at this point.
		 */
	}
	return;
}

/*
 * Process a packet that acks previously unacknowleges data, but
 * does not take us out of Fast Retransmit.
 *
 * The need for a mechanism to ensure that Sack TCP sends a packet in
 * response to a partial ACK has been discussed in
 * "Challenges to Reliable Data Transport over Heterogeneous
 * Wireless Networks", Hari Balakrishnan, 1998, and in
 * "Responding to Spurious Timeouts in TCP", Andrei Gurtov and 
 * Reiner Ludwig, 2003. 
 */
void
Sack1TcpAgent::partial_ack_action()
{
	if (next_pkt_ < highest_ack_ + 1) {
		next_pkt_ = highest_ack_ + 1;
	}
	// Output two packets in response to a partial ack,
	//   so as not to do worse than slow-start.
	int i;
	for (i = 1; i<=2; i++) {
		int getNext = scb_->GetNextUnacked(next_pkt_);
		if (getNext > next_pkt_) {
			next_pkt_ = getNext;
		}
		if (t_seqno_ < next_pkt_) {
			t_seqno_ = next_pkt_;
		}
		output(next_pkt_, TCP_REASON_PARTIALACK);	
		scb_->MarkRetran(next_pkt_);
		++next_pkt_; 
	}
	return;
}

void Sack1TcpAgent::timeout(int tno)
{
	// F-RTO is not allowed if earlier SACK fast recovery is underway.
	int no_frto = fastrecov_;

	if (tno == TCP_TIMER_RTX) {
		/*
		 * IF DSACK and dynamic adjustment of numdupacks_,
		 *  check whether a smaller value of numdupacks_
		 *  would have prevented this retransmit timeout.
		 * If DSACK and detection of premature retransmit
		 *  timeouts, then save some info here.
		 */ 
		/* begin: ali munir */
	//	dctcp_alpha_update_seq = tno; /* should be t_seqno_ */
		dctcp_alpha_update_seq = t_seqno_;
		dctcp_maxseq = dctcp_alpha_update_seq;

		/* end: ali munir */

		/* FlowBender reroutes when RTO occurs, assuming it may be due to a failure */
		if(flowBender_) {
			++defttl_; //do rerouting
			numRTTsCongested_ = 0; // SMI-Commented-jul25
			totalAckPkts = 0; // SMI-Commented-jul25
			markedAckPkts_F = 0; // SMI-Commented-jul25
			fl_bndr_timer.resched(rtt_timeout()); // * tcp_tick_ // SMI-Commented-jul25
		}

		dupacks_ = 0;
		fastrecov_ = FALSE;
		timeout_ = TRUE;
		if (highest_ack_ > last_ack_)
			last_ack_ = highest_ack_;
#ifdef DEBUGSACK1A
		printf ("timeout. highest_ack: %i seqno: %i fid: %i\n", 
			(int)highest_ack_, (int)t_seqno_, fid_);
#endif
		recover_ = maxseq_;
		scb_->ClearScoreBoard();
	} else {
		if(tno == TCP_FB_TIMER) {
			/* FlowBender Timer */ 
			if(debug_dctcp) {
				printf("Calling checkCongestionFB()! \n");
			}
			checkCongestionFB(); // SMI-Commented-jul25
		}
	}
	TcpAgent::timeout(tno);

	// Overrule frto_ setting that may have been done in TcpAgent
	if (no_frto)
		frto_ = 0;
}

/**
   This function is called after every RTT for FlowBender. Check to see if for N consecutive RTTs, the fraction F of marked packets exceeds threshold T
   If so, reroute the flow to a new path. If not, do nothing. Reset all relevant variables, and reschedule the timer.
 **/
void Sack1TcpAgent::checkCongestionFB() {
	double now = Scheduler::instance().clock();
	if(debug_dctcp) {
		printf("checkCongestionFB -> Time now: %f markedAckPkts_F=%d totalAckPkts=%d flowBender_T=%f \n", now, markedAckPkts_F, totalAckPkts, flowBender_T);
	}

	double fractionMarked = 0.0;
	if(totalAckPkts > 0) {
		fractionMarked = ((double) markedAckPkts_F) /totalAckPkts;
	}

	if((totalAckPkts > 0) && (fractionMarked > flowBender_T)) {
		if(++numRTTsCongested_ >= flowBender_N) {
			//defttl_ = defttl_ + Random::integer(3) + 1;
			 defttl_ = defttl_ + 1; 	//toggleTTL_++; //do rerouting....!
			if(debug_dctcp) {
				printf("checkCongestionFB -> Rerouting Time now: %f markedAckPkts_F=%d totalAckPkts=%d defttl_=%d numRTTsCongested_=%d ", now, markedAckPkts_F, totalAckPkts, defttl_, numRTTsCongested_);
				printf("ndatapack_=%d \n", (int) ndatapack_);
			}
			numRTTsCongested_ = 0;
		}
	} else {
		numRTTsCongested_ = 0;
	}
	// reset variables...
	totalAckPkts = 0;
	markedAckPkts_F = 0;

	// reschedule the FB timer
	fl_bndr_timer.resched(rtt_timeout()); // * tcp_tick_ // SMI-Commented-jul25
}

/*
 * Update dctcp alpha based on the ecn bit in the received packet.
 * This procedure is called only when dctcp_ is 1.
 */
void Sack1TcpAgent::update_dctcp_alpha(Packet *pkt)
{
	int ecnbit = hdr_flags::access(pkt)->ecnecho();
	int ackno = hdr_tcp::access(pkt)->ackno();
	int seqno = hdr_tcp::access(pkt)->seqno();
	int acked_bytes = seqno - highest_ack_; 

	if(debug_dctcp)
		printf ("timeout. highest_ack: %i seqno: %i fid: %i seqnum: %i\n", 
			(int)highest_ack_, (int)t_seqno_, fid_, seqno);

	if (acked_bytes <= 0) 
//		acked_bytes = size_;	/* it should be one */
		acked_bytes = 1;	/* it should be one */

	dctcp_total += acked_bytes;
	totalAckPkts += acked_bytes; // added SMI 19-jul // SMI-Commented-jul25

	if(debug_dctcp) {
		printf("dctcp total is = %d ; ECN bit is: %d \n", dctcp_total, ecnbit);
	}

	if (ecnbit) {
		dctcp_marked += acked_bytes;
		markedAckPkts_F += acked_bytes; // added SMI 19-jul // SMI-Commented-jul25
		//printf("FID: %i marked\n",fid_);
	}

	if(debug_dctcp)
		fprintf(stdout,"DCTCP update_alpha 1: (ackno=%i), (dctcp_alpha_update=%f), (dctcp_maxseq=%f), (acked_bytes=%d)\n", (int)ackno, (double)dctcp_alpha_update_seq, (double)dctcp_maxseq, acked_bytes);
	/* Check for barrier indicating its time to recalculate alpha.
	 * This code basically updated alpha roughly once per RTT.
	 */

	// if(flowBender_) {
	// 	checkCongestionFB(); // commented-jul26
	// }

	if (seqno > dctcp_alpha_update_seq) {
		if(debug_dctcp) {
			printf("seqno is greater than dctcp_alpha_update_seq!!!  \n");
		}
		double temp_alpha;
		dctcp_alpha_update_seq = dctcp_maxseq;
		//cout << "Entered the main if condition." << endl;
		if (dctcp_total > 0) {			
			temp_alpha = ((double) dctcp_marked) / dctcp_total;
			
			if(debug_dctcp) {
				printf("Dctcp marked is = %d \t", dctcp_marked);
				//printf("temp_alpha = %f \n", temp_alpha);
			}
		}
		else {
			temp_alpha = 0.0;
		}

		// commented-jul27
		// if(dctcp_total > 0 && flowBender_) {
		// 	if (temp_alpha > flowBender_T) {
		// 		if(++numRTTsCongested_ >= flowBender_N) {
		// 			++defttl_; //do rerouting....!
		// 			if(debug_dctcp) {
		// 				printf("Rerouting Time now: markedAckBytes_F=%d totalBytes=%d defTTL_=%d numRTTsCongested_=%d ", dctcp_marked, dctcp_total, defttl_, numRTTsCongested_);
		// 				printf("ndatapack_=%d \n", (int) ndatapack_);
		// 			}
		// 			numRTTsCongested_ = 0;
		// 		}
		// 	} else {
		// 		numRTTsCongested_ = 0;
		// 	}
		// }
	
		// if(flowBender_) {	
		// 	if(debug_dctcp) {
		// 		printf("The ttl value is: %d ; temp_alpha=%f; flowBender_T=%f; flowBender_N=%d; numRTTsCongested_=%d \n", defttl_, temp_alpha, flowBender_T, flowBender_N, numRTTsCongested_);
		// 	}
		// 	if (temp_alpha > flowBender_T) {
		// 		//if (temp_alpha > 0.05) {
		// 		if(debug_dctcp) {
		// 			printf("temp_alpha greater than flowBender_T!\n");
		// 		}
		// 		numRTTsCongested_++;
		// 		if(numRTTsCongested_ >= flowBender_N) {
		// 			defttl_ = defttl_ + 1;	//toggleTTL_++; //do rerouting....!
		// 			if(debug_dctcp) {
		// 				printf("Rerouting Time now: markedAckBytes_F=%d totalBytes=%d defTTL_=%d numRTTsCongested_=%d ", dctcp_marked, dctcp_total, defttl_, numRTTsCongested_);
		// 				printf("ndatapack_=%d \n", (int) ndatapack_);
		// 			}
		// 			numRTTsCongested_ = 0;
		// 		}
		// 	} else {
		// 		numRTTsCongested_ = 0;
		// 	}
		// 	//if(temp_alpha>0.8) { defttl_ = defttl_+1; }
		// }
	
		
//		temp_alpha = ((double) dctcp_marked) / 35.0;
		dctcp_alpha_ = (1 - dctcp_g_) * dctcp_alpha_ + dctcp_g_ * temp_alpha;
		dctcp_marked = 0;
		dctcp_total = 0;
	} else {
		if(debug_dctcp) {
			printf("seqno smaller: seqno=%d dctcp_alpha_update=%f \n!!",seqno , (double) dctcp_alpha_update_seq);
		}
	}


	if(debug_dctcp)
		fprintf(stdout,"DCTCP update_alpha: (dctcpseq=%d), (marked=%d), (total=%d), (alpha=%f) at time= %f\n", dctcp_maxseq, dctcp_marked, dctcp_total, (double)dctcp_alpha_, (double)Scheduler::instance().clock());
}



void Sack1TcpAgent::send_much(int force, int reason, int maxburst)
{
	register int found, npacket = 0;
	int win = window();
	int xmit_seqno;

	found = 1;
	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	/*
	 * as long as the pipe is open and there is app data to send...
	 */
	while (((!fastrecov_ && (t_seqno_ <= last_ack_ + win)) ||
			(fastrecov_ && (pipe_ < int(cwnd_)))) 
			// && t_seqno_ < curseq_ && found) {
			&& (last_ack_ + 1) < curseq_ && found) {

		if (overhead_ == 0 || force) {
			found = 0;
			xmit_seqno = scb_->GetNextRetran ();

#ifdef DEBUGSACK1A
			printf("highest_ack: %d xmit_seqno: %d\n", 
			(int)highest_ack_, xmit_seqno);
#endif
			if (xmit_seqno == -1) { 
				if ((!fastrecov_ && t_seqno_<=highest_ack_+win)||
					(fastrecov_ && t_seqno_<=highest_ack_+int(wnd_))) {
					if (t_seqno_ < curseq_) {
						found = 1;
						xmit_seqno = t_seqno_++;
					}
#ifdef DEBUGSACK1A
					printf("sending %d fastrecovery: %d win %d\n",
						xmit_seqno, fastrecov_, win);
#endif
				}
			} else if (recover_>0 && xmit_seqno<=highest_ack_+int(wnd_)) {
				found = 1;
				scb_->MarkRetran (xmit_seqno);
				win = window();
			}
			if (found) {
				output(xmit_seqno, reason);
				if (t_seqno_ <= xmit_seqno)
					t_seqno_ = xmit_seqno + 1;
				npacket++;
				pipe_++;
				if (QOption_)
					process_qoption_after_send () ;
	                        if (qs_approved_ == 1) {
	                                double delay = (double) t_rtt_ * tcp_tick_ / cwnd_;
	                                delsnd_timer_.resched(delay);
	                                return;
	                        }
			}
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 * This is only for the simulator,to add some
			 *   randomization if speficied.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if (maxburst && npacket == maxburst)
			break;
	} /* while */
}

void Sack1TcpAgent::plot()
{
#ifdef notyet
	double t = Scheduler::instance().clock();
	sprintf(trace_->buffer(), "t %g %d rtt %g\n", 
		t, class_, t_rtt_ * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d dev %g\n", 
		t, class_, t_rttvar_ * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d win %f\n", t, class_, cwnd_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d bck %d\n", t, class_, t_backoff_);
	trace_->dump();
#endif
}
