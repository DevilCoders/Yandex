# coding=utf-8

import os
import sys
import re
import tempfile


def get_content(filename):
    with open(filename) as f:
        return f.read()


def set_content(filename, content):
    with open(filename, 'w') as f:
        f.write(content)

data_path = os.path.abspath(os.curdir) + "/../../../../arcadia_tests_data/"

def generate_config(temp_dir):
    thesaurus_dir_path = data_path + "/wizard/thesaurus/"
    geosearch_config_path = os.path.join(temp_dir, 'geosearch.cfg')
    wizard_config_path = data_path + "/wizard/conf"

    print "append=", thesaurus_dir_path
    sys.path.append(thesaurus_dir_path)

    sys.path.append(wizard_config_path)
    from config import Config

    # generate geosearch config
    with open(geosearch_config_path, 'w') as conf_file:
        Config('geo', port='8891', out=conf_file, baseDir=wizard_config_path)

    geo_config = get_content(geosearch_config_path)
    geo_config = re.sub(r'RequestPopularityTrieFile .*', lambda x: x.group(0) + "\nUseTestData yes", geo_config)
    set_content(geosearch_config_path, geo_config)

    return geosearch_config_path


temp_dir = tempfile.mkdtemp()
geosearch_config_path = generate_config(temp_dir)

command = [
    "gdb",
    "--args",
    "../printwzrd",
    "--data", data_path,
    "--config", geosearch_config_path,
    "--add-cgi", "wizextra=verifyalltrees=da&rn=Geosearch&geoaddr_geometa=1",
    "--nocache", "--verify-gzt", "--lemma-count",
    "--thread-count", " 8"
]

import subprocess
print "command=", command
PIPE = subprocess.PIPE
p = subprocess.Popen(command)
p.wait()
