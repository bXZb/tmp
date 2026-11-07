# User-Defined C2 Beacon Object File Visual Studio Template

This repository contains the User-Defined C2 Beacon Object File Visual Studio (UDC2-VS) Solution. This project is designed to simplify the design, development and debugging of UDC2 BOFs built using the Beacon Object File Visual Studio template ([BOF-VS](https://github.com/Cobalt-Strike/bof-vs)).

**Note:** This repository assumes familiarity with BOF-VS. The BOF-VS project README contains 
information about the Dynamic Function Resolution (DFR) macros and helper functions used 
throughout this project.

### Prerequisites:

* An x64 Windows 10/11 development machine (without a security solution)
* Visual Studio Community/Pro/Enterprise 2022 (Desktop Development with C++ installed)
* Python 3 for the BOF linter (optional) and example UDC2 servers
* Network visibility of the team server's UDC2 listener

## Creating A User-Defined C2 Channel

A UDC2 channel requires two separate components:

1. The UDC2 client which must be implemented as a Beacon Object File (BOF)
2. The UDC2 server which can be created in any programming language and run independently of the team server, provided it has network visibility of the UDC2 listener.

A high level architecture of UDC2 is shown here:

```
┌─────────────────┐        ┌─────────────────┐        ┌─────────────────┐
│                 │        │                 │        │                 │
│     Beacon      │ <----> │   UDC2 Server   │ <----> │   Team Server   │
│   [UDC2 BOF]    │        │                 │        │                 │
│                 │        │                 │        │                 │
└─────────────────┘        └─────────────────┘        └─────────────────┘
```

The Cobalt Strike client will stomp the UDC2 BOF into an exported payload. At runtime, Beacon will call the `go()` entry point of the UDC2 BOF and pass it a pointer to a UDC2_INFO structure. This structure allows the UDC2 BOF to tell Beacon where it can find the core UDC2 functions - `udc2Proxy()` and `udc2Close()`. Beacon will then call `udc2Proxy()` each time it needs to send and receive data, and finally `udc2Close()` when the Beacon exits.

The udc2-bof-vs solution's default example (udc2_bof.cpp) is a simple TCP channel implementation. It is only intended as a quick start guide to help UDC2 developers get up and running quickly and to understand the concepts behind sending and receiving frame data. 

There are four important functions in this file:

- go() - The BOF entry point
- init() - An init function which can be used to set up/initialize a UDC2 channel
- udc2Proxy() - The proxy function called by Beacon each time it needs to send and receive data
- udc2Close() - A function for cleanup tasks like freeing any memory, closing connections, etc,  which is called by Beacon when it is about to exit

### Understanding Beacon Frames

Beacon sends and receives data in encrypted frames. A frame is simply a buffer of data that contains a 4-byte little endian packed length value followed by the raw encrypted buffer. When Beacon calls your `udc2Proxy` function, it will pass a complete frame to you in the `sendBuf` parameter. This outbound frame simply needs to be sent out over your protocol as is. When your UDC2 server gets the response from the Team Server, it will also be in frame format and needs to be written back via the `recvBuf` parameter of your `udc2Proxy` function in that format as well. Do not strip off the packed length value when writing the data back to the read buffer. Problems with frame data formatting are going to be the most likely single biggest source of failure when designing your UDC2 implementation, so always double check that first. 

## UDC2 Server Quick Start Guide

The UDC2 server acts as a relay between your UDC2 BOF and the Cobalt Strike Team Server. It is responsible for unwrapping Beacon frames from your chosen protocol and sending those directly to the Team Server. If you are creating an HTTP channel for example, your UDC2 server would function as your HTTP endpoint and handle HTTP requests from your UDC2 BOF.

To build a UDC2 server:

1. Decide on which language best suits your needs
2. Implement your protocol handler
3. For EACH Beacon that connects to your UDC2 server, you MUST establish a new TCP connection to your UDC2 listener on the Team Server to relay the frame data
4. Each new TCP session that is established with the UDC2 listener requires that you first send a "go" frame to start the session. See the tcp_udc2_server.py script for an example. 
5. Once a new session is established, send the frame data you received from your Beacon to the UDC2 listener, and transfer the frame data you receive in the response back to your UDC2 BOF

## Debug Build Quick Start Guide

The `Debug` target builds the UDC2 BOF as an executable, which provides the convenience of debugging it directly within Visual Studio with its built-in debugger. This makes it possible to work at the source code level without running the BOF through a Beacon.

**Note**: The `Debug` build's `Main()` function is not designed to replicate the UDC2 server component. Instead, it retrieves a Beacon payload and runs it within the `Debug` executable to act as a shim between the Beacon payload and the UDC2 client. This makes it possible to properly simulate a live environment. Do not re-use functions that are part of this debugging setup as they are designed specifically for that scenario. Refer to the example code instead. 

To start Debugging:

1. Configure a "Debug only" UDC2 listener in the Cobalt Strike client
2. Start the TCP UDC2 server python script which is in the same folder as the Visual Studio solution - `python3 tcp_udc2_server.py --bind-addr <addr to bind to> --bind-port <port to listen on> --ts-addr <team server addr> --ts-port <udc2 debug listener port>`
3. Set the `gUDC2Server` and `gUDC2ServerPort` variables in udc2_bof.cpp to your UDC2 server bind address and port
4. Set the `UDC2_DEBUG_HOST` and `UDC2_DEBUG_PORT` variables in the debug main function (used to retrieve a debug payload from the UDC2 listener)
5. Click "Local Windows Debugger"

## Release Build Quick Start Guide

The `Release` target builds the UDC2 BOF into an object file for use in Cobalt Strike. You can build both x86 and x64 versions of the BOF. 

To start using your UDC2 BOF:

1. Set the `gUDC2Server` and `gUDC2ServerPort` variables in udc2_bof.cpp to the address you will bind your UDC2 server to and the port you will use
2. Build the UDC2 BOF for `Release`
3. Create a new UDC2 listener in the Cobalt Strike client (make sure "Debug Only" is not checked) and specify the full path to your UDC2 object file that you just built
4. Start the TCP UDC2 server python script which is in the same folder as the Visual Studio solution - `python3 tcp_udc2_server.py --bind-addr <addr to bind to> --bind-port <port to listen on> --ts-addr <team server addr> --ts-port <udc2 listener port>` making sure that the bind address and bind port match what you set in step 1
5. Export a payload from your Cobalt Strike client that uses your newly created UDC2 listener from step 3
6. Run your payload

## FAQ

- **What kind of C2 channels can I create with UDC2?**
    It is possible to create C2 channels over files, ICMP, HTTP, Slack, Discord, Azure, AWS, etc. If you can build a transport for it in your UDC2 BOF, it can work as a C2 channel. 
- **Can I use a round-robin approach to UDC2 and have Beacon try multiple UDC2 BOFs until it establishes a connection?**
    Currently this is not supported, however it may be implemented in a future release
- **Do UDC2 payloads link in specific Windows libraries for comms?**
    By default, the only comms library that the UDC2 payload will be linked against is WS2_32. If you need to utilize other Windows libraries such as WinINet for HTTP comms, you can use dynamic function resolution in your BOF 
- **Is there a size limit to UDC2 BOFs?**
    Yes, BOFs are currently limited to roughly 32 KB. This should be sufficient for most C2 implementations, however depending on usage and feedback, it may be increased. 
- **Can I adjust the size of the buffer pointed to by recvBuf to accommodate larger data transfers**
    Yes, that value is controlled via the `tasks_max_size` malleable c2 profile setting and by default starts at 1 MB
- **Can I link UDC2 Beacons in a P2P way?**
    Currently this is not supported, however it is possible that this will be added in a future release. 
- **Is my UDC2 BOF exposed in memory when Beacon is sleeping?**
    No, Beacon will mask the UDC2 BOF before it calls into its own sleepmask.