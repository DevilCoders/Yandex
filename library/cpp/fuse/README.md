FUSE protocol implementation library. Intends to replace libfuse.

Important classes:
* `TLowMount` - FUSE protocol implementation. Reads messages from FUSE channel, parses them and translates to `IDispatcher` calls.
* `IDispatcher` - interface that must be implemented by filesystem to use this library. Resembles `fuse_lowlevel_ops` from libfuse.
* `TTypedRequestContext<T>` - context of current FUSE request. Provides methods to send reply and access to request metadata (start time, deadline, uid & gid).
* `IChannel` - abstract FUSE communication channel that can send and receive messages.
    * `TFdChannel` - channel implementation that uses file descriptor (e.g. `/dev/fuse`) for communication.

