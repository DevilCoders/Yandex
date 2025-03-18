#!/usr/bin/env python

import sys
import re
import os
import traceback
import optparse

ARCADIA_ROOT="../../../../"

RUNNER_LIB_PATH = ARCADIA_ROOT + "yweb/scripts/datascripts/runnerLib"
sys.path.append(RUNNER_LIB_PATH)
import common

SCRIPTS_DIR_PATH = ARCADIA_ROOT + "yweb/scripts/collect_base"
sys.path.append(SCRIPTS_DIR_PATH)
import slice_local_base
import m2oneBase
import collectConfig

IGNORE_EXCEPTIONS = False
VERBOSE = False

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
            raise Exception("Required file not found: %s/%s" % (binDir, description.getName()))

def PrintRequiredBinaries(requiredBinaries):
    for description in requiredBinaries:
        print description.getName(), "\tPossible location:", description.getLocation()


class ListPlanCreator:
    def __init__(self, docIdListPath):
        self.DocIdListPath = docIdListPath
        self.DocIdList = []
        for line in open(docIdListPath):
            self.DocIdList.append(int(line))
            self.DocIdList.sort()

    def getIter(self, docidscount):
        newdocid = 0
        for docid in self.DocIdList:
            yield (docid, newdocid)
            newdocid += 1

    def getSuffix(self):
        processedFileName = re.sub("/", "-", self.DocIdListPath)
        return "l-" + processedFileName

def CheckRuleFiles(dbDir, rule):
    for filename in rule.getFilenames():
        if not os.path.isfile("%s/%s" % (dbDir, filename)):
            return False
    return True

def PrepareRemapRules(rules):
    preparedRules = rules
    preparedRules.append(collectConfig.RemapRule("index", "tag", False, False, ["indextag", "indextdr"]))
    return preparedRules

def VerbosePrint(message):
    if VERBOSE:
        print >> sys.stderr, message

def ExceptionInteraction():
    try:
        raise
    except (KeyboardInterrupt, SystemExit):
        raise
    except:
        if not IGNORE_EXCEPTIONS:
            raise
        else:
            if VERBOSE:
                traceback.print_exc(file = sys.stderr)


def slice(dbDir, resultDir, planCreator, remapRules):
    CheckRequiredBinaries(slice_local_base.BINS_DIR, REQUIRED_BINARIES)

    m2oneBase.recreateDir(slice_local_base.TEMP_DIR)
    m2oneBase.recreateDir(resultDir)

    if not os.path.isfile(slice_local_base.WORDWHTKEYFILE):
        tempFile = open(slice_local_base.WORDWHTKEYFILE, "w")
        tempFile.close()

    searchplanpath = "%s/search.plan" % slice_local_base.TEMP_DIR
    archiveplanpath = "%s/archive.plan" % slice_local_base.TEMP_DIR
    if not os.path.isfile(searchplanpath) or not os.path.isfile(archiveplanpath):
        slice_local_base.makeplan(dbDir, searchplanpath, archiveplanpath, planCreator)

    VerbosePrint("Starting database remapping...")
    sourceDatabase = m2oneBase.Database(dbDir)
    resultDatabase = m2oneBase.Database(resultDir)
    remapper = m2oneBase.DatabaseRemapper(slice_local_base.BINS_DIR, "%s/remap" % \
                                          slice_local_base.TEMP_DIR, [sourceDatabase], searchplanpath, archiveplanpath)
    succeededTasks = []
    failedTasks = []
    skippedTasks = []
    for rule in remapRules:
        VerbosePrint("Remapping: %s" % rule.__str__())
        if CheckRuleFiles(dbDir, rule):
            try:
                remapper.remapIndex(rule, resultDir, forced = False)
                succeededTasks.append(rule.__str__())
            except:
                ExceptionInteraction()
                failedTasks.append(rule.__str__())
        else:
            skippedTasks.append(rule.__str__())
            VerbosePrint("Remapping: not enough files for %s, skipping" % rule.__str__())

    try:
    except:
        ExceptionInteraction()


    VerbosePrint("Copying remaining files...")
    for f in collectConfig.ADDITIONAL_FILES:
        if os.path.isfile("%s/%s" % (dbDir, f)):
            VerbosePrint("Copying: %s" % f)
            common.sExec("cp %s/%s %s/%s" % (dbDir, f, resultDir, f))
            succeededTasks.append(f)
        else:
            skippedTasks.append(f)
            VerbosePrint("Copying: %s nonexists, skipping" % f)

    VerbosePrint("Making arr...")
    makeArrTaskName = "makeArr"
    try:
        slice_local_base.makeArr(resultDir)
        succeededTasks.append(makeArrTaskName)
    except:
        ExceptionInteraction()
        failedTasks.append(makeArrTaskName)

    VerbosePrint("Finished!")

    print "---------------------------------------------"
    print "Database slice summary:"
    print "Target base:\n\t", dbDir
    print "Sliced base:\n\t", resultDir
    print "Tasks succeeded: %s" % len(succeededTasks)
    print "Tasks skipped: %s\n\t" % len(skippedTasks), "\n\t".join(skippedTasks)
    if len(failedTasks) > 0:
        print "Tasks failed: %s\n\t" % len(failedTasks), "\n\t".join(failedTasks)
    print "---------------------------------------------"

if __name__ == '__main__':
    try:
        usage = ["%prog [options] fromBaseDir resultBaseDir docIdList",
                 "%prog --list-required-files",
                 "details: http://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/SnipML/LocalBaseOperations/slice"]
        parser = optparse.OptionParser("\n" + "\n".join(usage))
        parser.add_option("-l", "--list-required-files", action="store_true", dest = "listRequiredFiles", default = False, help = "lists files, required for work and exit")
        parser.add_option("-t", "--tempdir", action="store", dest = "tempDir", default = "temp", help = "path to directory with temporary files, './temp' by default")
        parser.add_option("-b", "--binsdir", action="store", dest = "binsDir", default = "bin", help = "path to direcory with binary files, '.' by default")
        parser.add_option("-w", "--wordwheight", action="store", dest = "wordwheight", help = "word wheight file, empty file by default")
        parser.add_option("-i", "--ignore-exceptions", action="store_true", dest = "ignoreExceptions", help = "don't stop if some exception will be raised")
        parser.add_option("-v", "--verbose", action="store_true", dest = "verbose", help = "be verbose, tell all what is doing")
        (options, args) = parser.parse_args()

        VERBOSE = options.verbose
        slice_local_base.TEMP_DIR = options.tempDir
        slice_local_base.BINS_DIR = options.binsDir

        if options.listRequiredFiles:
            PrintRequiredBinaries(REQUIRED_BINARIES)
            sys.exit(1)

        if len(args) != 3:
            parser.print_help()
            sys.exit(1)
        else:
            dbDir           = args[0]
            resultDir       = args[1]
            docIdListPath   = args[2]

        if os.path.isdir(slice_local_base.TEMP_DIR) == False:
            os.mkdir(slice_local_base.TEMP_DIR)

        if options.wordwheight:
            slice_local_base.WORDWHTKEYFILE = options.wordwheight
        else:
            tempWordKeyFilePath = "%s/wordwht" % slice_local_base.TEMP_DIR
            slice_local_base.WORDWHTKEYFILE = tempWordKeyFilePath

        if options.ignoreExceptions:
            IGNORE_EXCEPTIONS = True

        planCreator = ListPlanCreator(docIdListPath)

        remapRules = PrepareRemapRules(collectConfig.SEARCH_REMAP_RULES)
        slice(dbDir, resultDir, planCreator, remapRules)

    except Exception, e:
        print >> sys.stderr, "Exception caught: %s" % e
        if VERBOSE:
            traceback.print_exc(file = sys.stderr)

