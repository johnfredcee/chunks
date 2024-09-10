
#include <stdio.h>
#include <stdbool.h>
#include <stdalign.h>
 
#define CHUNKS_IMPLEMENTATION
#include "chunks.h"
#define UTEST_C_IMPLEMENTATION
#include "utest.h"

DECLARE_TEST(single_chunk);
DECLARE_TEST(three_chunks);
DECLARE_TEST(two_subchunks);

TEST(single_chunk)
{
	uint8_t* data = malloc(16);
	for(int i = 0; i < 16; i++)
	{
		((uint8_t*) data)[i] = i;
	}
	t_CHUNK* chunk = make_chunk(data, 16);
	free(data);
	chunk->id = 0x1111;
	write_chunked("singlechunk.bin", chunk);
	free(chunk);
	chunk = load_chunked("singlechunk.bin");
	TEST_ASSERT(chunk != NULL);
	TEST_ASSERT(chunk->id = 0x1111);
	data = chunk_data(chunk);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(chunk->size == 16);
	for(int i = 0; i < 16; i++)
	{
		TEST_ASSERT(data[i] == i);
	}
}

TEST(three_chunks)
{
	uint8_t* data = malloc(16);
	for(int i = 0; i < 16; i++)
	{
		((uint8_t*) data)[i] = i;
	}
	t_CHUNK* chunk = make_chunk(data, 16);
	chunk->id = 0x1111;

	uint8_t* data1 = malloc(16);
	for(int i = 16; i < 32; i++)
	{
		((uint8_t*) data1)[i-16] = i;
	}
	t_CHUNK* chunk1 = make_chunk(data1, 16);
	chunk1->id = 0x2222;
	add_chunk(chunk, chunk1);
	
	uint8_t* data2 = malloc(16);
	for(int i = 0; i < 16; i++)
	{
		((uint8_t*) data2)[i] = 15-i;
	}
	t_CHUNK* chunk2 = make_chunk(data2, 16);
	chunk1->id = 0x3333;
	add_chunk(chunk, chunk2);

	free(data);
	free(data1);
	free(data2);
	write_chunked("threechunk.bin", chunk);
	free_chunk(chunk);
	chunk = load_chunked("threechunk.bin");
	TEST_ASSERT(chunk != NULL);

	TEST_ASSERT(chunk->id = 0x1111);
	data = chunk_data(chunk);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(chunk->size == 16);
	for(int i = 0; i < 16; i++)
	{
		TEST_ASSERT(data[i] == i);
	}

	chunk=chunk->next;
	TEST_ASSERT(chunk->id = 0x2222);
	data = chunk_data(chunk);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(chunk->size == 16);
	for(int i = 0; i < 16; i++)
	{
		TEST_ASSERT(data[i] == i+16);
	}
	chunk=chunk->next;

	TEST_ASSERT(chunk->id = 0x3333);
	data = chunk_data(chunk);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(chunk->size == 16);
	for(int i = 0; i < 16; i++)
	{
		TEST_ASSERT(data[i] == 15-i);
	}

}

TEST(two_subchunks)
{
	uint8_t* data = malloc(16);
	for(int i = 0; i < 16; i++)
	{
		((uint8_t*) data)[i] = i;
	}
	t_CHUNK* chunk = make_chunk(data, 16);
	chunk->id = 0x1111;

	uint8_t* data1 = malloc(16);
	for(int i = 16; i < 32; i++)
	{
		((uint8_t*) data1)[i-16] = i;
	}
	t_CHUNK* chunk1 = make_chunk(data1, 16);
	chunk1->id = 0x2222;
	add_subchunk(chunk, chunk1);
	
	uint8_t* data2 = malloc(16);
	for(int i = 0; i < 16; i++)
	{
		((uint8_t*) data2)[i] = 15-i;
	}
	t_CHUNK* chunk2 = make_chunk(data2, 16);
	chunk1->id = 0x3333;
	add_subchunk(chunk, chunk2);

	free(data);
	free(data1);
	free(data2);
	write_chunked("threechunk.bin", chunk);
	free_chunk(chunk);

	chunk = load_chunked("threechunk.bin");
	TEST_ASSERT(chunk != NULL);

	TEST_ASSERT(chunk->id = 0x1111);
	data = chunk_data(chunk);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(chunk->size == 16);
	for(int i = 0; i < 16; i++)
	{
		TEST_ASSERT(data[i] == i);
	}

	chunk=chunk->subchunk;
	TEST_ASSERT(chunk->id = 0x2222);
	data = chunk_data(chunk);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(chunk->size == 16);
	for(int i = 0; i < 16; i++)
	{
		TEST_ASSERT(data[i] == i+16);
	}
	chunk=chunk->next;

	TEST_ASSERT(chunk->id = 0x3333);
	data = chunk_data(chunk);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(chunk->size == 16);
	for(int i = 0; i < 16; i++)
	{
		TEST_ASSERT(data[i] == 15-i);
	}

}

int main(int argc, char* argv[])
{
	utest_init();
	int result = TEST_RUN(single_chunk);
	result = result + TEST_RUN(three_chunks);
	result = result + TEST_RUN(two_subchunks);
	if (result == 0)
	{
		printf("All good.\n");
	}
	return result;
} 