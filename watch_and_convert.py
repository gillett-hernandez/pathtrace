import os
from PIL import Image
import time

def main():
    while True:
        for a,b,c in os.walk("."):
            for file in c:
                if file.endswith(".ppm"):
                    try:
                        im = Image.open(file)
                        im.save(file.replace(".ppm", ".png"))
                        os.remove(file)
                    except:
                        pass
        time.sleep(5)

if __name__ == "__main__":
    main()
