/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
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
// converts hexdecimal to binary 
string hexToBin(string hex_str) {
    stringstream ss;
    ss << hex << hex_str;
    unsigned n;
    ss >> n;
    bitset<32> b(n);
    return b.to_string();
}

// calculate blocks nubmer
inline int blocksNumCalc(unsigned int BSize, unsigned int DSize) {
	return pow(2, DSize - BSize);
}

int setCalc(string hex_address, int assoc) {
	string bin_address = hexToBin(hex_address);
	reverse(bin_address.begin(), bin_address.end());
	string set_bin = bin_address.substr(2, assoc);
	reverse(set_bin.begin(), set_bin.end());
	return strtoul(set_bin.c_str(), NULL, 2); 
}


int tagCalc(string hex_address, int assoc) {
	string bin_address = hexToBin(hex_address);
	string tag_bin = bin_address.substr(0, bin_address.length() - (3 + assoc));
	return strtoul(tag_bin.c_str(), NULL, 2); 
}

// calculate sets number - shape of cache table
inline int setsNumCalc(int blocks_num, int assoc) {
	return blocks_num/pow(2, assoc);
}



// main data struct which includes l1 l2 and relevent data for both caches
class Mem {
	class cache {
		class Line {
		public:
			int tag;
			bool valid;
			bool dirty;
		};
		int BSize; // block size
		int sets_num;
		int ways_num;
		int DSize; // cache data size
		// queue sorted by last used way index for LRU protocol
		queue<int> LRU_by_way_index;
	public:
		Line** table; // 2D array set X way for data
		cache(int Sblock, int ways_number, int Sdata, int sets_number) {
			BSize=Sblock;
			sets_num=sets_number; 
			ways_num=ways_number; 
			DSize=Sdata;
			table = new Line*[sets_num];
			for(int i = 0; i < sets_num; ++i) {
				table[i] = new Line[ways_num];
			}
			for(int i = 0; i < sets_num; ++i) {
				for(int j = 0; j < ways_num; ++j) {
					table[i][j].valid = false;
				}
			}
		}
		~cache() {
			for(int i = 0; i < sets_num; ++i) {
				delete table[i];
			}
			delete table;
		}
		// need to implement according to wr allocate
		void insert(int tag, int way, int set, bool wr_allocate);
		// function for DEBUG only 
		void printTable() {
			for (int i = 0; i < sets_num; ++i) {
				for(int j = 0; j < ways_num; ++j) {
					cout << "way: " << j << endl;
					cout << "set: " << i << endl;
					if(table[i][j].valid){
						cout << table[i][j].tag << endl;
					}
					else {
						cout << "invalid" << endl;
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
	Mem(unsigned MemCyc,  unsigned BSize , unsigned L1Size,
	 unsigned L2Size, unsigned L1Assoc, unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc)
	  : mem_cyc(MemCyc),
	  wr_alloc(WrAlloc),
	  l1_cyc(L1Cyc),
	  l2_cyc(L2Cyc),
	  L1(BSize, pow(2, L1Assoc), L1Size,setsNumCalc(blocksNumCalc(BSize, L1Size), L1Assoc)),
	  L2(BSize, pow(2, L2Assoc), L2Size,setsNumCalc(blocksNumCalc(BSize, L2Size), L2Assoc))
	  {}
	~Mem() = default;
	void read();
	void write();
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
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress << endl;
		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
	}
	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}



