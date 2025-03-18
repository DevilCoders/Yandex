#!/usr/bin/env python

import sys;
import optparse;
import os.path;
import subprocess;

import gen_aggregated_factor_names;

def GetCommandLineParams():
    optParser = optparse.OptionParser();
    optParser.add_option("-d", "--antirobot_daemon", action="store", dest="antirobot_daemon",
            default="./antirobot_daemon", help="antirobot_daemon binary");
    optParser.add_option("-f", "--factors", action="store", dest="factors_file" ,
            default=sys.stdin, help="factors, produced by antirobot_evlogdump");
    (options, args) = optParser.parse_args();

    if not os.path.isfile(options.antirobot_daemon):
        optParser.error("Error: cannot find '%s'" % options.antirobot_daemon);

    return options;


def GetFactorNames(options):
    args = [options.antirobot_daemon, "-F"];
    daemonProc = subprocess.Popen(args, stdout = subprocess.PIPE, stderr = open("antirobot_daemon_err", "wt"));
    factorNames = gen_aggregated_factor_names.make_aggregated_names(daemonProc.stdout);
    daemonProc.wait();
    return factorNames;


def main():
    options = GetCommandLineParams();

    factorNames = GetFactorNames(options);
    numFactors = len(factorNames);

    if options.factors_file != sys.stdin:
        options.factors_file = open(options.factors_file, 'r')

    for line in options.factors_file:
        (timeStamp, frameId, evType, reqId, factorsStr) = line.split('\t');
        factorValues = factorsStr.rstrip()[1:-1].split(",");

        if len(factorValues) != numFactors:
            print >>sys.stderr, "invalid number of factors in line: %s" % line,;
            continue;

        print "%s\t%s\t%s" % (timeStamp, reqId, " ".join("%s=%s" % (name, value) for name, value in zip(factorNames, factorValues)));



if __name__ == "__main__":
    main();

