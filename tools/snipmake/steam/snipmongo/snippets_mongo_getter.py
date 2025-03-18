#!/usr/bin/python
# -*- coding: utf-8 -*-

import pymongo;

from steam import Snippet;

class Getter:

    client = "";
    collection = "";
    cursor = "";
    current = 0;
    l_limit = 0;
    r_limit = 0;

    def __init__(self, coll, start_number, end_number):
        self.current = 0;
        if start_number > end_number:
            self.l_limit = end_number;
            end_number = start_number;
        else:
            self.l_limit = start_number;
        self.r_limit = end_number;
        try:
            self.client = pymongo.MongoClient("localhost", 27017);
        except:
            raise Exception("Can't connect to MongoDB");
        db = self.client.steam;
        if coll != "system.indexes" and coll in db.collection_names():
            self.collection = db[coll];
        else:
            raise Exception("Wrong collection name!");
        if self.r_limit > self.collection.count():
            self.r_limit = self.collection.count();
        self.cursor = self.collection.find();
        self.cursor = self.cursor.sort([("query", pymongo.ASCENDING),
                                        ("url", pymongo.ASCENDING)]);

    def get(self):
        for snippet_dict in self.cursor:
            self.current += 1;
            if self.l_limit <= self.current - 1 < self.r_limit:
                yield Snippet(snippet_dict);


def main():
    file = open("snippets.txt", "w");
    name = "36b598b33cfdb003824dd89750934bccacfe6a833179bca4cf27a2da741a6fc0";
    try:
        getter = Getter(name, 0, 160000);
    except Exception as exc:
        print(exc.args[0]);
        file.close();
        exit();
    except:
        print("Unexpected error");
        file.close();
        exit();
    for snippet in getter.get():
        file.write(repr(snippet.__dict__) + "\n");
    file.close();


if __name__ == "__main__":
    main();

