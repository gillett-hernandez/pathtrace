import argparse
from random import random
from typing import Optional
import json
import os
import pygame
import time
from pygame.locals import *

config_path = os.path.abspath("./config.json")

default_image_path: Optional[str] = None
default_paths_path: Optional[str] = None
if os.path.exists(config_path):
    print("loading config")
    with open(config_path, "r") as fd:
        data = json.load(fd)
    default_paths_path = os.path.abspath(
        os.path.join(".", data["traced_paths_2d_output_path"])
    )
    default_image_path = os.path.abspath(
        os.path.join(".", data["ppm_output_path"].replace("ppm", "png"))
    )
else:
    print(f"not loading config, couldn't find at {config_path}")

parser = argparse.ArgumentParser()

parser.add_argument("--paths", type=str, default=default_paths_path)
parser.add_argument("--image", type=str, default=default_image_path)


def main(args):
    pygame.init()
    print(args.image)
    image = pygame.image.load(args.image)
    width, height = image.get_rect().size
    print(width, height)
    display = pygame.display.set_mode((width, height))
    clock = pygame.time.Clock()
    with open(args.paths, "r") as fd:
        paths = fd.read().splitlines()
    actual_paths = [[]]
    for line in paths:
        print(line)
        if line == "":
            actual_paths.append([])
        else:
            if line.endswith("!"):
                line = line[:-1]
                exited_world = True
            else:
                exited_world = False
            p0, p1 = line.split(",")
            actual_paths[-1].append((float(p0), float(p1), exited_world))

    actual_paths.sort(key=lambda l: len(l), reverse=True)
    max_i = 0
    path_idx = 0
    while True:
        for event in pygame.event.get():
            if event.type == KEYDOWN:
                if event.key == K_q or event.key == K_ESCAPE:
                    pygame.quit()
                    return
                elif event.key == K_r:
                    # reset path state for the selected path
                    max_i = 0
                elif event.key == K_SPACE:
                    # next path
                    path_idx += 1
                    max_i = 0
                elif event.key == K_BACKSPACE:
                    path_idx -= 1
                    max_i = 0
            elif event.type == QUIT:
                pygame.quit()
                return
        display.blit(image, (0, 0))

        path = actual_paths[path_idx]
        while len(path) < 2:
            path_idx += 1
            path = actual_paths[path_idx]
        # for i, point in enumerate(paths[path_idx]):
        # if i == 0:
        #     lastpoint = point
        #     continue
        # if i > max_i:
        #     break
        print(path_idx, max_i, path)
        pygame.draw.circle(
            display,
            pygame.color.Color("green"),
            (int(width * path[0][0]), int(height * path[0][1])),
            5,
            0,
        )
        pygame.draw.circle(
            display,
            pygame.color.Color("black" if path[max_i][-1] else "red"),
            (int(width * path[max_i][0]), int(height * path[max_i][1])),
            5,
            0,
        )
        if max_i >= 1 and max_i <= len(path):
            pygame.draw.lines(
                display,
                pygame.color.Color("white"),
                False,
                [
                    (width * x + 3 * random(), height * y + 3 * random())
                    for x, y, exited_world in path[: max_i + 1]
                ],
                3,
            )
        if max_i < len(path) - 1:

            max_i += 1
        pygame.display.flip()
        clock.tick(60)
    pygame.quit()


if __name__ == "__main__":
    main(parser.parse_args())
