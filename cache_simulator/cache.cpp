#include <math.h>
#include <string.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

//globals
int l1_data_rd_hit = 0;
int l1_data_wr_hit = 0;
int l1_ins_hit = 0;

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
      return "";
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
    //line = line.substr(0, line.size() - 1);
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
    ans = ans * 2 + dig;
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
  //  int bit_total;
  int bit_offset;
  int bit_setind;

  //valid bits, put them together rather distributed to every cache
  vector<bool> valids;

  //tags, put them together rather distributed to every cache
  vector<string> tags;

  //LRUs
  vector<int> LRU_ways;

  //dirty
  vector<bool> dirties;

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
  void printResults(string FILE, vector<string> & input);
  int next_way(int set_ind);
  void updateResult(char cmdType, bool miss);
  void accessCache(string FILE, vector<string> & input);
};

void CacheConfig::printResults(string FILE, vector<string> & input) {
  accessCache(FILE, input);
  if (l1) {
    if (cacheType == "instruction") {
      l1_ins_hit = instr - ins_miss;
    }
    else if (cacheType == "data") {
      l1_data_rd_hit = data_rd - rd_miss;
      l1_data_wr_hit = data_wr - wr_miss;
    }
    else {
      l1_ins_hit = instr - ins_miss;
      l1_data_rd_hit = data_rd - rd_miss;
      l1_data_wr_hit = data_wr - wr_miss;
    }
  }
  else if (!l1) {
    if (cacheType == "instruction") {
      instr = instr - l1_ins_hit;
    }
    else if (cacheType == "data") {
      data_rd = data_rd - l1_data_rd_hit;
      data_wr = data_wr - l1_data_wr_hit;
    }
    else {
      instr = instr - l1_ins_hit;
      data_rd = data_rd - l1_data_rd_hit;
      data_wr = data_wr - l1_data_wr_hit;
    }
    total_fetch = instr + data_rd + data_wr;
  }
  cout << "Metrics:              Total       Instruction   Data       Read       Write      "
       << endl;
  cout << "---------------------------------------------------------------------------------"
       << endl;
  cout << "Demand Fetches        " << left << setw(12) << total_fetch << setw(14) << instr
       << setw(11) << data_rd + data_wr << setw(11) << data_rd << setw(11) << data_wr << endl;
  cout << "Fraction of total     " << left << setw(12) << (float)total_fetch / total_fetch
       << setw(14) << (float)instr / total_fetch << setw(11)
       << (float)(data_rd + data_wr) / total_fetch << setw(11)
       << (float)data_rd / ((data_rd + data_wr) == 0 ? 1 : (data_rd + data_wr)) << setw(11)
       << (float)data_wr / ((data_rd + data_wr) == 0 ? 1 : (data_rd + data_wr)) << endl;
  cout << "Demand Misses         " << left << setw(12) << ins_miss + rd_miss + wr_miss << setw(14)
       << ins_miss << setw(11) << rd_miss + wr_miss << setw(11) << rd_miss << setw(11) << wr_miss
       << endl;
  cout << "Demand Miss-rate      " << left << setw(12)
       << (float)(ins_miss + rd_miss + wr_miss) / total_fetch << setw(14)
       << (float)ins_miss / ((instr == 0) ? 1 : instr) << setw(11)
       << (float)(rd_miss + wr_miss) / total_fetch << setw(11)
       << (float)rd_miss / (data_rd == 0 ? 1 : data_rd) << setw(11)
       << (float)wr_miss / (data_wr == 0 ? 1 : data_wr) << endl;
  cout << endl;
}

void CacheConfig::accessCache(string FILE, vector<string> & input) {
  vector<string> res;
  if (l1 || cacheType == "instruction") {
    getCommnad(FILE, res);
  }
  else {
    res = input;
  }

  total_fetch = res.size();
  for (int i = 0; i < total_fetch; ++i) {
    char cmdType = res[i][0];
    string t = res[i].substr(1);
    reverse(t.begin(), t.end());
    string t2 = t.substr(bit_offset, bit_setind);
    int set_ind = str2int_base2(t2);
    string tag = t.substr(bit_offset + bit_setind);
    //ineffient but easy to write...
    if ((cacheType == "instruction" && cmdType != '2') || (cacheType == "data" && cmdType == '2')) {
      continue;
    }
    if (l1 && cacheType == "data") {
      input.push_back(res[i]);
    }
    updateResult(cmdType, false);

    bool flag_hit = false;
    int j = 0;
    for (j = 0; j < A; ++j) {
      if (valids[A * set_ind + j] == true && tags[A * set_ind + j] == tag) {
        flag_hit = true;
        if (cmdType == '1' && cacheType == "data") {
          dirties[A * set_ind + j] = true;
        }
        break;
      }
      if (valids[A * set_ind + j] == false) {
        break;
      }
    }
    if (flag_hit)
      continue;
    updateResult(cmdType, true);
    //write through
    if (cmdType == '1' && !writeAllocate) {
      continue;
    }

    if (j < A) {
      tags[j + A * set_ind] = tag;
      valids[j + A * set_ind] = true;
    }
    else {
      if (cmdType == '0' && dirties[A * set_ind + j] == true) {
        string t = tags[j + A * set_ind] + t2 + res[i].substr(0, bit_offset);
        reverse(t.begin(), t.end());
        t = "1" + t;
        input.push_back(t);
        dirties[j + A * set_ind] = false;
      }
      tags[A * set_ind + next_way(set_ind)] = tag;
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
    srand(1);
    ans = rand() % A;
    ans = 0;
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

  bit_offset = log(B) / log(2);
  bit_setind = log(num_set) / log(2);

  valids = vector<bool>(num_way, false);
  tags = vector<string>(num_way, string(32 - bit_offset - bit_setind, '0'));
  LRU_ways = vector<int>(num_set, 0);
  dirties = vector<bool>(num_way, false);
  rd_miss = 0;
  wr_miss = 0;
  ins_miss = 0;
  data_rd = 0;
  data_wr = 0;
  instr = 0;
  total_fetch = 0;
}

int main() {
  CacheConfig l1_ins, l1_data, l2_ins, l2_data;
  l1_ins.initilizeConfig("l1_ins.txt");
  l1_data.initilizeConfig("l1_data.txt");
  l2_ins.initilizeConfig("l2_ins.txt");
  l2_data.initilizeConfig("l2_data.txt");
  vector<string> input1;
  vector<string> input2;

  l1_data.printResults("FULL.txt", input2);
  l1_ins.printResults("FULL.txt", input1);
  l2_data.printResults("FULL.txt", input2);
  vector<string> dummy;

  l2_ins.printResults("FULL.txt", dummy);

  return EXIT_SUCCESS;
}
