import os.path
import subprocess


ycToken = None
yavToken = None


def getYcToken():
    global ycToken
    if ycToken is None:
        with open(os.path.expanduser("~/.ssh/yc_token")) as f:
            ycToken = f.read().strip()
    return ycToken


def getYavToken():
    global yavToken
    if yavToken is None:
        with open(os.path.expanduser("~/.ssh/yav_token")) as f:
            yavToken = f.read().strip()
    return yavToken


def addSshKey():
    subprocess.call(["ssh-add", os.path.expanduser("~/.ssh/id_rsa")])


addSshKey()
