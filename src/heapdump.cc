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

#include "heapdump.h"

#include "node.h"
#include "uv.h"
#include "v8.h"
#include "v8-profiler.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

using v8::AccessorInfo;
using v8::Arguments;
using v8::DEFAULT;
using v8::DontDelete;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::HeapProfiler;
using v8::HeapSnapshot;
using v8::HeapStatistics;
using v8::Local;
using v8::Object;
using v8::OutputStream;
using v8::String;
using v8::Undefined;
using v8::V8;
using v8::Value;

namespace
{

#if defined(_WIN32)
// Emulate snprintf() on windows, _snprintf() doesn't zero-terminate the buffer
// on overflow.
int snprintf(char* buf, unsigned int len, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = _vsprintf_p(buf, len, fmt, ap);
  if (len) buf[len - 1] = '\0';
  va_end(ap);
  return n;
}
#endif

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

Handle<Value> WriteSnapshot(const Arguments& args)
{
  heapdump::WriteSnapshot();
  return Undefined();
}

char prefix[201] = "heapdump-";

Handle<Value> GetFilePrefix(Local<String> property, const AccessorInfo&) {
  return String::New(prefix);
}

void SetFilePrefix(Local<String> property, Local<Value> value,
                   const AccessorInfo&) {
  snprintf(prefix, sizeof(prefix), "%s", *String::AsciiValue(value));
}

void Init(Handle<Object> obj)
{
  HandleScope scope;
  heapdump::PlatformInit();
  obj->Set(String::New("writeSnapshot"),
           FunctionTemplate::New(WriteSnapshot)->GetFunction());
  obj->SetAccessor(String::New("filePrefix"),
           GetFilePrefix, SetFilePrefix, Handle<Value>(), DEFAULT, DontDelete);
}

NODE_MODULE(heapdump, Init)

} // namespace anonymous


namespace heapdump
{

void WriteSnapshotHelper()
{
  uint64_t now = uv_hrtime();
  unsigned long sec = static_cast<unsigned long>(now / 1000000);
  unsigned long usec = static_cast<unsigned long>(now % 1000000);

  char filename[256];
  snprintf(filename, sizeof(filename), "%s%lu.%lu.log", prefix, sec, usec);
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) return;

  HeapStatistics stats;
  V8::GetHeapStatistics(&stats);
  fprintf(fp, "version: 1.0\n");
#define X(attr)                                                               \
  fprintf(fp, #attr ": %lu\n", static_cast<unsigned long>(stats.attr()))
  X(total_heap_size);
  X(total_heap_size_executable);
  X(used_heap_size);
  X(heap_size_limit);
#undef X
  fclose(fp);

  snprintf(filename,
           sizeof(filename),
           "heapdump-%lu.%lu.heapsnapshot",
           sec,
           usec);
  fp = fopen(filename, "w");
  if (fp == NULL) return;

  const HeapSnapshot* snap = HeapProfiler::TakeSnapshot(String::Empty());
  FileOutputStream stream(fp);
  snap->Serialize(&stream, HeapSnapshot::kJSON);
  fclose(fp);
}

} // namespace heapdump
