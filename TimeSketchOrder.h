#ifndef _TIME_SKETCH_ORDER_H_
#define _TIME_SKETCH_ORDER_H_

#include "bobhash32.h"

#define MAX_(arg1, arg2) ((arg1) > (arg2) ? (arg1) : (arg2))
#define MIN_(arg1, arg2) ((arg1) < (arg2) ? (arg1) : (arg2))

class TimeSketchOrder{
private:
	const uint HASH_NUM;
	const uint LENGTH;
	uint MAX_QUEUE_NUM_;

	uint *counter_byte;
	uint *counter_packet;
	uint *last_arrive_queue;
	double *last_arrive_time;

	double interval_msg_;

public:
	TimeSketchOrder(uint _MAX_QUEUE_NUM, double interval_msg,
		uint _HASH_NUM = 4, uint _LENGTH = 6401):
		MAX_QUEUE_NUM_(_MAX_QUEUE_NUM), interval_msg_(interval_msg),
		HASH_NUM(_HASH_NUM), LENGTH(_LENGTH)
	{
		counter_byte = new uint[LENGTH];
		counter_packet = new uint[LENGTH];
		last_arrive_queue = new uint[LENGTH];
		last_arrive_time = new double[LENGTH];

		memset(counter_byte, 0, sizeof(uint) * LENGTH);
		memset(counter_packet, 0, sizeof(uint) * LENGTH);
		memset(last_arrive_queue, 0, sizeof(uint) * LENGTH);
		for(int i = 0; i < LENGTH; ++i)
			last_arrive_time[i] = -1;
	}

	~TimeSketchOrder(){
		delete[] counter_byte;
		delete[] counter_packet;
		delete[] last_arrive_queue;
		delete[] last_arrive_time;
	}

	/*
	 * update last_arrive_time
	 * btw, clean out-dated item
	 */
	void update_time(uint flow_id, double now){
		for(uint i = 0; i < HASH_NUM; ++i)
		{
			uint pos = get_pos(flow_id, i);
			if(now - last_arrive_time[pos] > interval_msg_)
			{
				counter_byte[pos] = 0;
				counter_packet[pos] = 0;
				last_arrive_queue[pos] = 0;
			}
			last_arrive_time[pos] = now;
		}
	}

	/*
	 * recommend a qid based on packet count
	 */
	int recommend_qid(uint flow_id)
	{
		uint min_pkt_count = 0x7FFFFFFF;
		uint min_byte_count = 0x7FFFFFFF;
		uint max_qid = 0;
		for(uint i = 0; i < HASH_NUM; ++i)
		{
			uint pos = get_pos(flow_id, i);
			min_pkt_count = MIN_(min_pkt_count, counter_packet[pos]);
			min_byte_count = MIN_(min_byte_count, counter_byte[pos]);
			max_qid = MAX_(max_qid, last_arrive_queue[pos]);
		}
		if(min_byte_count == 0)	return 0;
		if(min_pkt_count == 0)	return MAX_(max_qid - 1, 0);
		return max_qid;
	}

	/*
	 * update byte
	 */
	double add_byte(uint flow_id, int size)
	{
		uint min_byte_count = 0x7FFFFFFF;
		for(uint i = 0; i < HASH_NUM; ++i)
		{
			uint pos = get_pos(flow_id, i);
			counter_byte[pos] += (uint)size;
			min_byte_count = MIN_(min_byte_count, counter_byte[pos]);
		}
		return (double)min_byte_count;
	}

	/*
	 * update last_arrive_queue
	 */
	void update_qid(uint flow_id, int qid)
	{
		for(uint i = 0; i < HASH_NUM; ++i)
		{
			uint pos = get_pos(flow_id, i);
			last_arrive_queue[pos] = MAX_(last_arrive_queue[pos], qid);
			counter_packet[pos] += 1;
		}
	}

	/*
	 * deque a packet
	 */
	void deque_pkt(uint flow_id)
	{
		for(uint i = 0; i < HASH_NUM; ++i)
		{
			uint pos = get_pos(flow_id, i);
			counter_packet[pos] = MAX_(counter_packet[pos] - 1, 0);
		}
	}

private:
	inline uint get_pos(uint flow_id, uint i){
		return BOBHash32((uchar*)(&flow_id), sizeof(uint), i) % LENGTH;
	}
};


#endif
