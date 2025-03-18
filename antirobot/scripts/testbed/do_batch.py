#!/usr/local/bin/python

import sys;
import subprocess;
import os.path

def main():
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: do_bacth.py <batch file> <log file>";
        sys.exit(1);

    batchFile = sys.argv[1];
    logFile = sys.argv[2];

    for line in open(batchFile):
        (daemon, tool, config, oldtime) = line.strip().split("\t");
        
        commandLine = "%s/antirobot_testbed.py -t %s -d %s -c %s -f %s %s" % (
                os.path.dirname(sys.argv[0]), tool, daemon, config, logFile, "--oldtime" if oldtime == "1" else "");
        print "running %s" % commandLine;

        testbedProc = subprocess.Popen(commandLine.split(' '));#, stdout = subprocess.PIPE, stderr = subprocess.PIPE);
        testbedProc.wait();


if __name__ == "__main__":
    main();

