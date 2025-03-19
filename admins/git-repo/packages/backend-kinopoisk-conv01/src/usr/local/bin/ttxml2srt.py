#! /usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, re, math
from zipfile import ZipFile
from datetime import date
import shutil
import subprocess

import xml.etree.ElementTree as ET
import codecs

xml_filename = os.path.realpath(sys.argv[1])
input_file = codecs.open(sys.argv[1], "r", "utf-8");

text = input_file.read()
text = text.replace("<br/>", "\n")

input_file.close()

tmp_file_name = xml_filename + ".tmp"
tmp_file = codecs.open(tmp_file_name, "w", "utf-8")
tmp_file.write(text)
tmp_file.close()

tree = ET.parse(tmp_file_name)
root = tree.getroot()

captions = root.findall('./{http://www.w3.org/2006/10/ttaf1}body/{http://www.w3.org/2006/10/ttaf1}div/{http://www.w3.org/2006/10/ttaf1}p')
srt = ""
i = 0

for node in captions:
    # print node.attrib, node.text #, ET.tostring(node, "utf-8") # ET.dump(node)
    if node.text == None:
        node.text = ""
    i += 1
    begin = node.attrib['begin'].replace('.', ',')
    end = node.attrib['end'].replace('.', ',')
    srt += str(i) + "\n" + begin + ' --> ' + end + "\n" + node.text + "\n\n"

f = codecs.open(sys.argv[2] if len(sys.argv) > 2 else "subs.srt", "w", "utf-8")
f.write(srt)
f.close()

os.remove(tmp_file_name)

print "SRT conversion OK\n"
