import os

import sys

import getopt
import subprocess

from antirobot.scripts.utils.join_files import JoinFiles

from antirobot.scripts.learn.make_learn_data import data_types
import convert_factors_to_one_version as conv
from antirobot.scripts.learn.make_learn_data.tweak import rnd_request


FACTORS_DICT = None

def MergeFunc(line1, line2):
    reqDataRaw = data_types.RndReqDataRaw.FromString(line1.rstrip())

    captchaData = data_types.FinalCaptchaData.FromString(line2.rstrip())

    GetFactor = lambda factorName: captchaData.Factors.factors[FACTORS_DICT[factorName]]

    result = data_types.RndReqData()
    result.Raw = reqDataRaw.Copy()
    result.TweakFlags.numDocs = GetFactor('num_docs')
    result.TweakFlags.haveSyntax = GetFactor('have_syntax')
    result.TweakFlags.haveRestr = GetFactor('have_restr')
    result.TweakFlags.quotes = GetFactor('quotes')
    result.TweakFlags.cgiUrlRestr = int(GetFactor('cgi_serverurl') or
                                        GetFactor('cgi_surl') or
                                        GetFactor('cgi_site'))

    result.NumRedir = captchaData.NumRedir.Copy()
    result.Flags = captchaData.Flags.Copy()

    return result.ToString()


def RndReqDataRawKey(line):
    data = data_types.RndReqDataRaw.FromString(line)
    return data.reqid


def RndCaptchasOneVersionKey(line):
    data = data_types.FinalCaptchaData.FromString(line)
    return data.Ident.reqid


def Execute(goodFactorsFile, rndReqDataRaw, rndCaptchasOneVersion, rndReqData):
    with open(goodFactorsFile) as f:
        fnames = conv.load_fnames_from_file(f);

    global FACTORS_DICT
    FACTORS_DICT = conv.make_factors_index(fnames);

    JoinFiles(rndReqDataRaw, rndCaptchasOneVersion, RndReqDataRawKey, RndCaptchasOneVersionKey, rndReqData, mergeFunc=MergeFunc)
