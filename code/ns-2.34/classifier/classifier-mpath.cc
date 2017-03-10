/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: classifier-mpath.cc,v 1.10 2005/08/25 18:58:01 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-mpath.cc,v 1.10 2005/08/25 18:58:01 johnh Exp $ (USC/ISI)";
#endif

#include "classifier.h"
#include "ip.h"
#include "flags.h"
#include "tcp.h"
#include "packet.h"

//#define debug_mpath_smi 1


class MultiPathForwarder : public Classifier {
public:
 	MultiPathForwarder() : ns_(0), nodeid_(0), nodetype_(0), perflow_(0), packetSpraying_(0), flowcellSpraying_(0), selectiveSpraying_(0), K_(0), failureCase_(0) { // checkpathid_(0)
		bind("nodeid_", &nodeid_); 
		bind("nodetype_", &nodetype_);
		bind("perflow_", &perflow_);
		//bind("checkpathid_", &checkpathid_);
		bind("packetSpraying_", &packetSpraying_); // SMI 26-March
		bind("flowcellSpraying_", &flowcellSpraying_); // SMI 12-Dec-2015
		bind("selectiveSpraying_", &selectiveSpraying_); // SMI 8-Mar-16
		bind("K_", &K_); // SMI 8-May
		bind("failureCase_", &failureCase_); // SMI 30-jul-16
	}

	/* Now have to do 
		(1) Add the checking for whether isElephant is set in fh header
		(2) Where are we setting the perflow_ variable, is it being set correctly??? i.e. ECMP flag thingy 
	*/

	virtual int classify(Packet* p) {
		int cl;
		hdr_tcp* tcph = hdr_tcp::access(p);
		hdr_ip* h = hdr_ip::access(p);
		hdr_flags* fh = hdr_flags::access(p);

		hdr_cmn* ch = hdr_cmn::access(p);

		//printf("Debug Message in MPath Classifier..!! \n");

		if(perflow_ || (packetSpraying_==0 && fh->isElephant()==0)) { // || checkpathid_
#ifdef debug_mpath_smi
				printf("Normal ECMP Code running !!\n ");
#endif

			// do ECMP
			struct hkey {
				//int nodeid;
				int32_t sport, dport;
	       			nsaddr_t src, dst;
				int fid;
				int ttl;
			};
			struct hkey buf_;		
			///buf_.nodeid = nodeid_;
			buf_.src = mshift(h->saddr());
			buf_.dst = mshift(h->daddr());
			buf_.sport = h->sport();
			buf_.dport = h->dport();
			buf_.fid = h->flowid();
			buf_.ttl = h->ttl();				//OAJ: Added on 2015-07-03

			char* bufString = (char*) &buf_;
			int length = sizeof(hkey);

			unsigned int ms_ = (unsigned int) HashString(bufString, length); // commented 8-May
			//int ms_ = (int) HashString(bufString, length);


			//// if (checkpathid_) { }
			if(K_ == 0) { // this is the case of leaf-spine
				ms_ %= (maxslot_ + 1);
			} else { // this is the case of fat-tree
				if(fh->visitedToR() == 1) { // at the Agg Switch
					ms_ = fh->aggDecision();
					fh->visitedToR_ = 0;
				} else { // at the ToR Switch
					//if(ms_ < 0) {
					//	printf("at ToR: negative value for ms_ noticed!\n");
					//	ms_ = ms_ * -1;// added 8-may SMI
					//}
					
					ms_ %= (K_);
					fh->aggDecision_ = ms_ / (maxslot_ + 1);
					ms_ %= (maxslot_ + 1);
					fh->visitedToR_ = 1;
				}
			}
			
			int fail = ms_;
			do {
				cl = ms_++;
				ms_ %= (maxslot_ + 1);
			} while (slot_[cl] == 0 && ms_ != fail);

		//} else if(packetSpraying_ || fh->isElephant()) {
		} else if(packetSpraying_) { // do PS
			//printf("DEBUG: nodeid=%d seqno=%d linkID=%d ndatapack_=%d\n",nodeid_,tcph->seqno_,tcph->linkID,tcph->pkts_sent_so_far_);	
#ifdef debug_mpath_smi
			if(nodeid_==0) {
				printf("PktType=%s fh->failureDetected=%d at = %f \t", packet_info.name(ch->ptype()) , fh->failureDetected(),  (float) Scheduler::instance().clock());
			}
#endif

			if(selectiveSpraying_ && fh->failureDetected()) {
#ifdef debug_mpath_smi
				if(nodeid_==0) {
					printf("SelectiveSpraying==TRUE && Failure is Detected: linkID=%d !!\n ", tcph->linkID);
				}
#endif
				//int fail = tcph->linkID - 1;
				//if(fail < 0) fail += (maxslot_ + 1);
				ns_ = tcph->linkID; // ns_ = tcph->linkID % (maxslot_ + 1);
				cl = ns_;

				//while(slot_[cl]==0 && ns_ !=fail) {
				//cl = ns_++;
				//ns_ %= (maxslot_ + 1);
				//}

			} else {
				int fail = ns_;
				do {
					cl = ns_++;
					ns_ %= (maxslot_ + 1);
				} while (slot_[cl] == 0 && ns_ != fail);
#ifdef debug_mpath_smi
				if(nodeid_==0) {
					printf("Normal RPS Code running at Node %d: Slot Chosen=%d !!\n ", nodeid_, cl);
				}
#endif
			}
		} else if(flowcellSpraying_) {
			// WFCS, WFCS-P, UFCS, WPS 

			if(failureCase_==2) {
				//printf("failureCase_ is Full Failure... \n");
				ns_ = tcph->linkID;
				int fail = ns_;
				cl = ns_++;
				while (slot_[cl] == 0 && ns_ != fail) {
			        	cl = ns_++;
                                	ns_ %= (maxslot_ + 1);
				}
			} else {
				ns_ = tcph->linkID; // ns_ = tcph->linkID % (maxslot_ + 1);
				cl = ns_;
			}

			/*if (nodeid_ == 0){
				ns_ = tcph->linkID; // ns_ = tcph->linkID % (maxslot_ + 1);
				cl = ns_;
			} else {
				int fail = ns_;
				do {
			        	cl = ns_++;
                                	ns_ %= (maxslot_ + 1);

				} while (slot_[cl] == 0 && ns_ != fail);

			}*/
			
#ifdef debug_mpath_smi
			printf("Flowcell Spraying LB: tcph->linkID=%d, cl=%d, SeqNo=%d, FlowcellNum=%d, ", tcph->linkID, cl, tcph->seqno(), fh->flowcellSeq());
			printf("FlowID=%d \n", h->flowid());
#endif

		} else {
			// Does such a case exist, not ECMP, not PS, then what ??
#ifdef debug_mpath_smi
			printf("No valid LB: PS=%d, ECMP=%d, isElephant=%d", packetSpraying_, perflow_, fh->isElephant());
#endif
		}

		return cl;
	}
private:
	int pkts_sent_so_far;
	int ns_;
	// Mohammad: adding support for perflow multipath
	int nodeid_;	
	int nodetype_;
	int perflow_;
	//int checkpathid_;
	int packetSpraying_;
	int flowcellSpraying_;
	int selectiveSpraying_; // 8-Mar-16
	int K_; // if this is zero, we will assume it's not fat-tree topology
	int failureCase_; // 31 Jul 16 (SMI), used for full failure case specifically
	
	static unsigned int HashString(register const char *bytes,int length) {
		register unsigned int result = 0;
		register int i;

		for (i = 0;  i < length;  i++) {
			result += (result<<3) + *bytes++;
		}
		return result;
	}
};

static class MultiPathClass : public TclClass {
public:
	MultiPathClass() : TclClass("Classifier/MultiPath") {} 
	TclObject* create(int, const char*const*) {
		return (new MultiPathForwarder());
	}
} class_multipath;
