# Multi-shard Buffer and Storage

## Goal

This project implements a high performance multiple sharded buffer, a useful data structure that can be used for building various high performance applications and data storages.

The multiple sharded buffer is basically a parallel data structure mimic the interfaces of regular array or vector operations, as introduced in the following section, where a user can declare an array, dynamically resize or erease it (in this sense, it is more like the `std::vector`), and append data to an element of the array. Wherever an `array` or a `std::vector` can be used, usually the multiple sharded buffer can be used either. 

The mulitple sharded array brings features that a conventional array or `std::vector` may not always have. These features include:

  -- Multiple shards: Each shard is a like a segment of the array. Thus, it is sort of similar to a `std::vector<std::vector<buffer_type> >`. Each shard can be handled by a thread in concurrent applications.
  -- Quick persistence: Each shard can be viewed as a contiguous buffer in memory, making it highly efficient for persistence or loading from disks. It is also efficient to be replicated for recovery.
  -- Partitioning: The shard itself is a natural way for horizontally partition the data; besides, vertical partitioning can also be implmenet when using mulitple multi-shard buffers together.
  -- Blob-compability: Each element in the multi-shard array can be a buffer of fixed size or variant size. When it is a variant size, it can be dynamically resize, resulting a highly flexible storage
  -- MVCC: each shard/element can be in multipel versions.

## Interface

The basic interfaces for the multiple sharded buffer include:

  -- Declare a multi-shard buffer and decide the size of shards, where `n` is the number of element in a shard

```
    multishard_array_t arr(size_t n)
```

  -- Check the number of elements

```
    size_t size()
```  

  -- Reserve spaces, where `m` is the number of elements to be reserved. It returns an `int` as an error code, similar to all the following interfaces.
  
```
    int reserve(size_t m)
```    

  -- Push back an element to a multi-shard buffer

```
    int  push_back(char*buf, size_t bufsz)
```

  - Assign a buffer to an index

```
    int set(size_t index, char* buf, size_t bufsz) 
```

  - Read a buffer out. The input is a buffer address provided by the user and its capacity; the actual size loaded overwrite the bufsz. 
  
```
    int get(size_t index, char* buf, size_t *bufsz)
```    

  - Append a buffer to the end of an existing element
  
```
    int append(size_t index, char* buf, size_t bufsz)
```  

  - Serialize buffers in parallel into a compact form
  
```
    int serialize()
```

## Design

The multi-shard buffer storage mainly consists of the *entries* and a set of *chunks*. The *entries* are a vector, where each element stores the offset and size of a multi-shard buffer element, as well as an optionally a set of *growing buffers*. The element of the entry vector is of the following type:

```cpp
    typedef struct {
      size_t idx;
      size_t sz;
      string buf;
    }
```
The `idx` is an offset in the corresponding *chunck* and the `sz` corresponds to the size in terms of numbers of bytes. The `buf` is a string (i.e. a buffer) initialized as an empty `std::string`. Note that a chunck shard is a buffer block of a certain size, say 16MB. 

Each *chunk* is a block of contiguous memory space, typically in a large size, featured by highly efficient loading, persisting, and duplication. Usually a chunk contains multiple elements from the multi-shard buffer, one followed another compactly. The growing buffers are reserved for modification of elements in chunks that exceed the space currently taken up in the chunk. 

If we start with an empty multi-shard buffer, then an new element, say `t`, always has the initial values as `t.{idx=0,sz=0,buf=""}`. When user assign a buffer, say a string `s`, to the multi-shard array, `s` is copied to `t`. This is not much different from `std::vector<string>`, except its multi-sharding. The really difference comes when the `serialize()` is called, either explictly or implicitly, where the serialization concatenates all data in the strings to the chunks. While the chunk is populated, teh index and size in the *entries* are updated accordingly. Note the operation within a chunk is straightforward and it is independent for different chunks.

After the serialization, the multi-shard can be efficiently persisted, as it becomes a set of arrays (the growing buffers are all empty and therefore skipped). After loading from disk, the format remains as the same as that after serialization. 

