/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <bitset>
#include <math.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using namespace std;
// main data struct 
// class mem {
// 	cache L1;
// 	cache L2;
// 	int mem_cyc;
// 	bool wr_alloc;
// };

class Line {
public:
	int tag;
};

// // for l1 and l2 
// class cache {
// 	int BSize;
// 	int set_num;
// 	int way_num;
// 	int DSize;
	
// public:
// 	Line[set_num][way_num];
// 	cache(int Bsize, int way_num, int DSize, int set_num);
// 	~cache();
// };

// cache::cache(int Bsize, int way_num, int DSize, int set_num) : 
// 	Bsize=Bsize,
// 	set_num=set_num, 
// 	way_num=way_num, 
// 	DSize=DSize {

// }

// cache::~cache()
// {
// }

string hexToBin(string hex_str) {
    stringstream ss;
    ss << hex << hex_str;
    unsigned n;
    ss >> n;
    bitset<32> b(n);
    return b.to_string();
}

inline int blocksNumCalc(unsigned int BSize, unsigned int DSize) {
	return pow(2, DSize - BSize);
}

int setCalc(string hex_address, int assoc) {
	string bin_address = hexToBin(hex_address);
	string set_bin = bin_address.substr(2, assoc);
	return stoi(set_bin, 0, 2); 
}


int tagCalc(string hex_address, int assoc) {
	string bin_address = hexToBin(hex_address);
	string set_bin = bin_address.substr(2 + assoc);
	return stoi(set_bin, 0, 2); 
}

inline int setsNumCalc(int blocks_num, int assoc) {
	return blocks_num/pow(2, assoc);
}

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
	int blocks_num = blocksNumCalc(BSize, L1Size);
	int ways_num = pow(2, L1Assoc);
	int sets_num = setsNumCalc(blocks_num, L1Assoc);
	// cout << "blocks nums: " << blocks_num;
	// cout << "sets num: " << sets_num;
	// cout << "ways num: " << ways_num;
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
		cout << ", address (hex)" << cutAddress;
		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (set) " << setCalc(cutAddress, L1Assoc) << endl;
		cout << " (tag) " << tagCalc(cutAddress, L1Assoc) << endl;
		

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}



