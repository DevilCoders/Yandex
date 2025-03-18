#!/usr/bin/env python

import os
import optparse
import shutil
import sys

SOURCE_DIR = "/place/home/alivshits/SHARED/reindex_useful_files"

REQUIRED_FOLDERS = [
    "config" \
    , "temp" \
    , "walrus" \
    , "workindex" \
    , "yandex" \
    ]

REQUIRED_FILES = ["filter.obj", "urlmenu.trie"]

REQUIRED_LINKS = ["config", "filter.obj", "urlmenu.trie"]

def CheckDirTree(srcDir):
    checkedResult = True
    if not os.path.isdir(srcDir):
        return False
    for dirname in REQUIRED_FOLDERS:
        dirPath = os.path.join(srcDir, dirname)
        if not os.path.isdir(dirPath):
            print "not exist: ", dirPath
            checkedResult = False

    for filename in REQUIRED_FILES:
        filePath = os.path.join(srcDir, filename)
        if not os.path.isfile(filePath):
            print "not exist: ", filePath
            checkedResult = False

    return checkedResult

def CopyFolderIfNeed(srcDir, destDir, folder):
    if folder == "config":
        return

    srcDirPath = os.path.join(srcDir, folder)
    destDirPath = os.path.join(destDir, folder)
    if not os.path.isdir(destDirPath):
        shutil.copytree(srcDirPath, destDirPath)

def CreateSymLinkIfNeed(srcDir, destDir, linkname):
    srcLinkPath = os.path.join(srcDir, linkname)
    destLinkPath = os.path.join(destDir, linkname)
    if not os.path.islink(destLinkPath):
        os.symlink(srcLinkPath, destLinkPath)

if __name__ == '__main__':
    usage = ["%prog -s sourceDir -d destDir"]
    parser = optparse.OptionParser("\n" + "\n".join(usage))
    parser.add_option("-s", action = "store", dest = "srcDir", default = SOURCE_DIR, help = "path to dir with all needed files, default is scrooge:%s" % SOURCE_DIR)
    parser.add_option("-d", action = "store", dest = "destDir", default = ".", help = "copy all files here")
    (options, args) = parser.parse_args()

    if not os.path.isdir(options.destDir):
        os.mkdir(options.destDir)

    if CheckDirTree(options.srcDir):
        for folder in REQUIRED_FOLDERS:
            CopyFolderIfNeed(options.srcDir, options.destDir, folder)
        for link in REQUIRED_LINKS:
            CreateSymLinkIfNeed(options.srcDir, options.destDir, link)
