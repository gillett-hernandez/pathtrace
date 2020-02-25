#!/usr/bin/env python3
# script to be run pre-render

import os
import json

if __name__ == "__main__":
    with open("config.json", "r") as fd:
        data = json.load(fd)
    output_paths = []
    output_paths.append(data["ppm_output_path"])
    output_paths.append(data["png_output_path"])
    output_paths.append(data["traced_paths_output_path"])
    output_paths.append(data["traced_paths_2d_output_path"])

    for output_path in output_paths:
        directory, filename = os.path.split(output_path)
        os.makedirs(directory, exist_ok=True)
