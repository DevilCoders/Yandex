#!/skynet/python/bin/python

import os
import sys

import time
import json

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import config
import gencfg
from core.argparse.parser import ArgumentParserExt

import subprocess
import logging
import shutil

import filecmp

def get_parser():
    parser = ArgumentParserExt("""Build config diff (tag vs local)""")
    parser.add_argument('-t', '--old-tag', dest='old_tag', type=str, help='tag or trunk')
    parser.add_argument('-i', '--template', dest='template', default='db/configs/intmetasearchv2/web/mmeta.yaml',
        type=str, help='template for custom_generators/intmetasearchv2/generate_configs.py')
    parser.add_argument('-f', '--full-diff', action='store_true',
        default=False, help='Optional. Don\'t try to cut some long strings.')
    parser.add_argument('-v', '--verbose', action='store_true',
        default=False, help='Optional. Explain what is being done.')
    return parser

def diff(old_dir, new_dir, full_diff_flag):
    IGNORE_LIST = ['DNSCache', 'CgiSearchPrefix']
    MAX_IGNORE_LEN = 100

    cmd = 'diff -r ' + old_dir + ' ' + new_dir

    logging.info('Do:%s', cmd)
    process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
    output, error = process.communicate()
    if error:
        print "Error during diff:" + error

    print "Diff:\n"
    for line in output.split('\n'):
        ignored = False
        for pattern in IGNORE_LIST:
            if line.find(pattern) != -1 and len(line) > MAX_IGNORE_LEN and not full_diff_flag:
                print line[0:MAX_IGNORE_LEN] + '..., line length = ' + str(len(line))
                ignored = True
                break
        if not ignored:
            print line

def get_tag(tag):
    import utils.standalone.get_gencfg_repo
    dest = './config_diffbuilder_tmp_' + tag
    repo_type = utils.standalone.get_gencfg_repo.ERepoType.FULL
    utils.standalone.get_gencfg_repo.clone(dest, repo_type, tag=tag)
    return dest

def generate_config(working_dir, template):
    config_dir = 'generated_config'
    full_config_dir = os.path.join(working_dir, config_dir)
    if not os.path.exists(full_config_dir):
        os.makedirs(full_config_dir)

    cmd = './custom_generators/intmetasearchv2/generate_configs.py -a genconfigs -t %s -o %s' % (template, config_dir)
    logging.info('generate config at: %s', full_config_dir)
    process = subprocess.Popen(cmd.split(), cwd=working_dir, stdout=subprocess.PIPE)
    logging.info(process.communicate()[0])
    return full_config_dir

def main(options):
    if options.verbose:
        logging.basicConfig(level=logging.INFO)
    else:
        logging.basicConfig(level=logging.ERROR)

    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
    tag_dir = get_tag(options.old_tag)
    tag_config_dir = generate_config(tag_dir, options.template)
    local_config_dir = generate_config(root_dir, options.template)
    diff(tag_config_dir, local_config_dir, options.full_diff)
    shutil.rmtree(tag_dir)
    shutil.rmtree(local_config_dir)

if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
