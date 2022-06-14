from socket import *
import struct
import os
PORT = 5555
ADDRESS = "10.0.2.2"
id = 0
def send_file(sock, filepath):
    file_size = os.path.getsize(filepath)
    sock.sendall(struct.pack('!L',file_size))
    filename, ext = os.path.splitext(filepath)
    ext = ext[1:]
    print(f"Extension: {ext}\n")
    ext_msg = bytearray(ext, 'ascii')
    ext_msg.extend(bytes(6-len(ext_msg)))
    sock.sendall(ext_msg)
    f = open(filepath, 'rb')
    data = f.read(1024)
    while data:
        sock.sendall(data)
        data = f.read(1024)
    print("Done sending image!\n")
def receive_file(sock, filename):
    file_size = sock.recv(4)
    file_size, = struct.unpack('!L',file_size)
    ext = sock.recv(6)
    ext = ext.split(b'\x00')[0]
    ext = str(ext, 'ascii', 'ignore')
    file_path = filename + "." + ext
    f = open(file_path, 'wb+')
    bytes_left = file_size
    data = sock.recv(1024 if file_size>1024 else file_size)
    while bytes_left:
        f.write(data)
        bytes_left -= len(data)
        data = sock.recv(1024 if bytes_left>1024 else bytes_left)
    print("Received file!\n")
def get_image_path(title):
    print(f"{title}\n")
    img_path = input("Image path: ")
    return img_path
def get_operation_argument():
    arg = input("Argument: ")
    return arg
def send_processing_request(sock, filepath, op, arg):
    sock.sendall(id)
    sock.sendall(struct.pack('!H',op))
    if arg != None:
        arg_msg = bytearray(arg, 'ascii')
        arg_msg.extend(bytes(64-len(arg_msg)))
        sock.sendall(arg_msg)
    send_file(sock, filepath)
with socket(AF_INET, SOCK_STREAM) as s:
    s.connect((ADDRESS, PORT))
    print("Connected to server")
    id = s.recv(16)
    print("Client UUID:",id)
    while(1):
        print("Operations\nr.resize\nc.convert\nf.flip\nt.get tags\ne.exit")
        op = input("Select an operation:")
        if op == 'r':
            send_processing_request(s, get_image_path("Resize Image"), 100, get_operation_argument())
            receive_file(s, 'server_processed')
        elif op == 'c':
            send_processing_request(s, get_image_path("Convert Image"), 101, get_operation_argument())
            receive_file(s, 'server_processed')
        elif op == 'f':
            send_processing_request(s, get_image_path("Flip Image"), 102, None)
            receive_file(s, 'server_processed')
        elif op == 't':
            send_processing_request(s, get_image_path("Tag Image"), 199, None)
            tags = s.recv(128)
            tags = tags.split(b'\x00')[0]
            print("Tags are: ",str(tags, 'ascii', 'ignore'))
        elif op == 'e':
            break
        else:
            print(f"Option {op} is not a valid option")
        print("\n")


