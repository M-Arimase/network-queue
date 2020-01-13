/*
 * QCluster
 */

#include "ClusterQueueMap.h"
#include "flags.h"
#include "math.h"

#define max(arg1, arg2) ((arg1) > (arg2) ? (arg1) : (arg2))
#define min(arg1, arg2) ((arg1) < (arg2) ? (arg1) : (arg2))

static class ClusterQueueMapClass : public TclClass{
public:
	ClusterQueueMapClass():TclClass("Queue/ClusterQueueMap"){}
	TclObject* create(int, const char*const*){
		return (new ClusterQueueMap);
	}
} class_clusterqueuemap;

void ClusterQueueMap::enque(Packet* p)
{
// fprintf(stderr, "stage-0\n");
	hdr_ip *iph = hdr_ip::access(p);
	uint flow_id = iph->flowid();
	int size = hdr_cmn::access(p)->size() - 40;
	packet_t type = hdr_cmn::access(p)->ptype();
	hdr_flags* hf = hdr_flags::access(p);
	int qlimBytes = qlim_ * mean_pktsize_;

	int message_now = iph->connection();
	double queue_now = Scheduler::instance().clock();

	// 1 <= queue_num <= MAX_QUEUE_NUM
	queue_num_ = max(min(queue_num_, MAX_QUEUE_NUM), 1);

	// queue length exceeds the queue limit
	if(TotalByteLength() + size + 40 > qlimBytes){
		drop(p);
		return;
	}
// fprintf(stderr, "stage-1\n");
	int queue_chose = 0;
	double value = size;
	int queue_recommend = queue_num_ - 1;

	if(type == PT_TCP && size > 0){
		if(last_message_time.find(flow_id) == last_message_time.end()
			|| message_now != last_message_time[flow_id])
		{
			count_item[flow_id] = 0;
			last_en_qid[flow_id] = 0;
			queue_recommend = 0;
		}

		if(size >= 1460){
			count_item[flow_id] += size;
			value = count_item[flow_id];

			if(queue_recommend != 0){
				if(queue_now - last_queue_time[flow_id] > interval_qlet_)
					queue_recommend = max(last_en_qid[flow_id] - 1, 0);
				else
					queue_recommend = last_en_qid[flow_id];
			}
		}
		double thresholds[queue_num_ - 1] = {0};
		for(int i = 0; i < queue_num_ - 1; ++i){
			if(info[i].distinct < 1){
				if(i == 0)
					thresholds[i] = (1 << 25);
				else thresholds[i] = (1 << 25) + thresholds[i - 1];
				continue;
			}
			thresholds[i] = max(info[i].average(),
				sqrt(info[i].average() * info[i + 1].average()));

			if(i > 0 && thresholds[i] <= thresholds[i - 1])
				thresholds[i] = thresholds[i - 1] + 1460;
		}
		for(queue_chose = 0; queue_chose < queue_num_ - 1; ++queue_chose)
			if(value <= thresholds[queue_chose] + 1)
				break;
		queue_chose = max(queue_chose, queue_recommend);
	}
// fprintf(stderr, "stage-2\n");
	// enqueue packet
	q_[queue_chose]->enque(p);
	// fprintf(stderr, "q_[%d].size()=%d\n", queue_chose, q_[queue_chose]->length());

	if(type == PT_TCP && size > 0)
	{
		// fprintf(stderr, "updating queue\n");
		last_en_qid[flow_id] = queue_chose;
		last_queue_time[flow_id] = queue_now;
		last_message_time[flow_id] = message_now;

		info[queue_chose].distinct += 1;
		info[queue_chose].counting += value;
		if(Scheduler::instance().clock() - last_update > 1)
		{
			last_update = Scheduler::instance().clock();
			Update();
		}
		// fprintf(stderr, "end queue\n");
	}
// fprintf(stderr, "stage-3\n");
	// enqueue ECN marking: per-queue or per-port
	if((marking_scheme_ == PER_QUEUE_ECN && 
		q_[queue_chose]->byteLength() > thresh_ * mean_pktsize_) ||
		(marking_scheme_ == PER_PORT_ECN &&
		TotalByteLength() > thresh_ * mean_pktsize_))
	{
		if(hf->ect())		// if this packet is ECN-capable
			hf->ce() = 1;
	}
}

Packet* ClusterQueueMap::deque()
{
	if(TotalByteLength() > 0)
	{
		// high->low: 0-7
		for(int i = 0; i < queue_num_; ++i)
			if(q_[i]->length() > 0){
				Packet *p = q_[i]->deque();
				return (p);
			}
	}
	return NULL;
}
