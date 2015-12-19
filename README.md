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

## Design


