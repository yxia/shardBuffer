#include "shardBuf.hpp"

int shard_buffer_t::set(size_t idx, const char* userbuf, size_t userbuf_sz) {
  CHECK_OUT_RANGE(idx);

  // wait until no pending reader/writer 
  if (is_tx_mode) {
    short expect  = 0;
    while (!dyn_buf[idx].rw_flag.compare_exchange_weak(expect, WR_MODE))
      expect = 0;
  }

  entries[idx].chunk_id = 0;
  entries[idx].chunk_offset = 0;
  entries[idx].data_sz = 0;
  dyn_buf[idx].data.append(userbuf, userbuf_sz);
  
  // reset the flag to allow read/write
  if (is_tx_mode) dyn_buf[idx].rw_flag.store(0);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::get(size_t idx, char* userbuf, size_t &userbuf_sz) {
  CHECK_OUT_RANGE(idx);
  
  // wait if in write (need revision)
  if (is_tx_mode) {
    short expect = WR_MODE; // write flag
    while(dyn_buf[idx].rw_flag.compare_exchange_strong(expect, WR_MODE))
      expect = WR_MODE;
    dyn_buf[idx].rw_flag++;
  }

  size_t tot_data_sz = entries[idx].data_sz + dyn_buf[idx].data.size();
  if (tot_data_sz > userbuf_sz)
    return _SHARDBUF_OVERFLOW;

  userbuf_sz = 0;
  if (entries[idx].data_sz > 0) {
    size_t chunk_id     = entries[idx].chunk_id;
    size_t chunk_offset = entries[idx].chunk_offset; 
    size_t chunk_datasz = entries[idx].data_sz;
    memcpy(userbuf, &(chunks[chunk_id][chunk_offset]), chunk_datasz);
  }
  if (dyn_buf[idx].data.size() > 0) {
    memcpy(userbuf+userbuf_sz, dyn_buf[idx].data.data(), dyn_buf[idx].data.size());
    userbuf_sz += dyn_buf[idx].data.size();
  }
    
  if(is_tx_mode) dyn_buf[idx].rw_flag--;
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::app(size_t idx, const char* userbuf, size_t userbuf_sz) {
  CHECK_OUT_RANGE(idx);

  if (is_tx_mode) {
    short expect = 0;
    while (!dyn_buf[idx].rw_flag.compare_exchange_weak(expect, WR_MODE))
      expect = 0;
  }

  dyn_buf[idx].data.append(userbuf, userbuf_sz);

  if (is_tx_mode) dyn_buf[idx].rw_flag.store(0);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::del(size_t idx) {
  CHECK_OUT_RANGE(idx); 
  if (is_tx_mode) {
    short expect = 0;
    while (!dyn_buf[idx].rw_flag.compare_exchange_weak(expect, WR_MODE))
      expect = 0;
  }

  entries[idx].data_sz = 0;
  dyn_buf[idx].data.clear();

  if (is_tx_mode) dyn_buf[idx].rw_flag.store(0);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::push_back(const char* userbuf, size_t userbuf_sz) {


  // lock
  entry_t e;
  e.chunk_id = 0;
  e.chunk_offset = 0;
  e.data_sz = 0;

  dyn_buf_t b;
  b.data = string(userbuf, userbuf_sz);

  entries.push_back(e);
  dyn_buf.push_back(b);


  // unlock

  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::serialize() {
  vector<string> new_chunks(1);
  new_chunks.reserve(chunks.size()*2+1);

  size_t new_chunk_id  = 0; 
  size_t new_chunk_offset = 0;
  size_t new_chunk_datasz = 0;
  for (size_t i=0; i<entries.size(); i++) {
    // lock
    if (is_tx_mode) {
      short expect = 0;
      while (!dyn_buf[i].rw_flag.compare_exchange_weak(expect, WR_MODE))
	expect = 0;
    }

    size_t chunk_id      = entries[i].chunk_id;
    size_t chunk_offset  = entries[i].chunk_offset;
    size_t chunk_datasz  = entries[i].data_sz;

    entries[i].chunk_id     = new_chunk_id;
    entries[i].chunk_offset = new_chunk_offset;
    new_chunk_datasz = 0;

    if (chunk_datasz > 0) {
      new_chunks[new_chunk_id].append(chunks[chunk_id][chunk_offset], chunk_datasz);
      new_chunk_datasz += chunk_datasz;
    }
    if (dyn_buf[i].data.size() > 0) {
      new_chunks[new_chunk_id].append(dyn_buf[i].data);
      new_chunk_datasz += dyn_buf[i].data.size();
      dyn_buf[i].data.clear();
    }

    entries[i].data_sz = new_chunk_datasz;
    new_chunk_offset += new_chunk_datasz;      
    if ( (new_chunk_offset >> chunk_log) > 0) {
      new_chunk_id++;
      new_chunk_offset = 0;
    }
    if (new_chunk_id >= new_chunks.size())
      new_chunks.resize(new_chunk_id+1);

    // unlock
    if (is_tx_mode) dyn_buf[i].rw_flag.store(0);
  }
  chunks = move(new_chunks);
}

void shard_buffer_t::dump() {
  cout << "---- entries ----\n";
  for (size_t i=0; i<entries.size(); i++) {
    cout << "entry (" << entries[i].chunk_id <<"," 
	 << entries[i].chunk_offset << ","
	 << entries[i].data_sz 
	 << "): [" 
	 << dyn_buf[i].data << "]\n";
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
  shard_buffer_t buf(9);

  ifstream f("../data/sample.txt", ios_base::in);
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

  cout << "log of chunk_size: " << buf.get_chunk_size() << endl;

  return 0;
}
#endif

