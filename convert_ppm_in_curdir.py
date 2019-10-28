#!/usr/bin/env python3
import os
from PIL import Image
import time


def main():
    for a, b, c in os.walk("."):
        for file in c:
            if file.endswith(".ppm"):
                try:
                    im = Image.open(file)
                    im.save(file.replace(".ppm", ".png"))
                    os.remove(file)
                except:
                    pass


if __name__ == "__main__":
    main()
