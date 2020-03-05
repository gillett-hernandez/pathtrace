#!/usr/bin/env python3

import os
import time

if __name__ == "__main__":
    while True:
        os.system(
            "python3 "
            + os.path.abspath("./convert_ppm.py")
            + " --no-delete --path output"
        )
        time.sleep(0.5)
