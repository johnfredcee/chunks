#ifndef INCLUDE_CHUNKS_H
#define INCLUDE_CHUNKS_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

typedef struct t_CHUNK
{
    uint32_t id;    /* id of chunk - not used */
	uint32_t count; /* number of subchunks in chunk */
	size_t   size;  /* size of chunk + t_CHUNK */
    struct t_CHUNK *subchunk; /* first subchunk */
    struct t_CHUNK *next;  /* next sibling chunk */
} t_CHUNK;

// header size is first three fields of t_CHUNK - used when working out offsets to next chunk / subchunk
static const long header_size = (long)(sizeof(uint32_t) * 2 + sizeof(size_t));

t_CHUNK* alloc_chunk(size_t size);

t_CHUNK* make_chunk(void *data, size_t size);

void add_chunk(t_CHUNK* chunk, t_CHUNK* next_chunk);

void add_subchunk(t_CHUNK* chunk, t_CHUNK* sub_chunk);

t_CHUNK* get_last_chunk(t_CHUNK *chunk);

void close_chunked(FILE* fp, uint32_t count);

uint8_t *chunk_data(t_CHUNK *chunk);

t_CHUNK *next_chunk(t_CHUNK *chunk);

void write_chunked(const char *filename, t_CHUNK* head_chunk);

t_CHUNK* load_chunked(const char *filename);

void free_chunk(t_CHUNK* chunk);

#ifdef CHUNKS_IMPLEMENTATION

/* given a pointer to a chunk, return the actual data pointed to by a chunk */
uint8_t *chunk_data(t_CHUNK *chunk) {
    return ((uint8_t * )chunk) + sizeof(t_CHUNK); 
}

/* given a pointer to a chunk with a valid size field, calculate offset to end of chunk */
t_CHUNK *chunk_end_offset(t_CHUNK *chunk)
{
    size_t  size = chunk->size;
    uint8_t *data = (uint8_t *)chunk;
    return (t_CHUNK *)data + size;
}


/* allocate a chunk */
t_CHUNK* alloc_chunk(size_t size)
{
    t_CHUNK *chunk = malloc(sizeof(t_CHUNK) + size);
    chunk->size = size;
    chunk->count = 0;
    chunk->subchunk = NULL;
    chunk->next = NULL;
	chunk->id = 0xBADF00D;
    return chunk;
}

/* allocate a chunk with actual data of a given size */
t_CHUNK* make_chunk(void *data, size_t size)
{
    assert(data != NULL);
    /* create a chunk, add data */   
    t_CHUNK *chunk = alloc_chunk(size);
    memcpy(chunk_data(chunk), data, size);
    return chunk;
}

/* find the last chunk in a chain */
t_CHUNK* get_last_chunk(t_CHUNK *chunk)
{
    assert(chunk != NULL);
    t_CHUNK* final_chunk = chunk;
    while(final_chunk->next != NULL) {
        final_chunk = final_chunk->next;
    }
    return final_chunk;
}

/* find the last chunk in a chain */
t_CHUNK* get_last_subchunk(t_CHUNK *chunk)
{
    assert(chunk != NULL);
    t_CHUNK* subchunk = chunk->subchunk;
    return subchunk != NULL ? get_last_chunk(subchunk) : NULL;
}

/* get an (indexed) sub - chunk */
t_CHUNK* get_indexed_sub_chunk(t_CHUNK* chunk, uint32_t i)
{
    assert(chunk != NULL);
    assert(i < chunk->count);
    t_CHUNK* result = chunk->next;
    while ((i > 0) && (result != NULL))
    {
        result = result->subchunk;
        i--;
    }
    return result;
}

/* add a chunk to the end of the chain */
void add_chunk(t_CHUNK* chunk, t_CHUNK *next_chunk)
{
    t_CHUNK* final_chunk;
    assert(chunk != NULL);
    assert(next_chunk != NULL);
    final_chunk = get_last_chunk(chunk);
    assert(final_chunk != NULL);
    final_chunk->next = next_chunk;
    return;
}


void add_subchunk(t_CHUNK* chunk, t_CHUNK* subchunk) 
{
    assert(chunk != NULL);
    assert(subchunk != NULL);
    t_CHUNK* last_subchunk = get_last_subchunk(chunk);
    if (last_subchunk != NULL) {
        last_subchunk->next = subchunk;
    } else {
        chunk->subchunk = subchunk;
    };
    //chunk->size += sizeof(t_CHUNK) + subchunk->size;
    chunk->count++;
}

// returns number of chunks wtitten
uint32_t write_chunks(FILE *fp, t_CHUNK* chunk)
{
    uint32_t  count = 0;
    do {
        t_CHUNK write_chunk = *chunk;
        // make a note of the start of the chunk data
        long start = ftell(fp);
        // write the chunk
        fwrite(&write_chunk, sizeof(t_CHUNK), 1, fp);
        // write the data  of the chunk
        fwrite(chunk_data(chunk), write_chunk.size, 1, fp);
        // get the first subchunk
        t_CHUNK* subchunk = write_chunk.subchunk;
        long subchunk_pos = ftell(fp);
        if (subchunk != NULL) {
            write_chunk.subchunk = (t_CHUNK*) (ptrdiff_t) ftell(fp);
            write_chunk.count = write_chunks(fp, subchunk);
        }
        // make a note of the end, after all the subchunks
        long end = ftell(fp);        
        // now we have written the chunk and all subchunks we can work out actual size
        // write_chunk.size = (size_t) (end - start) - sizeof(t_CHUNK);
        
        t_CHUNK *next_chunk = chunk->next;
        // substitute offsedts for pointers - we subtract header size, because the loader will already have read past it
        write_chunk.next = (next_chunk != NULL) ? (t_CHUNK*) (ptrdiff_t) (end - header_size) : NULL;
        write_chunk.subchunk = (subchunk != NULL) ? (t_CHUNK*) (ptrdiff_t) (subchunk_pos - header_size) : NULL;
        // write the fixed - up chunk
        fseek(fp, start, SEEK_SET);
        fwrite(&write_chunk, sizeof(t_CHUNK), 1, fp);
        fseek(fp, end, SEEK_SET);
        // ensure pointers are restored
        chunk = next_chunk;
        count++;
    } while (chunk != NULL);
    return count;
}


/* write the header of a chunked file to disk */
void write_chunked_file_header(FILE* fp, uint32_t id, size_t size, uint32_t count)
{
    fwrite(&id, sizeof(uint32_t), 1, fp);
    fwrite(&size, sizeof(size_t), 1, fp);
    fwrite(&count, sizeof(uint32_t), 1, fp);
    return;
}

/* close and write size and count to head chunk */
void close_chunked(FILE* fp, uint32_t count)
{
    size_t size = (size_t) ( ftell(fp) - header_size );
    fseek(fp, 0, SEEK_SET);
    write_chunked_file_header(fp, 'MCHK', size, count);
    fclose(fp);
}

/* begin writing a chunked file */
void write_chunked(const char *filename, t_CHUNK* head_chunk)
{
    assert(filename != NULL);
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
        return;
    write_chunked_file_header(fp, 'MCHK', sizeof(uint32_t) * 2 + sizeof(size_t), 0);
    uint32_t count = write_chunks(fp, head_chunk);
    close_chunked(fp, count);
}



/* Loading is distinct from reading in that it is for game-side purposes to read data into memory. 
   No further processing of the data is expected. */


void fixup_chunks(void *head, t_CHUNK *tofixup)
{
    while (tofixup != NULL)
    {
        ptrdiff_t subchunk = (ptrdiff_t) tofixup->subchunk;
        tofixup->subchunk = subchunk != 0 ? (t_CHUNK*) (subchunk + (uint8_t*) head) : NULL;
        if (subchunk != 0) {
            fixup_chunks(head,tofixup->subchunk);
        }    
        ptrdiff_t next = (ptrdiff_t) tofixup->next;
        tofixup->next = (next != 0) ? (t_CHUNK*)  (next + (uint8_t*) head) : NULL;
        tofixup = tofixup->next;
    }
    return;
}

t_CHUNK* load_chunked(const char *filename)
{
    uint32_t id, count;
    size_t size;
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return NULL;
    fread(&id, sizeof(uint32_t), 1, fp);
    if (id != (uint32_t) 'MCHK')
    {
        fclose(fp);
        return NULL;
    }
    fread(&size, sizeof(size_t), 1, fp);
    fread(&count, sizeof(uint32_t), 1, fp);
    void* inmem = malloc(size);
    fread(inmem, 1, size, fp);
    fclose(fp);
    t_CHUNK* chunk = (t_CHUNK*) inmem;    
    fixup_chunks(inmem,chunk);
    return (t_CHUNK*) inmem;
};

void free_chunk(t_CHUNK* chunk)
{
    if (chunk->next != NULL)
        free_chunk(chunk->next);
    free(chunk);
    return;
}

#endif /* implmenentation */

#endif /* header guard */