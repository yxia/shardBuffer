#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <algorithm>

#define _SHARDBUF_SUCCEED   0
#define _SHARDBUF_OUT_RANGE 2
#define _SHARDBUF_OVERFLOW  3


#define CHECK_OUT_RANGE(n) {if((n) >= entries.size()) return _SHARDBUF_OUT_RANGE;}


using namespace std;

class shard_buffer_t {
  unsigned int chunk_log = 20;  // chunk_size = 2^20 = 1M
  vector<string> chunks;
  vector<string> growbuf;
  vector<pair<size_t, size_t> > entries;

public:
  shard_buffer_t () {};
  shard_buffer_t(size_t n): chunk_log(n) {};
  int set(size_t idx, const char* buf, size_t bufsz);
  int get(size_t idx,       char* buf, size_t &bufsz);
  int app(size_t idx, const char* buf, size_t bufsz);
  int del(size_t idx);
  int del(size_t idx, size_t offset, size_t sz);
  int push_back(const char* buf, size_t bufsz);
  int serialize();
};



