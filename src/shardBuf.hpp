#include <string.h>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>

#define WR_MODE -1
#define RD_MODE 1
#define _SHARDBUF_SUCCEED   0
#define _SHARDBUF_OUT_RANGE 2
#define _SHARDBUF_OVERFLOW  3

#define CHECK_OUT_RANGE(n) {if((n) >= entries.size()) return _SHARDBUF_OUT_RANGE;}


using namespace std;

typedef struct {
  size_t chunk_id = 0;
  size_t chunk_offset = 0;
  size_t data_sz = 0;
} entry_t;

class dyn_buf_t {
public:
  atomic<short> rw_flag;
  string data;

  dyn_buf_t() {
    rw_flag = 0;
  }
  dyn_buf_t(const dyn_buf_t& rhs) {
    data = rhs.data;
  }
};

class shard_buffer_t {
private:
  vector<entry_t>      entries;
  vector<dyn_buf_t>    dyn_buf;
  vector<string>       chunks;

  unsigned int         chunk_log = 8;  // 2^8 = 256 Bytes 
  bool                 is_tx_mode = false;

public:
  shard_buffer_t () {};
  shard_buffer_t (unsigned int n): chunk_log(n) {};
  
  int set(size_t idx, const char* buf, size_t bufsz);
  int get(size_t idx,       char* buf, size_t&bufsz);
  int app(size_t idx, const char* buf, size_t bufsz);
  int del(size_t idx);
  
  int push_back(const char* buf, size_t bufsz);
  int serialize();
  void dump();

  unsigned int get_chunk_size() { return chunk_log; }

};
