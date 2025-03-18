#!/usr/bin/env python

import optparse
import os
import sys

ARCADIA_ROOT="../../../../"

RUNNER_LIB_PATH = ARCADIA_ROOT + "yweb/scripts/datascripts/runnerLib"
sys.path.append(RUNNER_LIB_PATH)
import common

SCRIPTS_DIR_PATH = ARCADIA_ROOT + "yweb/scripts/collect_base"
sys.path.append(SCRIPTS_DIR_PATH)
import m2one
import m2oneBase
import collectConfig
import slice_local_base

class BinaryDescription():
    def __init__(self, name, location):
        self.name = name
        self.location = location
    def getName(self):
        return self.name
    def getLocation(self):
        return self.location

REQUIRED_BINARIES = [ \
    BinaryDescription("datawork", "arcadia/yweb/robot/datawork"), \
    BinaryDescription("m2nsort", "arcadia/yweb/robot/m2nsort"), \
    ]

def CheckRequiredBinaries(binDir, requiredBinaries):
    for description in requiredBinaries:
        if not os.path.isfile("%s/%s" % (binDir, description.getName())):
            return False
    return True

def PrintRequiredBinaries(requiredBinaries):
    print "Required binaries:"
    for description in requiredBinaries:
        print description.getName(), "\tPossible location:", description.getLocation()
    print ""

def GetLocalhost(tmpDir):
    return m2one.Server("localhost", tmpDir)

def GetLocalDirs(tmpDir, binDir):
    return m2one.ServerDirs(GetLocalhost(tmpDir), binDir)

def prepareMergeRemaps(srcBases, dstBase, localDirs):
    if dstBase.isRemote():
        raise Exception()
    dstBaseTemp = localDirs.getDBDirs(dstBase).getTmpDir()

    m2oneBase.createDir(dstBaseTemp)

    planPath = "%s/plan" % dstBaseTemp

    if not os.path.isfile(planPath):
        m2oneBase.createPlansFromSearchBases(srcBases, localDirs.getBinsDir(), planPath)

def CheckRuleName(rule):
    if rule.Prefix == "indexherf" or rule.Prefix == "indexerf":
        return False
    return True

def CheckRuleFiles(dbDir, rule):
    for filename in rule.getFilenames():
        if not os.path.isfile("%s/%s" % (dbDir, filename)):
            return False
    return True

def SkipRemapRule(rule, srcBases):
    if not CheckRuleName(rule):
        return True, "bad rule"
    for base in srcBases:
        if not CheckRuleFiles(base.getDir(), rule):
            return True, "not enough files"
    return False, "Ok"

def mergeBases(srcBases, dstBase, remapRules, tmpDir, binDir, wordwheight):
    if len(srcBases) < 2:
        raise Exception("There are not enough bases to merge")

    localDirs = GetLocalDirs(tmpDir, binDir)
    prepareMergeRemaps(srcBases, dstBase, localDirs)
    m2oneBase.createDir(dstBase.getDir())

    for rule in remapRules:
        skip, why = SkipRemapRule(rule, srcBases)
        if not skip:
            mergeBasesFile(srcBases, dstBase, rule, localDirs)
        else:
            m2one.VerbosePrinter.Message("Skipping %s: %s", rule, why)

    if os.path.isfile("%s/index.refkey" % dstBase.getDir()):
        if not os.path.isfile(wordwheight):
            tempFile = open(wordwheight, "w")
            tempFile.close()
        m2one.VerbosePrinter.Message("Making index.refaww")

    m2one.VerbosePrinter.Message("Making indexarr and indexiarr")
    slice_local_base.makeArr(dstBase.getDir())


def mergeBasesFile(srcBases, dstBase, rule, localDirs):
    m2one.VerbosePrinter.Message("Running merge %s", rule)

    dirs = localDirs
    dbDirs = dirs.getDBDirs(dstBase)
    remapDirs = dbDirs.getRemapDirs(rule)

    planPath = "%s/plan" % dbDirs.getTmpDir()

    mergeDone = True
    for filename in rule.getFilenames():
        if not os.path.isfile("%s/%s" % (dstBase.getDir(), filename)):
            mergeDone = False
            break

    if not mergeDone:
        m2oneBase.createDir(dstBase.getDir())
        remapper = m2oneBase.DatabaseRemapper(dirs.getBinsDir(), remapDirs.getTmpDir(), srcBases, planPath)
        remapper.remapIndex(rule, dstBase.getDir())


if __name__ == '__main__':
    usage = ["%prog [options] srcDir1 ... srcDirN resultDir",
                 "%prog -l",
             "details: http://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/SnipML/LocalBaseOperations/merge"]
    parser = optparse.OptionParser("\n" + "\n".join(usage))
    parser.add_option("-v", "--verbose", action = "store_true", dest = "verbose", default = False, help = "be verbose")
    parser.add_option("-t", "--tempdir", action = "store", dest = "tempdir", default = "temp", help = "temp dir")
    parser.add_option("-b", "--binsdir", action = "store", dest = "binsdir", default = "bin", help = "dir with binaries")
    parser.add_option("-w", "--wordwheight", action="store", dest = "wordwheight", help = "word wheight file, empty file by default")
    parser.add_option("-l", "--list-required-binaries", action="store_true", default = False, dest = "listbin", help = "list required binaries and exit")
    (options, args) = parser.parse_args()
    m2one.VERBOSE = options.verbose
    tmpDir = options.tempdir
    binDir = options.binsdir
    if options.listbin or not CheckRequiredBinaries(binDir, REQUIRED_BINARIES):
        PrintRequiredBinaries(REQUIRED_BINARIES)
        parser.print_help()
        sys.exit(1)
    if options.wordwheight:
        wordwheight = options.wordwheight
    else:
        tempWordKeyFilePath = "%s/wordwht" % tmpDir
        wordwheight = tempWordKeyFilePath

    srcBasesArgs = args[0:-1]
    print srcBasesArgs
    if len(srcBasesArgs) < 2:
        parser.error("You should specify at least two source bases and result directory")

    srcBases = []
    for i, dbDir in enumerate(srcBasesArgs):
        srcBases.append(m2one.RemappingDatabase(GetLocalhost(tmpDir), dbDir, "src-%s-%s" % (i, os.path.basename(dbDir))))
    dstBase = m2one.RemappingDatabase(GetLocalhost(tmpDir), args[-1], "result")

    remapRules = collectConfig.SEARCH_REMAP_RULES
    remapRules.append(collectConfig.RemapRule("index", "tag", False, False, ["indextag", "indextdr"]))

    mergeBases(srcBases, dstBase, remapRules, tmpDir, binDir, wordwheight)

