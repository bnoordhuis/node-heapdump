// Copyright (c) 2014, StrongLoop Inc.
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

#ifndef COMPAT_INL_H_  // NOLINT(build/header_guard)
#define COMPAT_INL_H_  // NOLINT(build/header_guard)

#include "compat.h"

namespace compat {

#if !NODE_VERSION_AT_LEAST(0, 11, 0)
#define COMPAT_ISOLATE
#define COMPAT_ISOLATE_
#else
#define COMPAT_ISOLATE isolate
#define COMPAT_ISOLATE_ isolate,
#endif

namespace I {

template <typename T>
inline void Use(const T&) {}

template <typename T>
inline v8::Local<T> ToLocal(v8::Local<T> handle) {
  return handle;
}

#if !NODE_VERSION_AT_LEAST(0, 11, 0)
template <typename T>
inline v8::Local<T> ToLocal(v8::Handle<T> handle) {
  return v8::Local<T>(*handle);
}
#endif

}  // namespace I

v8::Local<v8::Boolean> True(v8::Isolate* isolate) {
  I::Use(isolate);
  return I::ToLocal<v8::Boolean>(v8::True(COMPAT_ISOLATE));
}

v8::Local<v8::Boolean> False(v8::Isolate* isolate) {
  I::Use(isolate);
  return I::ToLocal<v8::Boolean>(v8::False(COMPAT_ISOLATE));
}

v8::Local<v8::Primitive> Null(v8::Isolate* isolate) {
  I::Use(isolate);
  return I::ToLocal<v8::Primitive>(v8::Null(COMPAT_ISOLATE));
}

v8::Local<v8::Primitive> Undefined(v8::Isolate* isolate) {
  I::Use(isolate);
  return I::ToLocal<v8::Primitive>(v8::Undefined(COMPAT_ISOLATE));
}

v8::Local<v8::Array> Array::New(v8::Isolate* isolate, int length) {
  I::Use(isolate);
  return v8::Array::New(COMPAT_ISOLATE_ length);
}

v8::Local<v8::Boolean> Boolean::New(v8::Isolate* isolate, bool value) {
  I::Use(isolate);
  return I::ToLocal<v8::Boolean>(v8::Boolean::New(COMPAT_ISOLATE_ value));
}

v8::Local<v8::FunctionTemplate> FunctionTemplate::New(
    v8::Isolate* isolate, FunctionCallback callback) {
  I::Use(isolate);
  return v8::FunctionTemplate::New(COMPAT_ISOLATE_ callback);
}

v8::Local<v8::Integer> Integer::New(v8::Isolate* isolate, int32_t value) {
  I::Use(isolate);
  return v8::Integer::New(COMPAT_ISOLATE_ value);
}

v8::Local<v8::Integer> Integer::NewFromUnsigned(v8::Isolate* isolate,
                                                uint32_t value) {
  I::Use(isolate);
  return v8::Integer::NewFromUnsigned(COMPAT_ISOLATE_ value);
}

void Isolate::SetAddHistogramSampleFunction(
    v8::Isolate* isolate, v8::AddHistogramSampleCallback callback) {
#if !NODE_VERSION_AT_LEAST(0, 11, 15)
  I::Use(isolate);
  v8::V8::SetAddHistogramSampleFunction(callback);
#else
  isolate->SetAddHistogramSampleFunction(callback);
#endif
}

void Isolate::SetCreateHistogramFunction(v8::Isolate* isolate,
                                         v8::CreateHistogramCallback callback) {
#if !NODE_VERSION_AT_LEAST(0, 11, 15)
  I::Use(isolate);
  v8::V8::SetCreateHistogramFunction(callback);
#else
  isolate->SetCreateHistogramFunction(callback);
#endif
}

void Isolate::SetJitCodeEventHandler(v8::Isolate* isolate,
                                     v8::JitCodeEventOptions options,
                                     v8::JitCodeEventHandler handler) {
#if !NODE_VERSION_AT_LEAST(1, 0, 0)
  I::Use(isolate);
  v8::V8::SetJitCodeEventHandler(options, handler);
#else
  isolate->SetJitCodeEventHandler(options, handler);
#endif
}

v8::Local<v8::Number> Number::New(v8::Isolate* isolate, double value) {
  I::Use(isolate);
  return v8::Number::New(COMPAT_ISOLATE_ value);
}

v8::Local<v8::Object> Object::New(v8::Isolate* isolate) {
  I::Use(isolate);
  return v8::Object::New(COMPAT_ISOLATE);
}

template <typename T>
bool Persistent<T>::IsEmpty() const {
  return handle_.IsEmpty();
}

ReturnType ReturnableHandleScope::Return(bool value) {
  return Return(Boolean::New(isolate(), value));
}

ReturnType ReturnableHandleScope::Return(int32_t value) {
  return Return(Integer::New(isolate(), value));
}

ReturnType ReturnableHandleScope::Return(uint32_t value) {
  return Return(Integer::New(isolate(), value));
}

ReturnType ReturnableHandleScope::Return(double value) {
  return Return(Number::New(isolate(), value));
}

ReturnType ReturnableHandleScope::Return(const char* value) {
  return Return(String::NewFromUtf8(isolate(), value));
}

ReturnType ReturnableHandleScope::Throw(v8::Local<v8::Value> exception) {
  Isolate::ThrowException(isolate(), exception);
  return Return();
}

ReturnType ReturnableHandleScope::ThrowError(const char* exception) {
  return ThrowError(String::NewFromUtf8(isolate(), exception));
}

ReturnType ReturnableHandleScope::ThrowError(v8::Local<v8::String> exception) {
  return Throw(v8::Exception::Error(exception));
}

ReturnType ReturnableHandleScope::ThrowRangeError(const char* exception) {
  return ThrowRangeError(String::NewFromUtf8(isolate(), exception));
}

ReturnType ReturnableHandleScope::ThrowRangeError(
    v8::Local<v8::String> exception) {
  return Throw(v8::Exception::RangeError(exception));
}

ReturnType ReturnableHandleScope::ThrowReferenceError(const char* exception) {
  return ThrowReferenceError(String::NewFromUtf8(isolate(), exception));
}

ReturnType ReturnableHandleScope::ThrowReferenceError(
    v8::Local<v8::String> exception) {
  return Throw(v8::Exception::ReferenceError(exception));
}

ReturnType ReturnableHandleScope::ThrowSyntaxError(const char* exception) {
  return ThrowSyntaxError(String::NewFromUtf8(isolate(), exception));
}

ReturnType ReturnableHandleScope::ThrowSyntaxError(
    v8::Local<v8::String> exception) {
  return Throw(v8::Exception::SyntaxError(exception));
}

ReturnType ReturnableHandleScope::ThrowTypeError(const char* exception) {
  return ThrowTypeError(String::NewFromUtf8(isolate(), exception));
}

ReturnType ReturnableHandleScope::ThrowTypeError(
    v8::Local<v8::String> exception) {
  return Throw(v8::Exception::TypeError(exception));
}

v8::Isolate* ReturnableHandleScope::isolate() const {
  return args_.GetIsolate();
}

#if !NODE_VERSION_AT_LEAST(0, 11, 0)

void CpuProfiler::StartCpuProfiling(v8::Isolate* isolate,
                                    v8::Local<v8::String> title) {
  if (title.IsEmpty()) title = v8::String::Empty(isolate);
  return v8::CpuProfiler::StartProfiling(title);
}

const v8::CpuProfile* CpuProfiler::StopCpuProfiling(
    v8::Isolate* isolate, v8::Local<v8::String> title) {
  if (title.IsEmpty()) title = v8::String::Empty(isolate);
  return v8::CpuProfiler::StopProfiling(title);
}

void Isolate::GetHeapStatistics(v8::Isolate* isolate,
                                v8::HeapStatistics* stats) {
  I::Use(isolate);
  return v8::V8::GetHeapStatistics(stats);
}

v8::Local<v8::Value> Isolate::ThrowException(v8::Isolate* isolate,
                                             v8::Local<v8::Value> exception) {
  I::Use(isolate);
  return I::ToLocal<v8::Value>(v8::ThrowException(exception));
}

const v8::HeapSnapshot* HeapProfiler::TakeHeapSnapshot(v8::Isolate* isolate) {
  return v8::HeapProfiler::TakeSnapshot(v8::String::Empty(isolate));
}

void HeapProfiler::DeleteAllHeapSnapshots(v8::Isolate* isolate) {
  I::Use(isolate);
  v8::HeapProfiler::DeleteAllSnapshots();
}

v8::Local<v8::String> String::NewFromUtf8(v8::Isolate* isolate,
                                          const char* data, NewStringType type,
                                          int length) {
  I::Use(isolate);
  if (type == kInternalizedString) {
    return v8::String::NewSymbol(data, length);
  }
  if (type == kUndetectableString) {
    return v8::String::NewUndetectable(data, length);
  }
  return v8::String::New(data, length);
}

HandleScope::HandleScope(v8::Isolate*) {}

template <typename T>
v8::Local<T> Persistent<T>::ToLocal(v8::Isolate*) const {
  return *handle_;
}

template <typename T>
void Persistent<T>::Reset() {
  handle_.Dispose();
  handle_.Clear();
}

template <typename T>
void Persistent<T>::Reset(v8::Isolate*, v8::Local<T> value) {
  Reset();
  handle_ = v8::Persistent<T>::New(value);
}

ReturnableHandleScope::ReturnableHandleScope(const ArgumentType& args)
    : args_(args) {}

ReturnType ReturnableHandleScope::Return() { return v8::Undefined(); }

ReturnType ReturnableHandleScope::Return(v8::Local<v8::Value> value) {
  return handle_scope_.Close(value);
}

#else

void CpuProfiler::StartCpuProfiling(v8::Isolate* isolate,
                                    v8::Local<v8::String> title) {
  const bool record_samples = true;
  if (title.IsEmpty()) title = v8::String::Empty(isolate);
#if !NODE_VERSION_AT_LEAST(3, 0, 0)
  return isolate->GetCpuProfiler()->StartCpuProfiling(title, record_samples);
#else
  return isolate->GetCpuProfiler()->StartProfiling(title, record_samples);
#endif
}

const v8::CpuProfile* CpuProfiler::StopCpuProfiling(
    v8::Isolate* isolate, v8::Local<v8::String> title) {
  if (title.IsEmpty()) title = v8::String::Empty(isolate);
#if !NODE_VERSION_AT_LEAST(3, 0, 0)
  return isolate->GetCpuProfiler()->StopCpuProfiling(title);
#else
  return isolate->GetCpuProfiler()->StopProfiling(title);
#endif
}

void Isolate::GetHeapStatistics(v8::Isolate* isolate,
                                v8::HeapStatistics* stats) {
  return isolate->GetHeapStatistics(stats);
}

v8::Local<v8::Value> Isolate::ThrowException(v8::Isolate* isolate,
                                             v8::Local<v8::Value> exception) {
  return isolate->ThrowException(exception);
}

const v8::HeapSnapshot* HeapProfiler::TakeHeapSnapshot(v8::Isolate* isolate) {
#if !NODE_VERSION_AT_LEAST(3, 0, 0)
  v8::Local<v8::String> title = v8::String::Empty(isolate);
  return isolate->GetHeapProfiler()->TakeHeapSnapshot(title);
#else
  return isolate->GetHeapProfiler()->TakeHeapSnapshot();
#endif
}

void HeapProfiler::DeleteAllHeapSnapshots(v8::Isolate* isolate) {
  return isolate->GetHeapProfiler()->DeleteAllHeapSnapshots();
}

v8::Local<v8::String> String::NewFromUtf8(v8::Isolate* isolate,
                                          const char* data, NewStringType type,
                                          int length) {
  return v8::String::NewFromUtf8(
      isolate, data, static_cast<v8::String::NewStringType>(type), length);
}

HandleScope::HandleScope(v8::Isolate* isolate) : handle_scope_(isolate) {}

template <typename T>
v8::Local<T> Persistent<T>::ToLocal(v8::Isolate* isolate) const {
  return v8::Local<T>::New(isolate, handle_);
}

template <typename T>
void Persistent<T>::Reset() {
  handle_.Reset();
}

template <typename T>
void Persistent<T>::Reset(v8::Isolate* isolate, v8::Local<T> value) {
  handle_.Reset(isolate, value);
}

ReturnableHandleScope::ReturnableHandleScope(const ArgumentType& args)
    : args_(args), handle_scope_(args.GetIsolate()) {}

ReturnType ReturnableHandleScope::Return() {}

ReturnType ReturnableHandleScope::Return(v8::Local<v8::Value> value) {
  // TODO(bnoordhuis) Creating handles for primitive values (int, double)
  // is less efficient than passing them to ReturnValue<T>::Set() directly.
  return args_.GetReturnValue().Set(value);
}

#endif

#undef COMPAT_ISOLATE_
#undef COMPAT_ISOLATE

}  // namespace compat

#endif  // COMPAT_INL_H_  // NOLINT(build/header_guard)
