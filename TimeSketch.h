#ifndef _TIME_SKETCH_NEW_H_
#define _TIME_SKETCH_NEW_H_

#include "bobhash32.h"

#define max(arg1, arg2) ((arg1) > (arg2) ? (arg1) : (arg2))
#define min(arg1, arg2) ((arg1) < (arg2) ? (arg1) : (arg2))
static int __cnt = 0;

class TimeSketch{
private:
	const uint HASH_NUM;
	const uint LENGTH;
	double interval_msg_;		// threshold for spliting message
	double interval_qlet_;		// threshold for spliting queue-let
	uint MAX_QUEUE_NUM_;

	uint *counter;
	double *last_arrive_time;
	uint *last_arrive_queue;
public:
	TimeSketch(double interval_msg, double interval_qlet, uint _MAX_QUEUE_NUM, uint _HASH_NUM = 4, uint _LENGTH = 6401):
		interval_msg_(interval_msg), interval_qlet_(interval_qlet),
		MAX_QUEUE_NUM_(_MAX_QUEUE_NUM), HASH_NUM(_HASH_NUM), LENGTH(_LENGTH)
	{
		counter = new uint[LENGTH];
		last_arrive_time = new double[LENGTH];
		last_arrive_queue = new uint[LENGTH];
		memset(counter, 0, sizeof(uint) * LENGTH);
		memset(last_arrive_queue, 0, sizeof(uint) * LENGTH);
		for(int i = 0; i < LENGTH; ++i)
			last_arrive_time[i] = -1;
	}

	~TimeSketch(){
		delete[] counter;
		delete[] last_arrive_time;
		delete[] last_arrive_queue;
	}

	/* insert a packet into the sketch 
	 * return the last_arrive_queue
	 */
	uint Init(uint flow_id, double now)
	{
		bool ret = false;		// ret=true, if the flow_id is a new message
		bool flag = false;		// flag=true, if the flow_id is a new queue-let
		uint qid = 0;

		for(uint i = 0; i < HASH_NUM; ++i)
		{
			uint position = BOBHash32((uchar*)(&flow_id), sizeof(uint), i) % LENGTH;
			if(now - last_arrive_time[position] > interval_qlet_)
				flag = true;

			if(now - last_arrive_time[position] > interval_msg_){
				counter[position] = 0;
				last_arrive_queue[position] = 0;
				ret = true;
			}

			qid = max(qid, last_arrive_queue[position]);
			counter[position] += 1460;
			last_arrive_time[position] = now;
		}
		if(ret)		return 0;	// if it is a new message, it should be placed into the first queue
		if(flag)	return max(qid - 1, 0);	// if it is a new qlet, it can be placed into the (qid-1)-th queue
		return qid;
	}

	uint Query_count(uint flow_id)
	{
		uint ret = 0x7FFFFFFF;
		for(uint i = 0; i < HASH_NUM; ++i)
		{
			 uint position = BOBHash32((uchar*)(&flow_id), sizeof(uint), i) % LENGTH;
			 ret = min(ret, counter[position]);
		}
		return ret;
	}

	/* when a packet is to be inserted in to a queue, 
	 * record its queue id
	 */
	void Update_queue(uint flow_id, uint queue_id)
	{		
		for(uint i = 0; i < HASH_NUM; ++i)
		{
			 uint position = BOBHash32((uchar*)(&flow_id), sizeof(uint), i) % LENGTH;
			 last_arrive_queue[position] = max(queue_id, last_arrive_queue[position]);
		}
	}
};



#endif
