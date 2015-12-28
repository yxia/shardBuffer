#include "shardBuffer.hpp"

int shard_buffer_t::set(size_t idx, const char* userbuf, size_t userbuf_sz) {
  CHECK_OUT_RANGE(idx);
  entries[idx] = make_pair(0,0);
  growbuf[idx] = string(userbuf, userbuf_sz);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::get(size_t idx, char* userbuf, size_t &userbuf_sz) {
  CHECK_OUT_RANGE(idx);
  size_t chunk_loc = entries[idx].first;
  size_t chunk_sz  = entries[idx].second;
  size_t data_sz   = chunk_sz + growbuf[idx].size();
  if (data_sz > userbuf_sz)
    return _SHARDBUF_OVERFLOW;

  userbuf_sz = 0;
  if (chunk_sz > 0) {
    size_t chunk_idx    = GET_CHUNK_INDEX(chunk_loc, chunk_log); 
    size_t chunk_offset = GET_CHUNK_OFFSET(chunk_loc, chunk_log);
    memcpy(userbuf, &(chunks[chunk_idx][chunk_offset]), chunk_sz);
    userbuf_sz += chunk_sz;
  }

  if (growbuf[idx].size() > 0) {
    memcpy(userbuf+userbuf_sz, growbuf[idx].data(), growbuf[idx].size());
    userbuf_sz += growbuf[idx].size();
  }
  return _SHARDBUF_SUCCEED; 
}

int shard_buffer_t::app(size_t idx, const char* userbuf, size_t userbuf_sz) {
  CHECK_OUT_RANGE(idx);
  growbuf[idx].append(userbuf, userbuf_sz);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::del(size_t idx) {
  CHECK_OUT_RANGE(idx);
  entries[idx].first = 0;
  entries[idx].second = 0;
  growbuf[idx].clear();
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::del(size_t idx, size_t offset, size_t sz) {
  CHECK_OUT_RANGE(idx);
  size_t chunk_loc = entries[idx].first;
  size_t chunk_sz  = entries[idx].second;

  size_t chunk_idx    = GET_CHUNK_INDEX(chunk_loc, chunk_log); 
  size_t chunk_offset = GET_CHUNK_OFFSET(chunk_loc, chunk_log);

  if (offset+sz <= chunk_sz) { // delete within the chunk
    chunks[chunk_idx].erase(chunk_offset+offset, sz);
    entries[idx] = make_pair(chunk_loc, chunk_sz-sz);
  } else if (offset <= chunk_sz) { // delete in both
    size_t delta = offset+sz - chunk_sz;
    chunks[chunk_idx].erase(chunk_offset+offset, sz-delta);
    growbuf[idx].erase(0, delta);
  } else if (offset+sz-chunk_sz < growbuf[idx].size()) { // delete in growbuf only
    offset -= chunk_sz;
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
  vector<string> new_chunks(1);
  new_chunks.reserve(chunks.size()*2+1);

  size_t tot_data_sz = 0;
  size_t new_chunk_idx = 0; 
  for (size_t i=0; i<entries.size(); i++) {
    size_t chunk_loc = entries[i].first;                                                                          
    size_t chunk_sz  = entries[i].second;
    size_t data_sz = 0;
    if (chunk_sz > 0) {
      size_t chunk_idx    = GET_CHUNK_INDEX(chunk_loc, chunk_log);       
      size_t chunk_offset = GET_CHUNK_OFFSET(chunk_loc, chunk_log);
      new_chunks[new_chunk_idx].append(chunks[chunk_idx][chunk_offset], chunk_sz);
      data_sz += chunk_sz;
    }
    if (growbuf[i].size() > 0) {
      new_chunks[new_chunk_idx].append(growbuf[i]);
      data_sz += growbuf[i].size();
      growbuf[i].clear();
    }

    entries[i] = make_pair(tot_data_sz, data_sz);
    tot_data_sz += data_sz;
    
    size_t t = GET_CHUNK_INDEX(tot_data_sz, chunk_log);
    new_chunk_idx = (t>new_chunk_idx)? t: new_chunk_idx;
    if (new_chunk_idx >= new_chunks.size())
      new_chunks.resize(new_chunk_idx+1);
  }

  chunks = move(new_chunks);
}



void shard_buffer_t::dump() {
  cout << "---- entries ----\n";
  for (size_t i=0; i<entries.size(); i++) {
    cout << "entry (" << entries[i].first <<"," 
	 << entries[i].second << "): [" 
	 << growbuf[i] << "]\n";
  }
  
  cout << "---- chunks ----\n";
  for (size_t i=0; i<chunks.size(); i++) {
    cout << "chunk-" << i << ": ["
	 << chunks[i] << "]\n";
  }

  cout <<"\n";
}


#ifdef DEBUG
int main() {
  shard_buffer_t buf;

  ifstream f("sample.txt", ios_base::in);
  if (!f.is_open()) {
    cerr << "file can not open" << endl;
    return -1;
  }

  while (f.good()) {
    string line;
    getline(f, line);

    buf.push_back(line.c_str(), line.size());
  }
  f.close();

  buf.dump();

  buf.serialize();
  buf.dump();


  return 0;
}
#endif
