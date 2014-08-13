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

namespace
{

using v8::Context;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::HeapSnapshot;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::OutputStream;
using v8::Persistent;
using v8::String;
using v8::V8;
using v8::Value;
using heapdump::on_complete_callback;

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

COMPAT(Handle<Value> WriteSnapshot(const v8::Arguments& args),
       void WriteSnapshot(const v8::FunctionCallbackInfo<Value>& args))
{
  Isolate* isolate = Isolate::GetCurrent();
  COMPAT(HandleScope handle_scope,
         HandleScope handle_scope(isolate));

  bool ok;
  Local<Value> maybe_function = args[0];
  if (1 < args.Length()) {
    maybe_function = args[1];
  }

  if (maybe_function->IsFunction()) {
    Local<Function> function = maybe_function.As<Function>();
    COMPAT(on_complete_callback = Persistent<Function>::New(function),
           on_complete_callback.Reset(isolate, function));
  }

  if (args[0]->IsString()) {
    String::Utf8Value filename(args[0]);
    ok = heapdump::WriteSnapshot(isolate, *filename);
  } else {
    ok = heapdump::WriteSnapshot(isolate, NULL);
  }

  // Throwing on error is too disruptive, just return a boolean indicating
  // success.
  COMPAT(return v8::Boolean::New(ok),
         args.GetReturnValue().Set(ok));
}

void Init(Handle<Object> obj)
{
  Isolate* isolate = Isolate::GetCurrent();
  heapdump::PlatformInit(isolate);
  Local<String> key = COMPAT(String::New("writeSnapshot"),
                             String::NewFromUtf8(isolate, "writeSnapshot"));
  Local<FunctionTemplate> t =
      COMPAT(FunctionTemplate::New(WriteSnapshot),
             FunctionTemplate::New(isolate, WriteSnapshot));
  obj->Set(key, t->GetFunction());
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

  const HeapSnapshot* snap = COMPAT(
      v8::HeapProfiler::TakeSnapshot(String::Empty()),
      isolate->GetHeapProfiler()->TakeHeapSnapshot(String::Empty(isolate)));
  FileOutputStream stream(fp);
  snap->Serialize(&stream, HeapSnapshot::kJSON);
  fclose(fp);
  // Only necessary on Windows because the snapshot is created inside the
  // main process.  On UNIX platforms, it's created from a fork that exits
  // after writing the snapshot to disk.
#if defined(_WIN32)
  const_cast<HeapSnapshot*>(snap)->Delete();
#endif
  return true;
}

void InvokeCallback()
{
  if (!on_complete_callback.IsEmpty()) COMPAT({
      HandleScope handle_scope;
      Local<Function> callback = *on_complete_callback;
      node::MakeCallback(Context::GetCurrent()->Global(), callback, 0, NULL);
      on_complete_callback.Dispose();
    }, {
      Isolate* isolate = Isolate::GetCurrent();
      HandleScope handle_scope(isolate);
      Local<Function> callback =
          Local<Function>::New(isolate, on_complete_callback);
      node::MakeCallback(isolate,
                         isolate->GetCurrentContext()->Global(),
                         callback,
                         0,
                         NULL);
      on_complete_callback.Reset();
    }
  );
}

} // namespace heapdump
