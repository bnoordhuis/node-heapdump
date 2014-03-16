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

#ifndef NODE_HEAPDUMP_H_
#define NODE_HEAPDUMP_H_

#include "node.h"  // Picks up BUILDING_NODE_EXTENSION on Windows, see #30.
#include "v8.h"

namespace heapdump
{

// Implemented in platform-posix.cc and platform-win32.cc.
void PlatformInit(v8::Isolate* isolate);
bool WriteSnapshot(v8::Isolate* isolate, const char* filename);

// Shared helper function, called by the platform WriteSnapshot().
bool WriteSnapshotHelper(v8::Isolate* isolate, const char* filename);

}

#endif // NODE_HEAPDUMP_H_
