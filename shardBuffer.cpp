#include "shardBuffer.hpp"

int shard_buffer_t::set(size_t idx, const char* buf, size_t bufsz) {
  CHECK_OUT_RANGE(idx);
  growbuf[idx] = string(buf, bufsz);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::get(size_t idx, char* buf, size_t &bufsz) {
  CHECK_OUT_RANGE(idx);
  size_t chunk_offset = entries[idx].first;
  size_t chunk_datasz = entries[idx].second;
  size_t valsz = chunk_datasz + growbuf[idx].size();
  if (valsz > bufsz)
    return _SHARDBUF_OVERFLOW;

  bufsz = 0;
  if (chunk_datasz > 0) {
    size_t chunk_idx = idx >> chunk_log;
    memcpy(buf, &(chunks[chunk_idx][chunk_offset]), chunk_datasz);
    bufsz += chunk_datasz;
  }

  if (growbuf[idx].size() > 0) {
    memcpy(buf+bufsz, growbuf[idx].data(), growbuf[idx].size());
    bufsz += growbuf[idx].size();
  }
  return _SHARDBUF_SUCCEED; 
}

int shard_buffer_t::app(size_t idx, const char* buf, size_t bufsz) {
  CHECK_OUT_RANGE(idx);
  growbuf[idx].append(buf, bufsz);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::del(size_t idx) {
  CHECK_OUT_RANGE(idx);
  //entries.erase(entries.begin()+idx);
  //growbuf.erase(growbuf.begin()+idx);
  entries[idx].first = 0;
  entries[idx].second = 0;
  growbuf[idx].clear();
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::del(size_t idx, size_t offset, size_t sz) {
  CHECK_OUT_RANGE(idx);
  size_t chunk_offset = entries[idx].first;
  size_t chunk_datasz = entries[idx].second;
  if (offset+sz <= chunk_datasz) { // delete within the chunk
    if (chunk_offset >= chunks.size()) return _SHARDBUF_OUT_RANGE;
    chunks[chunk_offset].erase(offset, sz);
  } else if (offset <= chunk_datasz) {
    return _SHARDBUF_OUT_RANGE;
  } else { // delete in growbuf
    offset -= chunk_datasz;
    growbuf[idx].erase(offset, sz);
  }
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::push_back(const char* buf, size_t bufsz) {
  entries.push_back(make_pair(0,0));
  growbuf.push_back(string(buf, bufsz));
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::serialize() {
  size_t num_new_chunks = (entries.size() >> chunk_log) + 1;
  vector<string> new_chunks(num_new_chunks);
  size_t new_chunk_idx = 0;
  for (size_t i=0; i<entries.size(); i++) {
    size_t chunk_index  = i >> chunk_log;
    size_t chunk_offset = entries[i].first;
    size_t chunk_datasz = entries[i].second;
   
    if (chunk_index <= new_chunks.size())
      new_chunks.resize(chunk_index+1);
    
    size_t new_chunk_offset = new_chunks[chunk_index].size();
    size_t new_chunk_datasz = chunk_datasz + growbuf[i].size();
    entries[i] = make_pair(new_chunk_offset, new_chunk_datasz);

    new_chunks[chunk_index].append(&chunks[chunk_index][chunk_offset], chunk_datasz);
    new_chunks[chunk_index].append(growbuf[i]);
    
    growbuf[chunk_index].clear();
  }
  chunks = move(new_chunks);
}

#ifdef DEBUG
int main() {
  shard_buffer_t buf;

  string str  = "hello ";
  string str2 = "world!";
  buf.push_back(str.c_str(), str.size());
  // buf.push_back(str2.c_str(), str2.size());
  buf.app(0, str2.c_str(), str2.size());

  char mybuf[1024];
  size_t sz = 1024;
  if (!buf.get(0, mybuf, sz))
    cout << "--> " << string(mybuf, sz) << endl;
  
  return 0;
}
#endif
