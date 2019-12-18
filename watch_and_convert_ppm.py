#!/usr/bin/env python3

import os
import time

if __name__ == "__main__":
    while True:
        os.system(
            "python3 " + os.path.abspath("./convert_ppm_in_curdir.py") + " --no-delete"
        )
        time.sleep(1)
