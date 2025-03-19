import logging
import os
import subprocess

COMMANDS = [
    "afplay", # MacOS
    "paplay", # Linux (Pulseaudio)
    "aplay" # Linux (ALSA)
]

def play(filename):
    for bin in COMMANDS:
        cmd = [bin, os.path.abspath(os.path.join(os.path.dirname(__file__), "data", filename))]
        logging.debug("Running %s", cmd)
        try:
            ret = subprocess.call(cmd)
            if ret == 0:
                logging.debug("Successfully called %s", cmd)
                return
        except OSError:
            pass
    logging.debug("Could not find any command to play sound with")
