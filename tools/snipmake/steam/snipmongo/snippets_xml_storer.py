#!/usr/bin/python
# -*- coding: utf-8 -*-

import xml.parsers.expat;
import pymongo;
import hashlib;
import sys;

from steam import Snippet;

class Parser:

    xml = "";
    expat = "";
    in_title = False;
    in_fragment = False;
    formed_snippet = "";
    storer = "";

    def __init__(self, xml_to_parse):
        coll_name = hashlib.sha256(xml_to_parse).hexdigest();
        try:
            self.storer = Storer(coll_name);
        except:
            raise
        self.xml = xml_to_parse;
        self.expat = xml.parsers.expat.ParserCreate();
        self.expat.StartElementHandler = self.start_element_handler;
        self.expat.EndElementHandler = self.end_element_handler;
        self.expat.CharacterDataHandler = self.cdata_handler;
        self.expat.Parse(self.xml, 0);
        self.storer.flush();
        self.storer.create_index([("query", pymongo.ASCENDING),
                                  ("url", pymongo.ASCENDING)]);
        self.storer.disconnect();

    def start_element_handler(self, name, attrs):
        if name == "qdpair":
            self.formed_snippet = Snippet();
            self.formed_snippet.set_region(attrs["region"]);
            self.formed_snippet.set_b64_qtree(attrs["richtree"]);
            self.formed_snippet.set_url(attrs["url"]);
        elif name == "fragment":
            self.in_fragment = True;
        elif name == "title":
            self.in_title = True;

    def end_element_handler(self, name):
        if name == "qdpair":
            self.storer.store(self.formed_snippet);
        elif name == "fragment":
            self.in_fragment = False;
        elif name == "title":
            self.in_title = False;

    def cdata_handler(self, data):
        if (data.isspace()):
            return;
        if self.in_title:
            self.formed_snippet.set_title_text(data);
        elif self.in_fragment:
            self.formed_snippet.set_fragment(data);
        else:
            self.formed_snippet.set_query(data);

class Storer:

    client = "";
    collection = "";
    buff = [];
    buff_size = 1250;

    def __init__(self, coll_name, size=1250):
        try:
            self.client = pymongo.MongoClient("localhost", 27017);
        except:
            raise Exception("Can't connect to MongoDB");
        db = self.client.steam;
        try:
            self.collection = db.create_collection(coll_name);
        except:
            self.dosconnect();
            raise Exception("This file is already added to MongoDB");
        self.buff = [];
        self.buff_size = size;

    def flush(self):
        self.collection.insert(self.buff);
        self.buff = [];

    def store(self, snippet):
        self.buff.append(snippet.__dict__);
        if len(self.buff) >= self.buff_size:
            self.flush();

    def create_index(self, idx_params):
        self.collection.create_index(idx_params);

    def disconnect(self):
        self.client.disconnect();


def main():
    if len(sys.argv) != 2:
        print("Specify snippets XML file name as the only argument");
        exit();
    try:
        file = open(sys.argv[1]);
    except Exception:
        print("Can't open file!");
        exit();
    try:
        parser = Parser(file.read());
    except Exception as exc:
        print(exc.args[0]);
        file.close();
        exit();
    except:
        print("Unexpected error");
        file.close();
        exit();
    file.close();

if __name__ == "__main__":
    main();

