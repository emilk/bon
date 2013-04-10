//
//  bench.cpp
//  BON
//
//  Created by emilk on 2013-04-10.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#include "catch.hpp"


extern "C" {
#include "bon.h"
#include "bon_private.h"
#include "crc32.h"
}


#include <msgpack.hpp>

#include <iostream>
#include <iomanip>
#include <numeric>  // accumulate


using namespace std;


//------------------------------------------------------------------------------


#if DEBUG
const int NUM_VALS = 16 * 1024;
#else
const int NUM_VALS = 16 * 1024*1024;
#endif

using namespace std::chrono;
typedef high_resolution_clock Clock;


template<typename Fun>
void time(const Fun& fun)
{
	Clock::time_point start = Clock::now();
	
	fun();
	
	Clock::time_point stop = Clock::now();
	
	auto ms = duration_cast<milliseconds>(stop - start).count();
	
	std::cout << ms << " ms" << std::endl;
}

template<typename It>
void printStatistics(It begin, It end)
{
	const auto N = std::distance(begin, end);
	typedef double Float;
	
	Float mean = std::accumulate(begin, end, (Float)0) / (Float)N;
	
	Float dev = std::sqrt( std::accumulate(begin, end, (Float)0, [=](Float acc, Float v){
		return acc + (v-mean)*(v-mean);
	} ) / N );
	
	auto p = std::minmax_element(begin, end);
	auto min = *p.first;
	auto max = *p.second;
	
	const int W = 5;
	std::cout  << "mean, dev, min, max: " << std::setprecision(5)
	<< std::setw(W) << mean << ", "
	<< std::setw(W) << dev << ", "
	<< std::setw(W) << min << ", "
	<< std::setw(W) << max << std::endl;
}

template<typename Fun, typename Cleanup>
void time_n(unsigned n, const Fun& fun, const Cleanup& cleanup)
{
	std::vector<long long> times;
	
	const int warmup = 3;
	
	for (unsigned i=0; i<warmup; ++i)
	{
		fun();
		cleanup();
	}
	
	for (unsigned i=0; i<n; ++i)
	{
		Clock::time_point start = Clock::now();
		fun();
		Clock::time_point stop = Clock::now();
		times.push_back( duration_cast<milliseconds>(stop - start).count() );
		
		cleanup();
	}
	
	printStatistics(begin(times), end(times));
}

template<typename Fun>
void time_n(unsigned n, const Fun& fun)
{
	time_n(n, fun, [](){});
}
	
#if 1

TEST_CASE( "BON/bench/writing", "Speed of bon_w_float")
{
	printf("\nBON write speed:\n");
	const std::vector<float> src(NUM_VALS, 3.14f);
	
	time_n(16, [&]() {
		bon_byte_vec vec = {0,0,0};
		
		bon_w_doc* B = bon_w_new_doc(bon_vec_writer, &vec, BON_W_FLAG_DEFAULT);
		bon_w_begin_obj(B);
		bon_w_key(B, "list");
		bon_w_begin_list(B);
		for (auto f : src) {
			bon_w_float(B, f);
		}
		bon_w_end_list(B);
		bon_w_end_obj(B);
		REQUIRE( bon_w_close_doc(B) == BON_SUCCESS );
		
		free(vec.data);
	});
}

TEST_CASE( "BON/bench/writing/msgpack", "MsgPack speed of writing floats")
{
	printf("\nMsgPack write speed:\n");
	const std::vector<float> src(NUM_VALS, 3.14f);
	
	time_n(16, [&]() {
		msgpack::sbuffer buffer;  // simple buffer
		
		//msgpack::pack(&buffer, src);
		for (auto f : src) {
			msgpack::pack(&buffer, f);
		}
	});
}

#endif 


#if 1 // !DEBUG

template<class SrcType, class DstType>
void run_float_benchmark(bool packed)
{
	const std::vector<SrcType> src(NUM_VALS, 3.14f);
	std::vector<DstType> dst(NUM_VALS, 0.0f);
	
	bon_byte_vec vec = {0,0,0};
	
	printf("Writing... ");
	time([&]() {
		bon_w_doc* B = bon_w_new_doc(bon_vec_writer, &vec, BON_W_FLAG_DEFAULT);
		bon_w_begin_obj(B);
		bon_w_key(B, "list");
		if (packed) {
			if (std::is_same<SrcType, float>::value) {
				bon_w_pack_array(B, src.data(), sizeof(SrcType)*NUM_VALS, NUM_VALS, BON_TYPE_FLOAT);
			} else {
				bon_w_pack_array(B, src.data(), sizeof(SrcType)*NUM_VALS, NUM_VALS, BON_TYPE_DOUBLE);
			}
		} else {
			bon_w_begin_list(B);
			if (std::is_same<SrcType, float>::value) {
			for (auto f : src) {
					bon_w_float(B, f);
			}
			} else {
				for (auto f : src) {
					bon_w_double(B, f);
				}
			}
			bon_w_end_list(B);
		}
		bon_w_end_obj(B);
		REQUIRE( bon_w_close_doc(B) == BON_SUCCESS );
	});
	
	bon_r_doc* B;
	bon_value* root;
	bon_value* list;
	
	auto parse = [&]() {
		B = bon_r_open(vec.data, vec.size, BON_R_FLAG_DEFAULT);
		root = bon_r_root(B);
		list = bon_r_get_key(B, root, "list");
	};
	
	printf("Parsing... ");
#if 0
	time_n(16, parse, [&]() {
		bon_r_close(B);
	});
	parse();
#else
	time([&]() { parse(); });
#endif
	
	printf("Copying... ");
	//time_n(10, [&]() {
	time([&]() {
		bool win = false;
		//if (true) {
		if (packed) {
			if (std::is_same<DstType, float>::value) {
				win = bon_r_unpack_fmt(B, list, dst.data(), sizeof(DstType)*NUM_VALS, "[#f]", NUM_VALS);
			} else if (std::is_same<DstType, double>::value) {
				win = bon_r_unpack_fmt(B, list, dst.data(), sizeof(DstType)*NUM_VALS, "[#d]", NUM_VALS);
			} else {
			}
		} else {
			for (bon_size ix = 0; ix < NUM_VALS; ++ix) {
				bon_value* elem = bon_r_list_elem(B, list, ix);
				if (!elem) {
					win = false;
					break;
				}
				if (std::is_same<DstType, float>::value) {
					dst[ix] = bon_r_float(B, elem);
				} else {
					dst[ix] = (DstType)bon_r_double(B, elem);
				}
			}
			win = true;
		}
		REQUIRE( win );
		REQUIRE( dst[1] == 3.14f );
	});
	
	bon_r_close(B);
	
	printf("Used %d MB\n", (int)std::round((float)vec.size / 1024 / 1024));
	
	free(vec.data);
}

template<class SrcType, class DstType>
void float_bench_msgpack()
{
	const std::vector<float> src(NUM_VALS, 3.14f);
	std::vector<float> dst;
	dst.reserve(NUM_VALS); // To make it fair
	
	msgpack::sbuffer buffer;  // simple buffer
	
	printf("Writing... ");
	time([&]() {
		msgpack::pack(&buffer, src);
	});
	
	msgpack::unpacked msg;
	msgpack_object mobj;
	
	printf("Parsing... ");
	time([&]() {
		msgpack::unpack(&msg, buffer.data(), buffer.size());
		mobj = msg.get();
	});
	msgpack::object obj = mobj;
	
	printf("Copying... ");
	time([&]() {
		obj.convert(&dst);
		REQUIRE( dst[1] == 3.14f );
	});
	
	printf("Used %d MB\n", (int)std::round((float)buffer.size() / 1024 / 1024));
}


template<class SrcType, class DstType>
void array_write_bench()
{
	printf("\n");
	printf("bon (packed):\n");
	run_float_benchmark<SrcType,DstType>(true);
	
	printf("\n");
	printf("bon (exploded):\n");
	run_float_benchmark<SrcType,DstType>(false);
	
	printf("\n");
	printf("msgpack:\n");
	float_bench_msgpack<SrcType,DstType>();
}


TEST_CASE( "BON/bench", "Benching writing and reading of packed values vs" )
{
	printf("----------WARMUP---------\n");
	array_write_bench<float, float>();
	printf("\n----\n");
	
#if 0
	run_float_benchmark<float,float>(false);
#else
	printf("Benchmark - packed vs 'exploded', writing and reading of %d values\n", NUM_VALS);
	
	printf("------------------------------------------------------------------------------\n");
	printf("float -> float\n");
	array_write_bench<float, float>();
	
	
	printf("------------------------------------------------------------------------------\n");
	printf("float -> double\n");
	array_write_bench<float, double>();
#endif
	
}
#endif // DEBUG/bench
