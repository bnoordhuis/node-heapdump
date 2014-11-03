// Copyright (c) 2014, Ben Noordhuis <info@bnoordhuis.nl>
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

try {
  var addon = require('../build/Release/addon');
} catch (e) {
  var addon = require('../build/Debug/addon');
}

var kNoForkFlag = addon.kNoForkFlag;
var kNoSignalFlag = addon.kNoSignalFlag;

var flags = kNoForkFlag;
var options = (process.env.NODE_HEAPDUMP_OPTIONS || '').split(/\s*,\s*/);
for (var i = 0, n = options.length; i < n; i += 1) {
  var option = options[i];
  if (option === '') continue;
  else if (option === 'fork') flags &= ~kNoForkFlag;
  else if (option === 'signal') flags &= ~kNoSignalFlag;
  else if (option === 'nofork') flags |= kNoForkFlag;
  else if (option === 'nosignal') flags |= kNoSignalFlag;
  else console.error('node-heapdump: unrecognized option:', option);
}
addon.configure(flags);

exports.writeSnapshot = function() {
  return addon.writeSnapshot.apply(null, arguments);
};
