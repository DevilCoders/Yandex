# -*- encoding: utf-8 -*-
# add num_docs and have_syntax factors to rnd_reqdata

import os
import subprocess

from antirobot.scripts.learn.serp_images import convert_factors_to_one_version as conv
from antirobot.scripts.learn.serp_images.extract_eventlog_data import RecordFieldIndices
from antirobot.scripts.learn.make_learn_data.tweak import rnd_request

def Execute(goodFactorsFile, rndReqDataRaw, rndReqData):
    with open(goodFactorsFile) as f:
        fnames = conv.load_fnames_from_file(f);
    fDict = conv.make_factors_index(fnames);

    NUM_FIELDS_FROM_CAPTCHAS = rnd_request.RNDREQ_LAST_INDEX - rnd_request.RNDREQ_NUMDOCS + 1

    FactorIndex = lambda x: fDict[x] + RecordFieldIndices.FACTORS_START_INDEX
    _POS = lambda x: '$%d' % x

    awkIndices = [None] * NUM_FIELDS_FROM_CAPTCHAS  # just init

    awkIndices[0] = _POS(RecordFieldIndices.REQID_INDEX + 1)
    awkIndices[1] = _POS(FactorIndex('num_docs') + 1)
    awkIndices[2] = _POS(FactorIndex('have_syntax') + 1)
    awkIndices[3] = _POS(FactorIndex('have_restr') + 1)
    awkIndices[4] = _POS(FactorIndex('quotes') + 1)

    awkIndices[5] = '(%s?1:0)' % '+'.join((
        _POS(FactorIndex('cgi_serverurl') + 1),
        _POS(FactorIndex('cgi_surl') + 1),
        _POS(FactorIndex('cgi_site') + 1))
        )

    awkIndices[6] = '0' # redirects
    awkIndices[7] = '0' # removals
    awkIndices[8] = _POS(RecordFieldIndices.XML_SEARCH_INDEX + 1)
    awkIndices[9] = _POS(RecordFieldIndices.IMAGE_DOWNLOADED_INDEX + 1)
    awkIndices[10] = '0'
    awkIndices[11] = '0'
    awkIndices[12] = '0'

    assert(None not in awkIndices)

    awkProc = subprocess.Popen(['awk', '-vOFS=\\t', '{print %s}' % ','.join(awkIndices), rndReqData], stdout = subprocess.PIPE)
    env = os.environ.copy();
    env['LC_ALL'] ='C';
    subprocess.Popen(['join', '-t', '\t', rndReqDataRaw, '-'], env = env, stdin = awkProc.stdout).wait();
