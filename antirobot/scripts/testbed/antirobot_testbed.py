#!/usr/bin/env python

import sys;
import optparse;
import subprocess;
import os;
import os.path;
import time;

SIGTERM = 15;

DAEMON_PORT = "13456";
DAEMON_LOG_FILE = "antirobot_daemon.log";
DAEMON_PID_FILE = "antirobot_daemon.pid";
TOOL_PID_FILE = "antirobot_tool.pid";

def CheckFile(optparser, fname):
    if not os.path.isfile(fname):
        optparser.error("no file '%s'" % fname);

def MakeCorrectBinaryPath(fname):
    if not fname.startswith("/"):
        return "./" + fname;
    else:
        return fname;

def GetCommandLineParams():
    optParser = optparse.OptionParser();
#    optParser.add_option("-C", "--testbedcfg", action="store", dest="tbCfg", help="testbed config file");
    optParser.add_option("-t", "--antirobot_tool", action="store", dest="antirobot_tool", default="./antirobot_tool", help="antirobot_tool binary");
    optParser.add_option("-d", "--antirobot_daemon", action="store", dest="antirobot_daemon", default="./antirobot_daemon", help="antirobot_daemon binary");
    optParser.add_option("-c", "--config", action="store", dest="tool_config", default="antirobot_threat_check.cfg", help="antirobot tool config");
    optParser.add_option("-C", "--daemon-config", action="store", dest="daemon_config", default=None,
            help="antirobot daemon config (if not specified, antirobot config is taken)");
    optParser.add_option("-w", "--whitelist", action="store", dest="whitelist", default="none", help="whitelist file");
    optParser.add_option("-B", "--blacklist", action="store", dest="blacklist", default="none", help="blacklist file");
    optParser.add_option("-f", "--file", action="append", dest="logfiles", default=None, help="log file for antirobot_tool (required)");
    optParser.add_option("", "--oldtime", action="store_true", dest="oldtime", default=False, help="Convert time to seconds for old antirobot versions");
    optParser.add_option("-l", "--local", action="store_true", dest="localmode", default=False, help="Use antirobot_daemon in local mode, so no antirobot_tool is needed");
    optParser.add_option("-o", "--out-file", action="store", dest="out_file", default=False, help="Output file name for antirobot_daemon.log");

    (options, args) = optParser.parse_args();

    optParser.set_usage("%prog -f <log_file> [other options]");

    if not options.logfiles:
        optParser.error("Error: no log file specified");

    if not options.daemon_config:
        options.daemon_config = options.tool_config;

    CheckFile(optParser, options.tool_config);
    CheckFile(optParser, options.daemon_config);

    if options.localmode:
        options.antirobot_tool = "notool";
    else:
        CheckFile(optParser, options.antirobot_tool);
        options.antirobot_tool = MakeCorrectBinaryPath(options.antirobot_tool);

    CheckFile(optParser, options.antirobot_daemon);
    options.antirobot_daemon = MakeCorrectBinaryPath(options.antirobot_daemon);

    realToolConfig = "antirobot_testbed_tool_cfg_tweaked";
    TweakConfig(options.tool_config, realToolConfig);
    options.originalToolConfig = options.tool_config;
    options.tool_config = realToolConfig;

    realDaemonConfig = "antirobot_testbed_daemon_cfg_tweaked";
    TweakConfig(options.daemon_config, realDaemonConfig);
    options.originalDaemonConfig = options.daemon_config;
    options.daemon_config = realDaemonConfig;

    if not options.out_file:
        options.out_file = ("%s.%d.%s.%s.%s.%s" % (DAEMON_LOG_FILE, int(time.time()), CollapseFileName(options.logfiles[0]),
            CollapseFileName(options.antirobot_tool),
            CollapseFileName(options.antirobot_daemon), CollapseFileName(options.originalToolConfig)))[:100];

    return options;

def RemoveRF(path):
    for f in os.listdir(path):
        fullPath = "%s/%s" % (path, f);
        if os.path.isdir(fullPath):
            remove_recurse(fullPath);
        else:
            os.remove(fullPath);
    os.rmdir(path);

def RemoveIfExists(fileName):
    if os.path.isfile(fileName):
        os.remove(fileName);
    elif os.path.isdir(fileName):
        RemoveRF(fileName);

def MoveIfExists(src, dst):
    if os.path.exists(src):
        os.rename(src, dst);

def KillByPidFile(pidFileName):
    try:
        pf = open(pidFileName);
        pid = int(pf.readline());
        pf.close();
        os.kill(pid, SIGTERM);
    except:
        pass;

def TweakConfig(configFileName, tweakedConfigFileName):
    inFile = open(configFileName);
    outFile = open(tweakedConfigFileName, "wt");
    for line in inFile:
        l1 = line.strip();
        if l1.startswith("DebugOutput"):
            print >>outFile, "DebugOutput = 0";
        elif l1.startswith("IgnoreYandexIps"):
            print >>outFile, "IgnoreYandexIps = 1";
        elif l1.startswith("DbDumpInterval"):
            print >>outFile, "DbDumpInterval = 10000";
        elif l1.startswith("XmlHandlersPingInterval"):
            print >>outFile, "XmlHandlersPingInterval = 1800";
        elif l1.startswith("DaemonsPingInterval"):
            print >>outFile, "DaemonsPingInterval = 1800";
        elif l1.startswith("XmlHandlersTimeoutMs"):
            print >>outFile, "XmlHandlersTimeoutMs = 10000";
        elif l1.startswith("DaemonsTimeoutMs"):
            print >>outFile, "DaemonsTimeoutMs = 10000";
        elif l1.startswith("XmlHandlers "):
            print >>outFile, "XmlHandlers = localhost:%s" % DAEMON_PORT;
        elif l1.startswith("Daemons "):
            print >>outFile, "Daemons = localhost:%s" % DAEMON_PORT;
        else:
            print >>outFile, line,;
    inFile.close();
    outFile.close();
        
def GetAntirobotToolOutputDir(logFileIndex):
    return "antirobot_tool_out_%d" % logFileIndex;
    
def RunDaemon(options):
    RemoveIfExists(DAEMON_LOG_FILE);
    RemoveIfExists("antirobot_persistence");
    RemoveIfExists("antirobot_factors");
    KillByPidFile(DAEMON_PID_FILE);

    port = "0" if options.localmode else DAEMON_PORT;

    args = [options.antirobot_daemon, "-p", port, "-c", options.daemon_config, "-P", DAEMON_PID_FILE];
    if options.whitelist != "none":
        args += ["-w", options.whitelist];

    if options.blacklist != "none":
        args += ["-B", options.blacklist];

    return subprocess.Popen(args, stdout = subprocess.PIPE, stderr = open("antirobot_daemon_err", "wt"),
            stdin = open(options.logfiles[0]) if options.localmode else None);

def RunTool(options, logFileIndex):
    outputDir = GetAntirobotToolOutputDir(logFileIndex);
    pidFileName = "%s/%s" % (outputDir, TOOL_PID_FILE);

    KillByPidFile("%s/%s" % (outputDir, pidFileName));
    RemoveIfExists(outputDir);
    os.mkdir(outputDir);


    args = [options.antirobot_tool, "threat_check",
            "-c", options.tool_config,
            "-P", pidFileName,
            "-o", outputDir];

    if options.whitelist != "none":
        args += ["-w", options.whitelist];

    if options.blacklist != "none":
        args += ["-B", options.blacklist];

    if not options.oldtime:
        args += ["-f", options.logfiles[logFileIndex]];
        return subprocess.Popen(args);#, stdout = subprocess.PIPE, stderr = subprocess.PIPE);
    else:
        print "Running %s/adapt_time.py %s" % (os.path.dirname(sys.argv[0]), options.logfiles[logFileIndex]);
        adaptProcess = subprocess.Popen(["%s/adapt_time.py" % os.path.dirname(sys.argv[0]), options.logfiles[logFileIndex]], stdout = subprocess.PIPE);
        print "Running %s" % " ".join(args);
        return subprocess.Popen(args, stdin = adaptProcess.stdout);#, stdout = subprocess.PIPE, stderr = subprocess.PIPE);


def CollapseFileName(fileName):
    return fileName.replace(".", "").replace("/", "_").replace("\\", "_");


def CalcQuality(markedAccessLogFileName, daemonLogFileName):
    mf = open(markedAccessLogFileName);
    df = open(daemonLogFileName);

    reqidFromMf = "";

    truePos = 0;
    falsePos = 0;
    trueNeg = 0;
    falseNeg = 0;

    for daemonLine in df:
        daemonFields = daemonLine.split('\t');
        reqidFromDaemon = daemonFields[14].rstrip();

        if len(reqidFromDaemon) < 17 or len(reqidFromDaemon.split('-')) < 2:
            continue;

        reqType = daemonFields[4];
        if reqType != "web-ys" and reqType != "news-news":
            continue;

        while not reqidFromMf.startswith(reqidFromDaemon):
            markedLine = mf.readline();
            if not markedLine:
                break;

            markedFields = markedLine.split('\t');
            if len(markedFields) < 2:
                print "Log file is not marked up, so no quality is calculated";
                return;

            reqidFromMf = markedFields[0].split(' ')[-2];

        if reqidFromMf == reqidFromDaemon:
            isEnemyMarkup = markedFields[1].rstrip() == "1";
            isEnemyDaemon = daemonLine.startswith("ENEMY");

            if isEnemyDaemon:
                if isEnemyMarkup:
                    truePos += 1;
                else:
                    falsePos += 1;
            else:
                if isEnemyMarkup:
                    falseNeg += 1;
                else:
                    trueNeg += 1;

    precision = (float(truePos) + 1.0) / (truePos + falsePos + 2.0);
    recall = (float(truePos) + 1.0) / (truePos + falseNeg + 2.0);

    print "true pos: %d, false pos: %d, true neg: %d, false neg: %d\nprecision: %0.2f%%, recall: %0.2f%%" % (
        truePos, falsePos, trueNeg, falseNeg, precision * 100.0, recall * 100.0);
 

def main():
    options = GetCommandLineParams();

    daemon = RunDaemon(options);
    time.sleep(5);

    if options.localmode:
        daemon.wait();
    else:
        toolPids = []
        for i in range(0, len(options.logfiles)):
            toolPids.append(RunTool(options, i));

        for tool in toolPids:
            tool.wait();

        os.kill(daemon.pid, SIGTERM);

    print "renaming %s to %s" % (DAEMON_LOG_FILE, options.out_file);
    
    CalcQuality(options.logfiles[0], DAEMON_LOG_FILE);

    MoveIfExists(DAEMON_LOG_FILE, options.out_file);




if __name__ == "__main__":
    main();

