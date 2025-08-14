import socket
import time
import pyaudio
import threading
import numpy as np

# Server details
SERVER_IP = "192.168.1.2"
SERVER_PORT = 1234
BUFFER_SIZE = 1024 * 2

# Audio settings
SAMPLE_RATE = 8000
CHANNELS = 2
SAMPLE_FORMAT = pyaudio.paInt16

# פקודה קבועה
COMMAND = bytes([0xAA, 0x01, 0, 8, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 215])

def send_loop(sock):
    while True:
        sock.sendto(COMMAND, (SERVER_IP, SERVER_PORT))
        time.sleep(0.02)  # 20ms בין פקודות, מתאים ל־8000Hz / 160 דגימות
        #time.sleep(180)
def receive_loop(sock, stream):
    while True:
        try:
            data, _ = sock.recvfrom(BUFFER_SIZE)
            if data[2] == 5:
                if len(data) >= 10:
                  #with open("audio_dump.raw", "ab") as f:
                   #f.write(data[7:-1])
                   stream.write(data[7:-1])
        except Exception as e:
            print("Error receiving:", e)

def udp_audio_client():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)

    audio = pyaudio.PyAudio()
    stream = audio.open(format=SAMPLE_FORMAT,
                        channels=CHANNELS,
                        rate=SAMPLE_RATE,
                        output=True)

    sender = threading.Thread(target=send_loop, args=(sock,), daemon=True)
    receiver = threading.Thread(target=receive_loop, args=(sock, stream), daemon=True)

    sender.start()
    receiver.start()

    try:
        while True:
            time.sleep(1)  # שמור את התהליך חי
    except KeyboardInterrupt:
        pass
    finally:
        stream.stop_stream()
        stream.close()
        audio.terminate()
        sock.close()

if __name__ == "__main__":
    udp_audio_client()
