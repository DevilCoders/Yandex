#!/usr/bin/env python

import sys
import os.path
import argparse
import urllib2
import subprocess
import tempfile

FML_SERVICE = "http://fml.yandex-team.ru"
FORMULA_URL_TEMPLATE = FML_SERVICE + "/download/computed/formula?file=matrixnet.info&id=%s"
FACTORS_MAPPING_URL_TEMPLATE = FML_SERVICE + "/api/get/%s-%s/factor_names.tsv"

FACTOR_NAMES = "factor_names"
MODEL_PROPS_SECTION = "Model props"

def OptParser():
    parser = argparse.ArgumentParser(
        description="Fetch MatrixNet-formula from http://fml.yandex-team.ru/ and write names of factors to formula property: %s" % FACTOR_NAMES,
    )
    parser.add_argument("fmlFormulaId",
        help="Formula ID from fml. For example, for formula http://fml.yandex-team.ru/formula/24160 id is 24160")
    parser.add_argument("formulaDstFile",
        help="Destination for saving")
    parser.add_argument("--mx_ops",
        default="mx_ops",
        help="Path to mx_ops binary, instead binary from $PATH")
    parser.add_argument("--forced_pool_author",
        default="",
        help="Reset pool author, expected if pool author != formula author")
    return parser

def FetchToFile(fromUrl, toFile):
    fmlResponse = urllib2.urlopen(fromUrl)
    if os.path.exists(toFile):
        raise RuntimeError("Destination file (%s) already exists. Exit." % toFile)
    with open(toFile, "w+b") as f:
        f.write(fmlResponse.read())

def ParseFmlProps(fmlInfo):
    result = {}
    for fmlInfoRaw in fmlInfo.split('\n'):
        cols = fmlInfoRaw.split('\t')
        if len(cols) >= 1 and len(cols[0]):
            section = cols[0].rstrip(':')
        if section == MODEL_PROPS_SECTION and len(cols) >= 3:
            result[cols[1].rstrip(':')] = cols[2]
    return result

def FetchFormulaToFile(formulaId, fileDst):
    try:
        FetchToFile(FORMULA_URL_TEMPLATE % formulaId, fileDst)
    except urllib2.HTTPError as e:
        raise RuntimeError("Error: %s\nCouldn't get formula from fml. Exit." % str(e)) # from e

def GetFormulaInfo(mxOpsPath, formulaFile):
    try:
        return subprocess.check_output([mxOpsPath, "info", formulaFile],
            stderr=subprocess.STDOUT)
    except OSError as e:
        raise RuntimeError("Error: %s\nCouldn't find mx_ops? Exit." % str(e)) # from e

def MakeFactorNamesUrl(fmlInfo, forcedPoolAuthor):
    try:
        fmlProps = ParseFmlProps(fmlInfo)
        fmlAuthor = fmlProps["author"]
        poolAuthor = fmlAuthor if len(forcedPoolAuthor) == 0 else forcedPoolAuthor
        print("INFO formula author:\t%s" % fmlProps["author"])
        print("INFO pool author:\t%s" % fmlProps["author"])
        return FACTORS_MAPPING_URL_TEMPLATE % (poolAuthor, fmlProps["pool-id"])
    except KeyError as e:
        raise RuntimeError("Error: %s\nCouldn't find formula's property. Exit." % str(e)) # from e

def FetchFactorNames(factorNamesUrl):
    try:
        return urllib2.urlopen(factorNamesUrl)
    except urllib2.HTTPError as e:
        raise RuntimeError("Error: %s\nCouldn't get factors mapping. Exit." % str(e)) # from e

def MakeFactorNamesList(factorsMappingFileObject, factorNamesUrl):
    factorsNamesList = []
    for factorRaw in factorsMappingFileObject.read().split('\n'):
        if len(factorRaw):
            (factorIdx, factorName) = factorRaw.split('\t')
            factorIdx = int(factorIdx)
            if len(factorsNamesList) != factorIdx:
                raise RuntimeError("File %s must contains continiuous indexes (%d != %d). Exit." % (factorNamesUrl, len(factorsNamesList), factorIdx))
            factorsNamesList.append(factorName)

    if len(factorsNamesList) == 0:
        raise RuntimeError("Empty factor names %s. Exit." % factorNamesUrl)

    return factorsNamesList

def WriteFactosNamesToFormula(mxOpsPath, formulaFile, factorNamesList):
    try:
        subprocess.check_call([mxOpsPath, "setprop", formulaFile, FACTOR_NAMES, '\t'.join(factorNamesList)],
            stderr=subprocess.STDOUT)
    except OSError as e:
        raise RuntimeError("Error: %s\nCouldn't find mx_ops? Exit." % str(e)) # from e

def main():
    args = OptParser().parse_args()

    try:
        FetchFormulaToFile(args.fmlFormulaId, args.formulaDstFile)
        formulaInfo=        GetFormulaInfo(args.mx_ops, args.formulaDstFile)
        factorNamesUrl =    MakeFactorNamesUrl(formulaInfo, args.forced_pool_author)
        factorsMappingObj = FetchFactorNames(factorNamesUrl)
        factorsNamesList =  MakeFactorNamesList(factorsMappingObj, factorNamesUrl)
        WriteFactosNamesToFormula(args.mx_ops, args.formulaDstFile, factorsNamesList)
    except RuntimeError as e:
        sys.exit(str(e))

    print("Done.")

if __name__ == "__main__":
    main()
