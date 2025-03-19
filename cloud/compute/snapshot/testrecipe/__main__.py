import subprocess as sp

from library.python.testing.recipe import declare_recipe


def start(argv):

    sp.call(["docker", "load", "-i", "snapshot-qemu-nbd-docker-image.tar"])


def stop(argv):
    pass


if __name__ == "__main__":
    declare_recipe(start, stop)
