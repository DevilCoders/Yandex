#!/usr/local/bin/python
#vim:fileencoding=utf-8

import copy
import hashlib
import itertools
import operator
import optparse
import random
import re
import sys
import urllib
import urlparse
import uuid
import yx_log_parser
import yx_click_statistics

from mapreducelib import MapReduce, Record
       

class ExctractRequestPageAndPageSizeMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        
        if not field_dictionary.has_key("service") or not field_dictionary.has_key("url") or \
           not field_dictionary["service"] == "www.yandex" or not field_dictionary.has_key("reqid"):
            return
            
        if field_dictionary["type"] == "REQUEST" and not field_dictionary.has_key("testtag"):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("query", field_dictionary["query"])
            formatter.add_field("url", field_dictionary["url"])
            formatter.add_field("request_time", record.subkey.strip())
            formatter.add_field("region_id", field_dictionary["user-region"].strip())
            formatter.add_field("page", field_dictionary["page"].strip())
            formatter.add_field("page-size", field_dictionary["page-size"].strip())
            yield Record(field_dictionary["query"], '1_request', formatter.to_string())
            
class MergeSessionAndSnippetDataPageAndPageSizeMapReduce:
    def __call__(self, query, records):
        page_urls = []
        for i in range(100):
            page_urls.append(" ")
            
        for record in records:
            field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
            page = int(field_dictionary["page"])
            page_size = int(field_dictionary["page-size"])
            if (page + 1)*page_size > 100 or len(field_dictionary["url"]) != page_size:
                continue
                
            for url_index in range(page_size):
                page_urls[page*page_size + url_index] = field_dictionary["url"][url_index]
        
        if page_urls.count(" ") == 0:
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("urls", page_urls)
            yield Record(query, '', formatter.to_string())
                          
class Find_1_vs_70_PairMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        formatter = yx_log_parser.LogFieldStringFormatter()        
        formatter.add_field("url1", field_dictionary["urls"][0])
        formatter.add_field("url2", field_dictionary["urls"][69])
        formatter.add_field("region_id", "213")
        yield Record(record.key, '', formatter.to_string())
        
def main():
    MapReduce.useDefaults(server = 'betula00:8013', verbose = True)
   
    MapReduce.runMap(ExctractRequestPageAndPageSizeMap(), srcTable = "user_sessions/20100913", dstTables = ["nordic/user_sessions_request_new"])
    MapReduce.runMap(ExctractRequestPageAndPageSizeMap(), srcTable = "user_sessions/20100914", dstTables = ["nordic/user_sessions_request_new"], appendMode = True)
    MapReduce.runMap(ExctractRequestPageAndPageSizeMap(), srcTable = "user_sessions/20100915", dstTables = ["nordic/user_sessions_request_new"], appendMode = True)
    MapReduce.runMap(ExctractRequestPageAndPageSizeMap(), srcTable = "user_sessions/20100916", dstTables = ["nordic/user_sessions_request_new"], appendMode = True)
    MapReduce.runMap(ExctractRequestPageAndPageSizeMap(), srcTable = "user_sessions/20100917", dstTables = ["nordic/user_sessions_request_new"], appendMode = True)
    MapReduce.runMap(ExctractRequestPageAndPageSizeMap(), srcTable = "user_sessions/20100918", dstTables = ["nordic/user_sessions_request_new"], appendMode = True)
    MapReduce.runMap(ExctractRequestPageAndPageSizeMap(), srcTable = "user_sessions/20100919", dstTables = ["nordic/user_sessions_request_new"], appendMode = True)
    
    MapReduce.runReduce(MergeSessionAndSnippetDataPageAndPageSizeMapReduce(), srcTable = "nordic/user_sessions_request_new", dstTables = ["nordic/user_sessions_request_result_new"])
    MapReduce.runMap(Find_1_vs_70_PairMap(), srcTable = "nordic/user_sessions_request_result_new", dstTables = ["nordic/1_vs_70_pairs"])
    
	
    return 0

if __name__ == '__main__':
    sys.exit(main())
    