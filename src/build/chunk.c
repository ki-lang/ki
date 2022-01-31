
#include "../all.h"

ContentChunk* init_content_chunk() {
  ContentChunk* chunk = malloc(sizeof(ContentChunk));
  chunk->fc = NULL;
  chunk->i = 0;
  return chunk;
}

void free_chunk(ContentChunk* chunk) {
  //
  free(chunk);
}

ContentChunk* content_chunk_pop(Array* chunks) {
  if (chunks->length == 0) {
    return NULL;
  }
  ContentChunk* cc = array_pop(chunks);
  return cc;
}

ContentChunk* content_chunk_create_for_fc(Array* chunks, FileCompiler* fc) {
  ContentChunk* cc = malloc(sizeof(ContentChunk));
  cc->fc = fc;
  cc->i = fc->i;
  array_push(chunks, cc);
  return cc;
}
