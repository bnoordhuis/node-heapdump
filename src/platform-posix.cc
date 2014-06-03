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

#include "uv.h"
#include "v8.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

namespace heapdump
{

using v8::Isolate;

uv_signal_t signal_handle;
uv_signal_t sigchld_handle;
pid_t child_pid = -1;

void OnSIGUSR2(uv_signal_t* handle, int signo)
{
  assert(handle == &signal_handle);
  Isolate* isolate = reinterpret_cast<Isolate*>(handle->data);
  heapdump::WriteSnapshot(isolate, NULL);
}

void OnSIGCHLD(uv_signal_t* handle, int signo)
{
  assert(handle == &sigchld_handle);
  int status;
  pid_t pid = waitpid(child_pid, &status, WNOHANG);
  if (pid == 0) {
    return;
  }
  // ECHILD is not an error, it means that libuv's internal SIGCHLD handler
  // came before use and consumed the event.  Just ignore it and stop the
  // signal watcher, our child is gone and that's what matters.
  if (pid == -1 && errno != ECHILD) {
    perror("(node-heapdump) waitpid");
    return;
  }
  uv_signal_stop(&sigchld_handle);
}

bool WriteSnapshot(Isolate* isolate, const char* filename)
{
  if (uv_is_active(reinterpret_cast<uv_handle_t*>(&sigchld_handle))) {
    return true;  // Already busy writing a snapshot.
  }
  child_pid = fork();
  if (child_pid == -1) {
    return false;
  }
  if (child_pid != 0) {
    uv_signal_start(&sigchld_handle, OnSIGCHLD, SIGCHLD);
    return true;
  }
  setsid();
  WriteSnapshotHelper(isolate, filename);
  _exit(42);
  return true;  // Placate compiler.
}

void PlatformInit(Isolate* isolate)
{
  const char* options = getenv("NODE_HEAPDUMP_OPTIONS");
  if (options == NULL || strcmp(options, "nosignal") != 0) {
    uv_signal_init(uv_default_loop(), &signal_handle);
    uv_signal_start(&signal_handle, OnSIGUSR2, SIGUSR2);
    uv_unref(reinterpret_cast<uv_handle_t*>(&signal_handle));
    signal_handle.data = isolate;
  }
  uv_signal_init(uv_default_loop(), &sigchld_handle);
}

} // namespace heapdump
