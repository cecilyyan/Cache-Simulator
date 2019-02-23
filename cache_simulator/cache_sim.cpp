#include <math.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;
string hex2bin_helper(char hex) {
  switch (hex) {
    case '0':
      return "0000";
    case '1':
      return "0001";
    case '2':
      return "0010";
    case '3':
      return "0011";
    case '4':
      return "0100";
    case '5':
      return "0101";
    case '6':
      return "0110";
    case '7':
      return "0111";
    case '8':
      return "1000";
    case '9':
      return "1001";
    case 'a':
      return "1010";
    case 'b':
      return "1011";
    case 'c':
      return "1100";
    case 'd':
      return "1101";
    case 'e':
      return "1110";
    case 'f':
      return "1111";
    default:
      return "xxxx";
  }
}

void hex2bin(string hex, string & bin) {
  for (size_t i = 0; i < hex.size(); ++i) {
    bin += hex2bin_helper(hex[i]);
  }
}

void getCommnad(string FILE, vector<string> & res) {
  ifstream f(FILE, ios::in);
  string line = "";
  while (getline(f, line)) {
    string tmp = "";
    tmp += line[0];
    line = line.substr(2);
    int t = 8 - line.size();
    string zero = "00000000";
    line = zero.substr(0, t) + line;
    hex2bin(line, tmp);
    res.push_back(tmp);
  }
  f.close();
}

int str2int_base2(string s) {
  int ans = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    int dig = s[i] - '0';
    ans += pow(2, i) * dig;
  }
  return ans;
}

class CacheConfig
{
 public:
  bool l1;
  int A;
  int B;
  int C;
  bool writeAllocate;
  bool LRU;
  string cacheType;

  //calculated parameters:
  int num_set;
  int num_way;
  int bit_total;
  int bit_offset;
  int bit_setind;

  //valid bits, put them together rather distributed to every cache
  vector<bool> valids;

  //tags, put them together rather distributed to every cache
  vector<string> tags;

  //LRUs
  vector<int> LRU_ways;

  //results
  int rd_miss;
  int wr_miss;
  int ins_miss;
  int data_rd;
  int data_wr;
  int instr;
  int total_fetch;

  void initilizeConfig(string FILE);
  void print();
  void printResults();
  int next_way(int set_ind);
  void updateResult(char cmdType, bool miss);
  void accessCache();
};

void CacheConfig::printResults() {
  accessCache();
  cout << "data_rd : " << data_rd << endl;
  cout << "data_wr : " << data_wr << endl;
  cout << "instruction : " << instr << endl;
  cout << "total : " << total_fetch << endl;
  cout << "read_miss : " << rd_miss << endl;
  cout << "write_miss : " << wr_miss << endl;
  cout << "instruction_miss : " << ins_miss << endl;
}

void CacheConfig::accessCache() {
  vector<string> res;
  getCommnad("TESTDATA.txt", res);
  total_fetch = res.size();
  for (size_t i = 0; i < res.size(); ++i) {
    char cmdType = res[i][0];
    int set_ind = str2int_base2(res[i].substr(bit_offset, bit_setind).c_str());
    string tag = res[i].substr(bit_offset + bit_setind);
    //ineffient but easy to write...
    if ((cacheType == "instruction" && cmdType != '2') || (cacheType == "data" && cmdType == '2')) {
      continue;
    }
    updateResult(cmdType, false);

    bool flag_hit = false;
    int j = 0;
    for (j = 0; j < A; ++j) {
      if (valids[A * set_ind + j] == true && tags[A * set_ind + j] == tag) {
        flag_hit = true;
        break;
      }
      if (valids[A * set_ind + j] == false) {
        break;
      }
    }
    if (flag_hit)
      continue;
    //write through
    if (cmdType == '1' && !writeAllocate) {
      continue;
    }

    if (j < A * set_ind + A) {
      tags[j] = tag;
      valids[i] = true;
    }
    else {  //write back
      tags[next_way(set_ind)] = tag;
    }
  }
}

int CacheConfig::next_way(int set_ind) {
  int ans = 0;
  if (LRU) {
    ans = LRU_ways[set_ind];
    ++LRU_ways[set_ind];
    if (LRU_ways[set_ind] > A - 1)
      LRU_ways[set_ind] = 0;
  }
  else {  //random
    ans = rand() % A;
  }
  return ans;
}

void CacheConfig::updateResult(char cmdType, bool miss) {
  if (miss) {  //update for misses
    if (cmdType == '0') {
      ++rd_miss;
    }
    else if (cmdType == '1') {
      ++wr_miss;
    }
    else if (cmdType == '2') {
      ++ins_miss;
    }
    else {
      cout << "invalid cmdType" << endl;
    }
  }

  else {
    if (cmdType == '0') {
      ++data_rd;
    }
    else if (cmdType == '1') {
      ++data_wr;
    }
    else if (cmdType == '2') {
      ++instr;
    }
    else {
      cout << "invalid cmdType" << endl;
    }
  }
}
void CacheConfig::print() {
  cout << "l1/l2:" << l1 << endl;
  cout << "A = " << A << endl;
  cout << "B = " << B << endl;
  cout << "C = " << C << endl;
  cout << "writeAllocate:" << writeAllocate << endl;
  cout << "LRU:" << LRU << endl;
  cout << cacheType << endl;

  //calculated parameters:
  cout << "set = " << num_set << endl;
  cout << "way = " << num_way << endl;
  cout << "total = " << bit_total << endl;
  cout << "offset = " << bit_offset << endl;
  cout << "setind = " << bit_setind << endl;

  cout << "rd_miss " << rd_miss << endl;
  cout << "wr_miss " << wr_miss << endl;
  cout << "ins_miss " << ins_miss << endl;
  cout << "data_rd " << data_rd << endl;
  cout << "data_wr " << data_wr << endl;
  cout << "instr " << instr << endl;
  cout << "total_fetch " << total_fetch << endl;
}

void CacheConfig::initilizeConfig(string FILE) {
  ifstream f(FILE, ios::in);
  string line;
  int rd_cnt = 0;
  while (getline(f, line)) {
    switch (rd_cnt) {
      case 0:
        l1 = (line == "l1");
        break;
      case 1:
        A = atoi(line.c_str());
        break;
      case 2:
        B = atoi(line.c_str());
        break;
      case 3:
        C = atoi(line.c_str());
        break;
      case 4:
        writeAllocate = (line == "writeAllocate");
        break;
      case 5:
        LRU = (line == "LRU");
        break;
      case 6:
        cacheType = line;
        break;
      default:
        cerr << "something is wrong" << endl;
        break;
    }
    ++rd_cnt;
  }
  num_set = C / B / A;
  num_way = C / B;
  bit_total = log(C) / log(2);
  bit_offset = log(B) / log(2);
  bit_setind = log(num_set) / log(2);

  valids = vector<bool>(num_way, false);
  tags = vector<string>(num_way, string(32 - bit_offset - bit_setind, '0'));
  LRU_ways = vector<int>(num_set, 0);
  rd_miss = 0;
  wr_miss = 0;
  ins_miss = 0;
  data_rd = 0;
  data_wr = 0;
  instr = 0;
  total_fetch = 0;
}

int main() {
  CacheConfig l1_ins, l1_data, l2;

  l1_ins.initilizeConfig("l1_ins.txt");
  l1_data.initilizeConfig("l1_data.txt");
  l2.initilizeConfig("l2.txt");
  l1_ins.printResults();
  l1_data.printResults();
  l2.printResults();
  return EXIT_SUCCESS;
}
