#!/usr/bin/python
# -*- coding: utf-8 -*-

class Snippet:

    query = "";
    b64_qtree = "";
    url = "";
    region = "";
    title_text = "";
    snip_text = [];

    def __init__(self, dictionary = {"query": "",
                                     "b64_qtree": "",
                                     "url": "",
                                     "region": "",
                                     "title_text": "",
                                     "snip_text": []}):
        self.__dict__ = dictionary;

    def set_query(self, value):
        self.query = value;

    def set_b64_qtree(self, value):
        self.b64_qtree = value;

    def set_url(self, value):
        self.url = value;

    def set_region(self, value):
        self.region = value;

    def set_title_text(self, value):
        self.title_text = value;

    def set_fragment(self, value):
        self.snip_text.append(value);

