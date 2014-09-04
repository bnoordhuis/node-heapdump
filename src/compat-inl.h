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

#if COMPAT_NODE_VERSION == 10
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

#if COMPAT_NODE_VERSION == 10
template <typename T>
inline v8::Local<T> ToLocal(v8::Handle<T> handle) {
  return v8::Local<T>::New(handle);
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

v8::Local<v8::Number> Number::New(v8::Isolate* isolate, double value) {
  I::Use(isolate);
  return v8::Number::New(COMPAT_ISOLATE_ value);
}

v8::Local<v8::Object> Object::New(v8::Isolate* isolate) {
  I::Use(isolate);
  return v8::Object::New(COMPAT_ISOLATE);
}

template <typename T>
Persistent<T>::~Persistent() {
  // Trying to dispose the handle when the VM has been destroyed
  // (e.g. at program exit) will segfault.
  if (v8::V8::IsDead()) return;
  Reset();
}

template <typename T>
bool Persistent<T>::IsEmpty() const {
  return handle_.IsEmpty();
}

ReturnType ReturnableHandleScope::Return(bool value) {
  return Return(Boolean::New(isolate(), value));
}

ReturnType ReturnableHandleScope::Return(intptr_t value) {
  return Return(Integer::New(isolate(), value));
}

ReturnType ReturnableHandleScope::Return(double value) {
  return Return(Number::New(isolate(), value));
}

ReturnType ReturnableHandleScope::Return(const char* value) {
  return Return(String::NewFromUtf8(isolate(), value));
}

v8::Isolate* ReturnableHandleScope::isolate() const {
  return args_.GetIsolate();
}

#if COMPAT_NODE_VERSION == 10

const v8::HeapSnapshot* HeapProfiler::TakeHeapSnapshot(
    v8::Isolate* isolate, v8::Local<v8::String> title) {
  if (title.IsEmpty()) title = v8::String::Empty(isolate);
  return v8::HeapProfiler::TakeSnapshot(title);
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

#elif COMPAT_NODE_VERSION == 12

const v8::HeapSnapshot* HeapProfiler::TakeHeapSnapshot(
    v8::Isolate* isolate, v8::Local<v8::String> title) {
  if (title.IsEmpty()) title = v8::String::Empty(isolate);
  return isolate->GetHeapProfiler()->TakeHeapSnapshot(title);
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
