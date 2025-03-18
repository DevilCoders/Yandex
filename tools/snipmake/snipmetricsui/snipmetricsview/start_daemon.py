#!/usr/bin/python
# -*- coding: utf-8 -*-

from daemon import Daemon
from sys import argv
import sys
import os
from subprocess import Popen
from options import get_option_value
from log import writeToLog

class AppDaemon(Daemon):
    def __init__(self, cmd, args, pidfile, stdin, stdout, stderr, cwd):
       Daemon.__init__(self, pidfile, stdin, stdout, stderr)
       self.cmd = cmd
       self.args = args
       self.currentDir = cwd

    def run(self):
        self.process = Popen(self.cmd + " " + " ".join(["'" + arg + "'" for arg in self.args]), shell = True, cwd = self.currentDir)

if __name__ == "__main__":
    try:
       d = AppDaemon(   "python " + get_option_value("homeDir") + "snipmetricsview/tasks.py", argv[1:],
                        get_option_value("logDir") + "pid",
                        "/dev/null",
                        get_option_value("logDir") + "output.txt",
                        get_option_value("logDir") + "error.log",
                        get_option_value("homeDir") + "snipmetricsview/")
       d.start()
    except SystemExit:
        pass
    except:
        writeToLog("Deamon thread: %s\n%s" % (argv[1:], str(sys.exc_info())) )
