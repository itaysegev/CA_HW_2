/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <algorithm>
#include <bitset>
#include <string>
#include <math.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using namespace std;

// converts hexadecimal to binary 
string hexToBin(string hex_str) {
    stringstream ss;
    ss << hex << hex_str;
    unsigned n;
    ss >> n;
    bitset<32> b(n);
    return b.to_string();
}

// calculate blocks number
inline int blocksNumCalc(unsigned int BSize, unsigned int DSize) {
	return (int)pow(2, DSize - BSize);
}





// calculate sets number - shape of cache table
inline int setsNumCalc(int blocks_num, int assoc) {
	return (int)(blocks_num/pow(2, assoc));
}



// main data struct which includes l1 l2 and relevent data for both caches
class Mem {
	class cache {
		class Block {
		public:
			int tag;
			bool valid;
			bool dirty;
			string full_adress;
			Block() : tag(0), valid(false), dirty(true), full_adress(8, '*') {}
		};
		int BSize; // block size
		int sets_num;
		int ways_num;
		int DSize; // cache data size
		// queue sorted by last used way index for LRU protocol
		list<int>* LRU_by_way_index;
	public:
		Block** table; // 2D array set X way for data
		cache(int Sblock, int ways_number, int Sdata, int sets_number) {
			BSize = Sblock;
			sets_num = sets_number;
			ways_num = ways_number;
			DSize = Sdata;
			table = new Block*[sets_num];
			for (int i = 0; i < sets_num; ++i) {
				table[i] = new Block[ways_num];
			}
			LRU_by_way_index = new list<int>[sets_num];
		}
		~cache() {
			
			for (int i = 0; i < sets_num; ++i) {
				delete[] table[i];
				
			}
			delete[] table;
			delete[] LRU_by_way_index;
		}

		int setCalc(string hex_address, int sets_num) {
			string bin_address = hexToBin(hex_address);
			reverse(bin_address.begin(), bin_address.end());
			string set_bin = bin_address.substr(BSize, log2(sets_num));
			reverse(set_bin.begin(), set_bin.end());
			return strtoul(set_bin.c_str(), NULL, 2);
		}


		int tagCalc(string hex_address, int sets_num) {
			string bin_address = hexToBin(hex_address);
			string tag_bin = bin_address.substr(0, 32 - BSize - log2(sets_num));
			//cout << tag_bin << endl;
			return strtoul(tag_bin.c_str(), NULL, 2);
		}

		// called only from l1
		// return the address if we evicted data that was dirty and therefore need to update l2
		void insertTOL1(string cut_address) {
			int curr_set = setCalc(cut_address, sets_num);
			int curr_tag = tagCalc(cut_address, sets_num);
			int curr_way = findFreeWay(curr_set);
			if (curr_way == -1) { // no free way need to remove 
				curr_way = LRU_by_way_index[curr_set].front();
				LRU_by_way_index[curr_set].pop_front();
				//if (table[curr_set][curr_way].dirty) {
				//	return string(table[curr_set][curr_way].full_adress);  // if dirty and evicted we need to update l2
			//	}
			}
			//insert new block
			table[curr_set][curr_way].valid = true;
			table[curr_set][curr_way].tag = curr_tag;
			table[curr_set][curr_way].full_adress = cut_address;
			table[curr_set][curr_way].dirty = false;
			LRU_by_way_index[curr_set].remove(curr_way);
			LRU_by_way_index[curr_set].push_back(curr_way); // update LRU queue

		}


		//called only from l2, fine a aline to evict or nullptr in case it wasn't needed.
		// so we will be able to snoop l1 before actually evict it.
		bool evict(string cut_address, string* evicted) {
			bool evict = false;
			int curr_set = setCalc(cut_address, sets_num);
			int curr_tag = tagCalc(cut_address, sets_num);
			int curr_way = findFreeWay(curr_set);
			if (curr_way == -1) { // no free way need to remove 
				curr_way = LRU_by_way_index[curr_set].front();
				LRU_by_way_index[curr_set].pop_front();
				*evicted = table[curr_set][curr_way].full_adress ;  // if dirty and evicted we need to update l2
				table[curr_set][curr_way].valid = false;
				 evict = true;
			}

			return evict;
		}

		// called only from l2 , after snooping l1 and knowing we have a free way for sure
		// if we evicted line we  
		void insertTOL2(string cut_address) {
			int curr_set = setCalc(cut_address, sets_num);
			int curr_tag = tagCalc(cut_address, sets_num);
			int curr_way = findFreeWay(curr_set);
			table[curr_set][curr_way].valid = true;
			table[curr_set][curr_way].tag = curr_tag;
			table[curr_set][curr_way].full_adress = cut_address;
			table[curr_set][curr_way].dirty = false;
			LRU_by_way_index[curr_set].remove(curr_way);
			LRU_by_way_index[curr_set].push_back(curr_way); // update LRU queue
		}


		//this function is to write in case of dirty update.
		void write(string cut_address) {
			// update stats
			int curr_set = setCalc(cut_address, sets_num);
			int curr_tag = tagCalc(cut_address, sets_num);
			int curr_way;
			for (curr_way = 0; curr_way < ways_num; curr_way++) {
				if (table[curr_set][curr_way].tag == curr_tag && table[curr_set][curr_way].valid) {
					break;
				}
			}
			table[curr_set][curr_way].dirty = true;
		}


		// search if is it hit or a miss in the cache 
		bool search(string cut_address) {
			int curr_set = setCalc(cut_address, sets_num);
			int curr_tag = tagCalc(cut_address, sets_num);
			for (int i = 0; i < ways_num; ++i) {
				if (table[curr_set][i].tag == curr_tag && table[curr_set][i].valid) {
					LRU_by_way_index[curr_set].remove(i);
					LRU_by_way_index[curr_set].push_back(i);
					return true;
				}
			}
			return false;
		}

		//Called from L1 When L2 need to evict a block, it snoop l1 to make it invalid
		//return true if is was dirty and so we need to make it dirty in l2
		//in this hw we dont care if it was dirty and so we need to update l2 because it will be evicted anyway
		bool snoop(string cut_address) {
			bool is_dirty = false;
			int curr_set = setCalc(cut_address, sets_num);
			int curr_tag = tagCalc(cut_address, sets_num);
			for (int i = 0; i < ways_num; ++i) {
				if (table[curr_set][i].tag == curr_tag && table[curr_set][i].valid) {
					//cout << "found in way num" << i << endl;
					table[curr_set][i].valid = false; //invalidate the data
					if (table[curr_set][i].dirty) {
						return true; // was dirty
					}

				}
			}
			return false; //either not existed or was not dirty
		}

		//called from L2- when L1 need to update the dirty bit in L2
		void dirty(string cut_address) {
			int curr_set = setCalc(cut_address, sets_num);
			int curr_tag = tagCalc(cut_address, sets_num);
			for (int i = 0; i < ways_num; ++i) {
				if (table[curr_set][i].tag == curr_tag && table[curr_set][i].valid) {
					table[curr_set][i].dirty = true; //invalidate the dat

				}
			}
		}
  
		int findFreeWay(int curr_set) {
			int i;
			for (i = 0; i < ways_num; i++) {
				if (!table[curr_set][i].valid) {
					return i;
				}
			}
			return -1;
		}

		// function for DEBUG only
		void printTable() {
			cout << "---------------TABLE------------------------" << endl;
			for (int i = 0; i < sets_num; ++i) {
				for(int j = 0; j < ways_num; ++j) {
					cout << "set: " << i << endl;
					cout << "way: " << j << endl;
					if(table[i][j].valid){
						cout << table[i][j].tag << endl;
					}
				}
			}
		}
	};
public:
	cache L1;
	cache L2;
	int mem_cyc;
	int wr_alloc;
	int l1_cyc;
	int l2_cyc;
	int l1_miss;
	int l1_access;
	int l2_miss;
	int l2_access;
	int mem_access;
	Mem(unsigned MemCyc,  unsigned BSize , unsigned L1Size,
	 unsigned L2Size, unsigned L1Assoc, unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc)
	  : mem_cyc(MemCyc),
	  wr_alloc(WrAlloc),
	  l1_cyc(L1Cyc),
	  l2_cyc(L2Cyc),
	  L1(BSize, pow(2, L1Assoc), L1Size,setsNumCalc(blocksNumCalc(BSize, L1Size), L1Assoc)),
	  L2(BSize, pow(2, L2Assoc), L2Size,setsNumCalc(blocksNumCalc(BSize, L2Size), L2Assoc)),
	  l1_miss(0),
	  l1_access(0),
	  l2_miss(0),
	  l2_access(0),
      mem_access(0)
	  {}


	~Mem()= default;
	void read(string cut_address) {
		//cout << "now reading from " << cut_address << endl;
		//try l1
		l1_access++;
		if(L1.search(cut_address)) {

			return;
		}
		l1_miss++;
		//tryl2
		l2_access++;
		//cout << "not in l1" << endl;
		if (L2.search(cut_address)) {
			L1.insertTOL1(cut_address);
			return;
			
		}
		//cout << "not in l2:)" << endl;
		l2_miss++;
		//go to Mem
		mem_access++;
		// update blocks in l1 and l2
		string evicted(8, '*');
		//cout << evicted << endl;
		//cout << cut_address << endl;
		bool evict = L2.evict(cut_address,&evicted);
		//cout << "the chosen is" << evicted << endl;
		if (evict) {
			L1.snoop(evicted);
		}
		L2.insertTOL2(cut_address);
		L1.insertTOL1(cut_address);
		//if (dirty) {
		//	L2.dirty(dirty);
		//return;					  `
		return;
	}



	void write(string cut_address) {
		//try l1
		//cout << "now writing to " << cut_address << endl;
		l1_access++;
		if(L1.search(cut_address)) {
			return;
		}
		//cout << "not in l1" << endl;
		//L1 miss
		l1_miss++;
		//try l2
		l2_access++;
		if(L2.search(cut_address)) {
			if (wr_alloc) {
				L1.insertTOL1(cut_address);
			}
			return;
		}
		//cout << "not in l2" << endl;
		l2_miss++;
		//go to mem
		mem_access++;
		if (wr_alloc) {
			string evicted(8, '*');
			//cout << evicted << endl;
			bool evict = L2.evict(cut_address, &evicted);
			//cout << cut_address << endl;
			//cout << evicted << endl;
			if (evict) {
				L1.snoop(evicted);
			}
			L2.insertTOL2(cut_address);
			L1.insertTOL1(cut_address);				  
			return;
		}			
	}
};

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	
	Mem mem(MemCyc, BSize, L1Size, L2Size, L1Assoc,
			L2Assoc, L1Cyc, L2Cyc, WrAlloc);

	int lines = 0;
	while (getline(file, line)) {
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		//cout << "operation: " << operation;
		lines++;
		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress << endl;
		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
		char R = 'r';
		if(operation == R) {
			mem.read(cutAddress);
		}
		else {
			mem.write(cutAddress);
		}
		// for DEBUG only
		//mem.L1.printTable();
		//mem.L2.printTable();
		//cout << "l1 miss is" << mem.l1_miss << endl;
		//cout << "l2 miss is" << mem.l2_miss << endl;
		//cout << "l1 access is" << mem.l1_access << endl;  
		//cout << "l2 access is" << mem.l2_access << endl;
		//cout << "mem" << mem.mem_access << endl;

	}
	double L1MissRate;
	double L2MissRate;
	double avgAccTime;
	//cout << "l1 miss is" << mem.l1_miss << endl;
	//cout << "l1 access is" << mem.l1_access << endl;
	L1MissRate = (double)mem.l1_miss / (double)mem.l1_access;
	L2MissRate = (double)mem.l2_miss / (double)mem.l2_access;
	avgAccTime = ((double)((mem.l1_access) * (mem.l1_cyc) + (mem.l2_access) * (mem.l2_cyc) + (mem.mem_access) * (mem.mem_cyc))) / ((double)(lines));

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}



