import serial
import numpy as np
from PIL import Image
import cv2
import pickle
import time
import signal
import os
import win32gui
import win32con
import win32api

# If the output prints are to be displayed in terminal
PRINT = True

# If the image is to be displayed when received
DISPLAY = True

# If the terminal and the camera display are TOPMOST over other windows
TOPMOST = False

# If the user's face is to be recognized or only detected
RECOGNITION = True

###################################################

WIDTH = 160
HEIGHT = 120

START_COMMAND = b"\x00"
DETECTED_COMMAND = b"2"
RECOGNIZED_COMMAND = DETECTED_COMMAND
NOT_DETECTED_COMMAND = b"3"
NOT_RECOGNIZED_COMMAND = NOT_DETECTED_COMMAND
###################################################


def printTerminal(string):
    if PRINT:
        print(string)


def terminationHandler(signum, frame):
    ser.write(b"2")  # Reset
    printTerminal("-- EXIT --")
    exit(1)


signal.signal(signal.SIGINT, terminationHandler)

hwnd = win32gui.GetForegroundWindow()
if PRINT:
    width = win32gui.GetWindowRect(hwnd)[2] - win32gui.GetWindowRect(hwnd)[0]
    height = win32gui.GetWindowRect(hwnd)[3] - win32gui.GetWindowRect(hwnd)[1]
    screen_width = win32api.GetSystemMetrics(0)
    screen_height = win32api.GetSystemMetrics(1)
    win32gui.SetWindowPos(
        hwnd, win32con.HWND_TOPMOST if TOPMOST else win32con.HWND_TOP, screen_width - width - 50, 300, width, height, 0
    )
    os.system("title DeskSpy")
    os.system("mode con: cols=30 lines=10")
    printTerminal("-- START --")
else:
    win32gui.ShowWindow(hwnd, win32con.SW_HIDE)

# Open serial connection
ser = serial.Serial(
    "COM5",
    1000000,
    timeout=2000,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
)

# Detect faces
face_detector = cv2.CascadeClassifier("haarcascade_frontalface_default.xml")

# Recognize faces from trained data
face_recognizer = cv2.face.LBPHFaceRecognizer_create()
face_recognizer.read("trainer.yml")

# Create dictionary{ID,name} for every person
labels = {}
with open("labels.pickle", "rb") as f:
    og_label = pickle.load(f)
    labels = {v: k for k, v in og_label.items()}

# Local buffer
buffer = bytearray()
result = -1

# Wait the Syn from the client
byte = ser.read(1)
# Send the Ack
ser.write(b"a")

# Read first byte
byte = ser.read(1)
# Cler the UDR0 buffer from previuos readings
while byte != START_COMMAND:
    byte = ser.read(1)

while True:
    if byte == START_COMMAND:
        # Read one frame
        for i in range(WIDTH * HEIGHT):
            byte = ser.read(1)
            buffer.append(int.from_bytes(byte, byteorder="big"))
        # Reshape buffer into array[HEIGHT][WIDTH]
        img_array = np.array(buffer).reshape(HEIGHT, WIDTH)
        # Convert the array to a grayscale image
        img_array = np.array(Image.fromarray(img_array, "L"))
        if DISPLAY:
            cv2.namedWindow("camera", cv2.WINDOW_NORMAL)
            if TOPMOST:
                cv2.setWindowProperty("camera", cv2.WND_PROP_TOPMOST, 1)
            cv2.imshow("camera", img_array)
            cv2.waitKey(1)
        # Detect faces
        faces = face_detector.detectMultiScale(
            img_array, scaleFactor=1.05, minNeighbors=3
        )
        clock = time.strftime("%H:%M:%S", time.localtime())
        if len(faces) > 0:
            if RECOGNITION:
                # Recognize the faces in trained data
                recognized = False
                for (x, y, w, h) in faces:
                    ID, conf = face_recognizer.predict(img_array[y : y + h, x : x + w])
                    if conf >= 20 and conf <= 115:
                        recognized = True
                        break
                if recognized:
                    printTerminal(clock + " Face recognized, ID: " + labels[ID])
                    result = RECOGNIZED_COMMAND
                else:
                    printTerminal(clock + " Face not recognized")
                    result = NOT_RECOGNIZED_COMMAND
            else:
                printTerminal(clock + " Face detected")
                result = DETECTED_COMMAND
        else:
            printTerminal(clock + " No face detected")
            result = NOT_DETECTED_COMMAND
        ser.write(result)
        # Clear the buffer
        buffer = bytearray()
        result = -1
    # Read next byte
    byte = ser.read(1)
