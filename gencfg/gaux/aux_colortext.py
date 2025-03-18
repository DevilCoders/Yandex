import sys

RESET_SEQ = "\033[0m"
COLOR_SEQ = "\033[1;%dm"


def red_text(text):
    if sys.stdout.isatty():
        return COLOR_SEQ % 31 + str(text) + RESET_SEQ
    else:
        return str(text)


def green_text(text):
    if sys.stdout.isatty():
        return COLOR_SEQ % 32 + text + RESET_SEQ
    else:
        return str(text)


def blue_text(text):
    if sys.stdout.isatty():
        return COLOR_SEQ % 36 + text + RESET_SEQ
    else:
        return str(text)


def yellow_text(text):
    if sys.stdout.isatty():
        return COLOR_SEQ % 33 + text + RESET_SEQ
    else:
        return str(text)


def dblue_text(text):
    if sys.stdout.isatty():
        return COLOR_SEQ % 34 + text + RESET_SEQ
    else:
        return str(text)
