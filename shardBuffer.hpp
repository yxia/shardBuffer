#include <iostream>
#include <string>
#include <algorithm>

#define _SHARDBUF_SUCCEED   0
#define _SHARDBUF_OUT_RANGE 2
#define _SHARDBUF_OVERFLOW  3


#define CHECK_OUT_RANGE (idx) { if (idx > entries.size()) return _SHARDBUF_OUT_RANGE; }

using namespace std;

class shard_buffer_t {
  unsigned int chunk_log = 20;  // chunk_size = 2^20 = 1M
  vector<string> chunks;
  vector<string> growbuf;
  vector<pair<size_t, size_t> > entries;

public:
  shard_buffer_t () {};
  shard_buffer_t(size_t n): chunk_log(n) {};
  int set(size_t idx, char* buf, size_t bufsz);
  int get(size_t idx, char* buf, size_t &bufsz);
  int app(size_t idx, char* buf, size_t bufsz);
  int del(size_t idx);
  int push_back(char* buf, size_t bufsz);
  int serialize();
};

int shard_buffer_t::set(size_t idx, char* buf, size_t bufsz) {
  CHECK_OUT_RANGE(idx);
  growbuf[idx] = string(buf, bufsz);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::get(size_t idx, char* buf, size_t &bufsz) {
  CHECK_OUT_RANGE(idx);
  size_t chunk_offset = entries[idx].first;
  size_t chunk_datasz = entries[idx].second
  size_t valsz = chunk_datasz + growbuf[idx];
  if (valsz > bufsz)
    return _SHARDBUF_OVERFLOW;

  bufsz = 0;
  if (chunk_datasz > 0) {
    size_t chunk_idx = idx >> chunk_log;
    memcpy(buf, &chunks[chunk_idx][chunk_offset], chunk_datasz);
    bufsz += chunk_datasz;
  }

  if (growbuf[idx].size() > 0) {
    memcpy(buf+bufsz, growbuf[idx].data(), growbuf[idx].size());
    bufsz += growbuf[idx].size();
  }
  return _SHARDBUF_SUCCEED; 
}

int shard_buffer_t::app(size_t idx, char* buf, size_t bufsz) {
  CHECK_OUT_RANGE(idx);

}
