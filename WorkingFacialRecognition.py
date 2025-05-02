import os
os.environ["TF_ENABLE_ONEDNN_OPTS"] = "0"

import tensorflow as tf
from keras.models import load_model
import cv2
import serial
import time
import atexit
import numpy as np
from tensorflow.keras.layers import DepthwiseConv2D

# Disable scientific notation for clarity
np.set_printoptions(suppress=True)

# Custom layer fix for loading Keras model
class CustomDepthwiseConv2D(DepthwiseConv2D):
    def __init__(self, *args, **kwargs):
        kwargs.pop("groups", None)
        super().__init__(*args, **kwargs)

# Connect to Arduino
arduino = serial.Serial(port='/dev/cu.usbmodem101', baudrate=9600, timeout=1)
time.sleep(2)  # Allow Arduino to initialize

def send_command(command):
    """Send a command to the Arduino and wait briefly for a response."""
    arduino.write(f"{command}\n".encode('utf-8'))
    time.sleep(0.05)
    response = arduino.readline().decode('utf-8').strip()
    return response

def cleanup():
    """Cleanup function to turn off LEDs and close Arduino connection."""
    print("Exiting and closing Arduino connection...")
    send_command("EXIT")
    arduino.close()

atexit.register(cleanup)

# Load the model
model = load_model(
    "/Users/jeffreychopin/Desktop/IT254/converted_keras/keras_model.h5",
    compile=False,
    custom_objects={"DepthwiseConv2D": CustomDepthwiseConv2D}
)

# Load class labels
class_names = open("/Users/jeffreychopin/Desktop/IT254/converted_keras/labels.txt", "r").readlines()

# Start webcam
camera = cv2.VideoCapture(0)  # Adjust if using a different camera index

print("System ready. Waiting for SCAN_FACE signal from Arduino...")

while True:
    # Check if Arduino has sent a command
    if arduino.in_waiting > 0:
        message = arduino.readline().decode('utf-8').strip()
        print("Arduino says:", message)

        if message == "SCAN_FACE":
            print("Capturing image for face recognition...")

            # Capture image
            ret, image = camera.read()
            if not ret:
                print("Failed to capture image.")
                arduino.write(b"NO_MATCH\n")
                continue

            # Resize, normalize, and preprocess image
            image_resized = cv2.resize(image, (224, 224), interpolation=cv2.INTER_AREA)
            image_array = np.asarray(image_resized, dtype=np.float32).reshape(1, 224, 224, 3)
            image_array = (image_array / 127.5) - 1

            # Display webcam preview
            cv2.imshow("Webcam Image", image_resized)
            cv2.waitKey(1)  # Needed to show image window

            # Predict
            prediction = model.predict(image_array)
            index = np.argmax(prediction)
            class_name = class_names[index]
            confidence_score = prediction[0][index]

            print(f"Class: {class_name.strip()} | Confidence: {confidence_score:.2f}")

            if class_name.strip() == "0 Accepted Faces" and confidence_score >= 0.8:
                arduino.write(b"MATCH\n")
                print("MATCH sent to Arduino")
            else:
                arduino.write(b"NO_MATCH\n")
                print("NO_MATCH sent to Arduino")

    # Escape key exits
    if cv2.waitKey(1) == 27:
        break

# Cleanup
camera.release()
cv2.destroyAllWindows()
