import serial
import time


def decypher_message(data: str) -> str:
    print(f"Raw data: {data}")
    result = []
    i = 0
    # First 3 bytes are ID (11 bit)
    ID = data[i : i + 3]
    i += 3
    # Next byte is DLC (data length code)
    DLC = int(data[i : i + 1])
    i += 1
    while i < len(data):
        # Take 2 chars and convert to byte
        byte_str = data[i : i + 2]
        byte_val = int(byte_str, 16)
        result.append(byte_val)
        i += 2
    print(f"{ID}: {result}")


# Configure serial connection
ser = serial.Serial(port="COM12", baudrate=115200, timeout=1)
ser.write(b"C\r")
response = ser.read(1)
if b"\r" in response:
    print("OK")
else:
    print("ERROR")
# Send "S8\r" to the device
ser.write(b"S8\r")
response = ser.read(1)
if b"\r" in response:
    print("OK")
else:
    print("ERROR")
ser.write(b"O\r")
response = ser.read(1)
if b"\r" in response:
    print("OK")
    res = []
    while True:
        data = ser.read(1)
        if not data:
            continue
        if data == b"\r":
            decypher_message("".join(res))
            res = []
        else:
            if data == b"t":
                continue
            res.append(data.decode(errors="replace"))
else:
    print("ERROR")

ser.write(b"t0946000000000000\r")
response = ser.read(1)
if b"\r" in response:
    print("OK")
else:
    print("ERROR")
# ser.write(b"F\r")
# time.sleep(0.1)
# response = ser.read_until(b"\r")
# print(list(response))


# Close the connection
ser.close()
