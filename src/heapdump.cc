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

#if NODE_MODULE_VERSION<=11
    using v8::Arguments;
    using v8::False;
#else
    using v8::FunctionCallbackInfo;
#endif
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::HeapProfiler;
using v8::HeapSnapshot;
using v8::Isolate;
using v8::Object;
using v8::OutputStream;
using v8::String;
using v8::True;
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

#if NODE_MODULE_VERSION<=11
    Handle<Value> WriteSnapshot(const Arguments& args)
#else
    void WriteSnapshot(const FunctionCallbackInfo<Value>& args)
#endif
{
  Isolate* isolate = args.GetIsolate();

#if NODE_MODULE_VERSION>11
  HandleScope handle_scope(isolate);
#endif

  bool ok;
  if (args[0]->IsString()) {
    String::Utf8Value filename(args[0]);
    ok = heapdump::WriteSnapshot(isolate, *filename);
  } else {
    ok = heapdump::WriteSnapshot(isolate, NULL);
  }

  // Throwing on error is too disruptive, just return a boolean indicating
  // success.
#if NODE_MODULE_VERSION<=11
  return ok ? True() : False();
#else
  args.GetReturnValue().Set(ok);
#endif
}

void Init(Handle<Object> obj)
{
  Isolate* isolate = 0;
#if NODE_MODULE_VERSION<=11
  HandleScope scope;
#else
  isolate = obj->CreationContext()->GetIsolate();
#endif

  heapdump::PlatformInit(isolate);

  obj->Set(
#if NODE_MODULE_VERSION<=11
  String::New("writeSnapshot"),
#else
    String::NewFromUtf8(isolate, "writeSnapshot"),
#endif

    FunctionTemplate::New(WriteSnapshot)->GetFunction()
  );
}

NODE_MODULE(heapdump, Init)

} // namespace anonymous


namespace heapdump
{

bool WriteSnapshotHelper(Isolate* isolate, const char* filename)
{
  char scratch[256];
  if (filename == NULL) {
    uint64_t now = uv_hrtime();
    unsigned long sec = static_cast<unsigned long>(now / 1000000);
    unsigned long usec = static_cast<unsigned long>(now % 1000000);
    snprintf(scratch,
             sizeof(scratch),
             "heapdump-%lu.%lu.heapsnapshot",
             sec,
             usec);
    filename = scratch;
  }

  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    return false;
  }

  const HeapSnapshot* snap =
#if NODE_MODULE_VERSION<=11
    HeapProfiler::TakeSnapshot(String::Empty());
#else
    isolate->GetHeapProfiler()->TakeHeapSnapshot(String::Empty());
#endif

  FileOutputStream stream(fp);
  snap->Serialize(&stream, HeapSnapshot::kJSON);
  fclose(fp);

  //emil-mi(02/01/14):Snapshots are retained so they increase the memory footprint of the process. This is apparent in
  //Windows since the process continues running after taking a snapshot as opposed to Linux where the process is forked,
  //a snapshot is created and then the process dies freeing all memory.

  //The const_cast is ok per http://v8.googlecode.com/svn/branches/3.5/test/cctest/test-heap-profiler.cc where in
  //DeleteHeapSnapshot they use this pattern copiously. The alternative is HeapProfiler::DeleteAllSnapshots() but
  //that might interfere with snapshots taken from other modules
  const_cast<v8::HeapSnapshot*>(snap)->Delete();

  return true;
}

} // namespace heapdump
