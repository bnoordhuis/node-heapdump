// Copyright (c) 2012, Ben Noordhuis <info@bnoordhuis.nl>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "node.h"  // Picks up BUILDING_NODE_EXTENSION on Windows, see #30.

#include "nan.h"
#include "uv.h"
#include "v8-profiler.h"
#include "v8.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

namespace {

static const int kMaxPath = 4096;
static const int kForkFlag = 1;
static const int kSignalFlag = 2;
inline bool WriteSnapshot(v8::Isolate* isolate, const char* filename);
inline bool WriteSnapshotHelper(v8::Isolate* isolate, const char* filename);
inline void InvokeCallback(const char* filename);
inline void PlatformInit(v8::Isolate* isolate, int flags);
inline void RandomSnapshotFilename(char* buffer, size_t size);
static Nan::Callback on_complete_callback;

}  // namespace anonymous

#ifdef _WIN32
#include "heapdump-win32.h"
#else
#include "heapdump-posix.h"
#endif

namespace {

class FileOutputStream : public v8::OutputStream {
 public:
  FileOutputStream(FILE* stream) : stream_(stream) {}

  virtual int GetChunkSize() {
    return 65536;  // big chunks == faster
  }

  virtual void EndOfStream() {}

  virtual WriteResult WriteAsciiChunk(char* data, int size) {
    const size_t len = static_cast<size_t>(size);
    size_t off = 0;

    while (off < len && !feof(stream_) && !ferror(stream_))
      off += fwrite(data + off, 1, len - off, stream_);

    return off == len ? kContinue : kAbort;
  }

 private:
  FILE* stream_;
};

const v8::HeapSnapshot* TakeHeapSnapshot(v8::Isolate* isolate) {
  (void) &isolate;
#if NODE_VERSION_AT_LEAST(3, 0, 0)
  return isolate->GetHeapProfiler()->TakeHeapSnapshot();
#elif NODE_VERSION_AT_LEAST(0, 11, 0)
  return isolate->GetHeapProfiler()->TakeHeapSnapshot(Nan::EmptyString());
#else
  return v8::HeapProfiler::TakeSnapshot(Nan::EmptyString());
#endif
}

NAN_METHOD(WriteSnapshot) {
  v8::Isolate* const isolate = info.GetIsolate();

  v8::Local<v8::Value> maybe_function = info[0];
  if (1 < info.Length()) {
    maybe_function = info[1];
  }

  if (maybe_function->IsFunction()) {
    v8::Local<v8::Function> function =
        Nan::To<v8::Function>(maybe_function).ToLocalChecked();
    on_complete_callback.Reset(function);
  }

  char filename[kMaxPath];
  if (info[0]->IsString()) {
    Nan::Utf8String filename_string(info[0]);
    snprintf(filename, sizeof(filename), "%s", *filename_string);
  } else {
    RandomSnapshotFilename(filename, sizeof(filename));
  }

  // Throwing on error is too disruptive, just return a boolean indicating
  // success.
  const bool success = WriteSnapshot(isolate, filename);
  info.GetReturnValue().Set(success);
}

inline bool WriteSnapshotHelper(v8::Isolate* isolate, const char* filename) {
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) return false;
  const v8::HeapSnapshot* const snap = TakeHeapSnapshot(isolate);
  FileOutputStream stream(fp);
  snap->Serialize(&stream, v8::HeapSnapshot::kJSON);
  fclose(fp);
  // Work around a deficiency in the API.  The HeapSnapshot object is const
  // but we cannot call HeapProfiler::DeleteAllHeapSnapshots() because that
  // invalidates _all_ snapshots, including those created by other tools.
  const_cast<v8::HeapSnapshot*>(snap)->Delete();
  return true;
}

inline void InvokeCallback(const char* filename) {
  Nan::HandleScope handle_scope;
  if (on_complete_callback.IsEmpty()) return;
  v8::Local<v8::Value> argv[] = {Nan::Null(),
                                 Nan::New(filename).ToLocalChecked()};
  const int argc = sizeof(argv) / sizeof(*argv);
  Nan::Call(on_complete_callback, argc, argv);
}

inline void RandomSnapshotFilename(char* buffer, size_t size) {
  const uint64_t now = uv_hrtime();
  const unsigned long sec = static_cast<unsigned long>(now / 1000000);
  const unsigned long usec = static_cast<unsigned long>(now % 1000000);
  snprintf(buffer, size, "heapdump-%lu.%lu.heapsnapshot", sec, usec);
}

NAN_METHOD(Configure) {
  PlatformInit(info.GetIsolate(), Nan::To<int32_t>(info[0]).FromJust());
}

NAN_MODULE_INIT(Initialize) {
  Nan::Set(target,
           Nan::New("kForkFlag").ToLocalChecked(),
           Nan::New(kForkFlag));
  Nan::Set(target,
           Nan::New("kSignalFlag").ToLocalChecked(),
           Nan::New(kSignalFlag));
  Nan::Set(target,
           Nan::New("configure").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(Configure)->GetFunction());
  Nan::Set(target,
           Nan::New("writeSnapshot").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(WriteSnapshot)->GetFunction());
}

NODE_MODULE(addon, Initialize)

}  // namespace anonymous
