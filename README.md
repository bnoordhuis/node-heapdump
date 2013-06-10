node-heapdump
===

Make a dump of the V8 heap for later inspection.

### Install

    npm install heapdump

Or, if you are running node.js v0.6 or v0.8:

    npm install heapdump@0.1.0

### Build

    node-gyp configure build

### Usage

Load the add-on in your application:

    var heapdump = require('heapdump');

The module exports a single, no-arg function called `writeSnapshot()` that
writes a `heapdump-xxxx.xxxx.heapsnapshot` file to the application's current
directory.

    heapdump.writeSnapshot();

On UNIX, it forks off a new process that writes out the snapshot in an
asynchronous fashion.  (That is, the function does not block.)

On Windows, however, it returns only after the snapshot is fully written.
If the heap is large, that may take a while.

On UNIX platforms, you can force a snapshot by sending the node.js process
a SIGUSR2 signal:

    $ kill -USR2 <pid>

### Inspecting the snapshot

Open [Google Chrome](https://www.google.com/intl/en/chrome/browser/) and
press F12 to open the developer toolbar.

Go to the `Profiles` tab, right-click in the tab pane and select
`Load profile...`.

Select the dump file and click `Open`.  You can now inspect the heap snapshot
at your leisure.
