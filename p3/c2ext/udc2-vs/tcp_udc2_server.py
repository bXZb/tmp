import socket
import struct
import argparse
import threading
from typing import Optional

def send_frame(sock: socket.socket, payload: bytes) -> None:
    """Send a frame with 4-byte length prefix followed by payload."""
    length = len(payload)
    frame = struct.pack('<I', length) + payload
    sock.sendall(frame)

def receive_frame(sock: socket.socket) -> bytes:
    """Receive a frame: 4-byte length prefix followed by payload."""
    length_bytes = sock.recv(4)
    if len(length_bytes) != 4:
        raise ConnectionError("Failed to read frame length")
    length = struct.unpack('<I', length_bytes)[0]
    payload = b''
    while len(payload) < length:
        chunk = sock.recv(length - len(payload))
        if not chunk:
            raise ConnectionError("Connection closed while reading payload")
        payload += chunk
    
    return length_bytes + payload 

def parse_arguments() -> argparse.Namespace:
    """Parse command line arguments for server configuration."""
    parser = argparse.ArgumentParser(
        description='UDC2 TCP Server - Relay frames between Beacon and TeamServer',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s --bind-addr 0.0.0.0 --bind-port 8443 --ts-addr 192.168.1.100 --ts-port 2222
        '''
    )
    
    # TeamServer connection settings
    parser.add_argument(
        '--ts-addr', '--teamserver-address',
        default='127.0.0.1',
        help='TeamServer IP address or hostname (default: %(default)s)'
    )
    
    parser.add_argument(
        '--ts-port', '--teamserver-port',
        type=int,
        default=2222,
        help='TeamServer port number (default: %(default)d)'
    )
    
    # Bind settings for incoming connections
    parser.add_argument(
        '--bind-addr', '--bind-address',
        default='127.0.0.1',
        help='Local IP address to bind for incoming connections (default: %(default)s)'
    )
    
    parser.add_argument(
        '--bind-port', '--bind-port',
        type=int,
        default=8443,
        help='Local port to bind for incoming connections (default: %(default)d)'
    )
    
    # Additional options
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose logging'
    )
    
    return parser.parse_args()

def handle_client_connection(client_socket: socket.socket, client_address: tuple, args: argparse.Namespace) -> None:
    """Handle a single client connection by proxying data to/from the TeamServer."""
    if args.verbose:
        print(f"[+] New client connection from {client_address}")
    
    try:
        # Connect to TeamServer
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ts_socket:
            try:
                ts_socket.connect((args.ts_addr, args.ts_port))
                if args.verbose:
                    print(f"[+] Connected to TeamServer at {args.ts_addr}:{args.ts_port}")
            except (ConnectionError, socket.error) as e:
                print(f"[-] Failed to connect to TeamServer {args.ts_addr}:{args.ts_port}: {e}")
                return
            
            # Send initial "go" frame
            send_frame(ts_socket, b"go")
            if args.verbose:
                print("[*] Sent initial 'go' frame")
            
            # Proxy loop: relay frames between client and TeamServer
            while True:
                try:
                    # Receive frame from client
                    client_frame = receive_frame(client_socket)
                    if args.verbose:
                        print(f"[+] Received {len(client_frame)} bytes from client")
                    
                    # Forward frame to TeamServer
                    ts_socket.sendall(client_frame)
                    if args.verbose:
                        print(f"[+] Sent {len(client_frame)} bytes to TeamServer")
                    
                    # Receive response from TeamServer
                    ts_response = receive_frame(ts_socket)
                    if args.verbose:
                        print(f"[+] Received {len(ts_response)} bytes from TeamServer")
                    
                    # Forward response back to client
                    client_socket.sendall(ts_response)
                    if args.verbose:
                        print(f"[+] Sent {len(ts_response)} bytes back to client")
                        
                except ConnectionError:
                    if args.verbose:
                        print(f"[*] Connection closed by client {client_address}")
                    break
                except Exception as e:
                    print(f"[-] Error in proxy loop for {client_address}: {e}")
                    break
                    
    except Exception as e:
        print(f"[-] Error handling client {client_address}: {e}")
    finally:
        try:
            client_socket.close()
        except:
            pass
        if args.verbose:
            print(f"[*] Closed connection to {client_address}")


def main() -> Optional[int]:
    # Parse command line arguments
    args = parse_arguments()
    
    if args.verbose:
        print(f"UDC2 TCP Server Configuration:")
        print(f"  Bind Address: {args.bind_addr}:{args.bind_port}")
        print(f"  TeamServer: {args.ts_addr}:{args.ts_port}")
        print(f"  Starting TCP proxy server...")
    
    # Create and configure server socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        # Bind to the specified address and port
        server_socket.bind((args.bind_addr, args.bind_port))
        server_socket.listen(5)  # Allow up to 5 queued connections
        
        # Set a timeout so accept() doesn't block indefinitely
        server_socket.settimeout(1.0)  # 1 second timeout
        
        print(f"[*] UDC2 TCP Server listening on {args.bind_addr}:{args.bind_port}")
        print(f"[*] Forwarding to TeamServer at {args.ts_addr}:{args.ts_port}")
        print(f"[*] Press Ctrl+C to stop the server")
        
        while True:
            try:
                # Accept incoming client connection
                client_socket, client_address = server_socket.accept()
                print(f"[*] Accepted connection from {client_address}")

                # Handle each client in a separate thread
                client_thread = threading.Thread(
                    target=handle_client_connection,
                    args=(client_socket, client_address, args),
                    daemon=True
                )
                client_thread.start()
                
            except socket.timeout:
                # Timeout is expected, just continue the loop to check for KeyboardInterrupt
                continue
            except Exception as e:
                print(f"[-] Error accepting connection: {e}")
                continue
                
    except KeyboardInterrupt:
        print("\n[*] Shutting down server...")
    except Exception as e:
        print(f"[-] Failed to start server: {e}")
        return 1
    finally:
        server_socket.close()
        if args.verbose:
            print("[*] Server socket closed")
    
    return 0


if __name__ == "__main__":
    try:
        exit_code = main()
        exit(exit_code or 0)
    except (ConnectionError, socket.error) as e:
        print(f"Error: {e}")
        exit(1)
