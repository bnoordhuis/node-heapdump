/*
 * Copyright (c) 2012, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "node.h"
#include "node_version.h"

#include "uv.h"
#include "v8.h"
#include "v8-profiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

#include <unistd.h>
#include <sys/time.h>

namespace {

using namespace v8;

uv_async_t async_handle;

class FileOutputStream: public OutputStream
{
public:
  FileOutputStream(FILE* stream): stream_(stream)
  {
  }

  virtual int GetChunkSize()
  {
    return 65536; // big chunks == faster
  }

  virtual void EndOfStream()
  {
  }

  virtual WriteResult WriteAsciiChunk(char* data, int size)
  {
    const size_t len = static_cast<size_t>(size);
    size_t off = 0;

    while (off < len && !feof(stream_) && !ferror(stream_))
      off += fwrite(data + off, 1, len - off, stream_);

    return off == len ? kContinue : kAbort;
  }

private:
  FILE* stream_;
};

void SignalHandler(int signum)
{
  if (uv_async_send(&async_handle)) abort();
}

void AsyncEvent(uv_async_t* handle, int status)
{
  assert(handle == &async_handle);
  if (fork() != 0) return;

  timeval tv;
  if (gettimeofday(&tv, NULL)) abort();

  char filename[256];
  snprintf(filename,
           sizeof(filename),
           "heapdump-%u.%u.log",
           static_cast<unsigned int>(tv.tv_sec),
           static_cast<unsigned int>(tv.tv_usec));
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) abort();

  HeapStatistics stats;
  V8::GetHeapStatistics(&stats);
  fprintf(fp, "version :1.0\n");
#define X(attr) fprintf(fp, #attr ": %zu\n", stats.attr())
  X(total_heap_size);
  X(total_heap_size_executable);
  X(used_heap_size);
  X(heap_size_limit);
#undef X
  fclose(fp);

  snprintf(filename,
           sizeof(filename),
           "heapdump-%u.%u.json",
           static_cast<unsigned int>(tv.tv_sec),
           static_cast<unsigned int>(tv.tv_usec));
  fp = fopen(filename, "w");
  if (fp == NULL) abort();

  const HeapSnapshot* snap = HeapProfiler::TakeSnapshot(String::Empty());
  FileOutputStream stream(fp);
  snap->Serialize(&stream, HeapSnapshot::kJSON);
  fclose(fp);

  _exit(42);
}

void Init(Handle<Object> obj)
{
  HandleScope scope;

  if (uv_async_init(uv_default_loop(), &async_handle, AsyncEvent)) abort();
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION == 6
  uv_unref(uv_default_loop());
#else
  uv_unref((uv_handle_t*)&async_handle);
#endif

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SignalHandler;
  if (sigaction(SIGUSR2,  &sa, NULL)) abort();
}

NODE_MODULE(heapdump, Init)

} // namespace anonymous
