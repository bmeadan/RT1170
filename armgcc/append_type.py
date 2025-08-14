import sys
import os

if __name__ == "__main__":
    type_int = int(sys.argv[1], 16) & 0xff
    target = sys.argv[2]
    current_directory = os.getcwd()
    filepath = os.path.join(current_directory, target)
    with open(filepath, 'ab') as f:
        f.write(bytes([type_int]))
