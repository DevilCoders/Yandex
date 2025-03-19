import os
import sys

hasColorSupport = (os.getenv("TERM") != "") and sys.stdout.isatty()


def green(text):
    if not hasColorSupport:
        return text
    return "\033[32m" + str(text) + "\033[0m"


def yellow(text):
    if not hasColorSupport:
        return text
    return "\033[33m" + str(text) + "\033[0m"


def red(text):
    if not hasColorSupport:
        return text
    return "\033[31m" + str(text) + "\033[0m"
