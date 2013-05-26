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
#include "node_version.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

namespace
{

uv_async_t async_handle;

void SignalHandler(int signum)
{
  if (uv_async_send(&async_handle)) abort();
}

void AsyncEvent(uv_async_t* handle, int status)
{
  assert(handle == &async_handle);
  heapdump::WriteSnapshot();
}

} // namespace anonymous


namespace heapdump
{

void PlatformInit()
{
  if (uv_async_init(uv_default_loop(), &async_handle, AsyncEvent)) abort();

#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION == 6
  uv_unref(uv_default_loop());
#else
  uv_unref(reinterpret_cast<uv_handle_t*>(&async_handle));
#endif

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SignalHandler;
  if (sigaction(SIGUSR2, &sa, NULL)) abort();
}

void WriteSnapshot()
{
  if (fork() != 0) return;
  setsid();
  WriteSnapshotHelper();
  _exit(42);
}

long GetPID()
{
  return (long) getppid();
}

} // namespace heapdump
