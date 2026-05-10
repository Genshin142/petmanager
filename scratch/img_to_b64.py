import base64
import os

files = [
    r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\royal_canin_dog_food_1778385348572.png",
    r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\orijen_cat_food_1778385364594.png",
    r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\pet_toy_frisbee_1778385382266.png"
]

for f in files:
    if os.path.exists(f):
        with open(f, "rb") as image_file:
            encoded_string = base64.b64encode(image_file.read()).decode('utf-8')
            # Only print first 50 chars and last 50 chars to avoid flooding
            print(f"File: {os.path.basename(f)}")
            print(f"Base64: {encoded_string[:50]}...{encoded_string[-50:]}")
            print("-" * 20)
    else:
        print(f"File not found: {f}")
