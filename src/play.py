import socket
import cv2
import numpy as np

# UDP configuration
UDP_IP = "0.0.0.0"  # Listen on all available interfaces
UDP_PORT = 1234      # Port to listen on
BUFFER_SIZE = 65507  # Max UDP packet size

# JPEG start and end markers
JPEG_START = b'\xff\xd8'
JPEG_END = b'\xff\xd9'

# Initialize UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

frame_buffer = bytearray()
receiving = False

while True:
    try:
        data, addr = sock.recvfrom(BUFFER_SIZE)
        
        if data.startswith(JPEG_START):
            frame_buffer.clear()
            frame_buffer.extend(data)
            receiving = True
        elif receiving:
            frame_buffer.extend(data)
            if data.endswith(JPEG_END):
                frame = np.frombuffer(frame_buffer, dtype=np.uint8)
                image = cv2.imdecode(frame, cv2.IMREAD_COLOR)
                if image is not None:
                    cv2.imshow('Received Frame', image)
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        break
                receiving = False
    except KeyboardInterrupt:
        break

sock.close()
cv2.destroyAllWindows()
