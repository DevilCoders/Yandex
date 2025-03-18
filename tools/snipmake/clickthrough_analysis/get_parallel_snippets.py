#!/usr/local/bin/python
#vim:fileencoding=utf-8

import copy
import hashlib
import itertools
import operator
import optparse
import math
import random
import re
import sys
import urllib
import urlparse
import uuid
import yx_log_parser
import yx_click_statistics

from mapreducelib import MapReduce, Record

PRODUCTION_TEST_TAG = 35
SHORT_SNIPPET_TEST_TAG = 270

MAX_POSITION_NUMBER = 20
MIN_SERP_SHOW_NUMBER = 20

class ExtractRequestAndClicksMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        
        if not field_dictionary.has_key("service") or not field_dictionary.has_key("url") or \
           not field_dictionary["service"] == "www.yandex" or not field_dictionary.has_key("reqid"):
            return
        
        if field_dictionary["type"] == "REQUEST" and (not "testtag" in field_dictionary \
           or ("testtag" in field_dictionary and int(field_dictionary["testtag"]) == SHORT_SNIPPET_TEST_TAG)):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("query", field_dictionary["query"])
            formatter.add_field("url", field_dictionary["url"])
            formatter.add_field("request_time", record.subkey.strip())
            formatter.add_field("region_id", field_dictionary["user-region"].strip())
            formatter.add_field("page", field_dictionary["page"].strip())
            formatter.add_field("page-size", field_dictionary["page-size"].strip())
            
            if not "testtag" in field_dictionary:
                field_dictionary["testtag"] = repr(PRODUCTION_TEST_TAG)
            
            formatter.add_field("testtag", field_dictionary["testtag"].strip())
            yield Record(field_dictionary["reqid"], '1_request', record.value)
        elif field_dictionary["type"] == "CLICK" and field_dictionary.has_key("pos"):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("clicked_url", field_dictionary["url"])
            formatter.add_field("pos", field_dictionary["pos"])
            formatter.add_field("click_time", record.subkey.strip())
            
            if "wiz-name" in field_dictionary:
                formatter.add_field("wiz-name", field_dictionary["wiz-name"])
            yield Record(field_dictionary["reqid"], '2_click', formatter.to_string())
         
class ExtractSnippetInfoMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value, "@@")
        if field_dictionary.has_key("reqid") and not isinstance(field_dictionary["reqid"], list) and field_dictionary.has_key("snippets_type") and \
           field_dictionary.has_key("snippets_count") and field_dictionary.has_key("snippets_crc") and \
           field_dictionary.has_key("snippets_headline"):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("extralink_pos", self.get_extralinks_positions(record.value))
            formatter.add_field("snippets_type", field_dictionary["snippets_type"])
            formatter.add_field("snippets_count", field_dictionary["snippets_count"])
            formatter.add_field("snippets_headline", field_dictionary["snippets_headline"])
            formatter.add_field("snippets_crc", field_dictionary["snippets_crc"])
            
            yield Record(field_dictionary["reqid"], '3_reqans', formatter.to_string())
            
    def get_extralinks_positions(self, value_string):
        pos = 0
        extralink_positions = []
        for str in value_string.split("\n"):
            field_dic = yx_log_parser.get_field_value_dictionary(str, "@@")
            if field_dic.has_key("url"):
                if field_dic.has_key("extralinks") and extralink_positions.count(pos) == 0:
                    extralink_positions.append(repr(pos))
                pos += 1
        return extralink_positions
        
class MergeSessionAndSnippetDataReduce:
    def __call__(self, request_id, records):
        request_value = ""
        clicked_urls_string = ""
        snippet_info = ""
        
        for record in records:
            if record.subkey == "1_request":
                request_value = record.value
            elif len(request_value) > 0:
                 if record.subkey == "2_click":
                    fields = yx_log_parser.get_field_value_dictionary(record.value)
                    if not "ip" in fields:
                        clicked_urls_string += "\t" + record.value
                 else:
                    snippet_info += "\t" + record.value
            
        if len(request_value) > 0 and len(snippet_info) > 0 and len(clicked_urls_string) > 0:
            fields = yx_log_parser.get_field_value_dictionary(request_value)
            if not "testtag" in fields:
                fields["testtag"] = repr(PRODUCTION_TEST_TAG)

            yield Record(fields["query"], fields["testtag"], request_value + clicked_urls_string + snippet_info)        


class GroupByQueryAndCalcCTRReduce:
    def __call__(self, query, records):
        serp_testtag_clickthrough = {}
        serp_urls = {}
        serp_snippet_src = {}
        serp_query = {}
        serp_shows = {}
        serp_clickthrough = {}
        serp_testtag_snippet_crc = {}
        
        if len(query.split()) > 3:
            return
            
        for record in records:
            fields = yx_log_parser.get_field_value_dictionary(record.value)
            if ("extralink_pos" in fields) and fields["extralink_pos"].count("0") != 0:
                continue
                
            if ("wiz-name" in fields) or fields["snippets_type"].count("generic") != len(fields["snippets_type"]):
                continue
                
            serp_hash = hashlib.sha224("@@".join(fields["url"])).hexdigest()
            if not serp_hash in serp_testtag_clickthrough:
                serp_testtag_clickthrough[serp_hash] = {}
                serp_urls[serp_hash] = fields["url"]
                serp_shows[serp_hash] = {}
                serp_query[serp_hash] = fields["query"]
                serp_testtag_snippet_crc[serp_hash] = {}
                serp_clickthrough[serp_hash] = {}                
            if not record.subkey in serp_testtag_clickthrough[serp_hash]:
                serp_testtag_clickthrough[serp_hash][record.subkey] = map(lambda x: 0, range(0, MAX_POSITION_NUMBER))
                serp_shows[serp_hash][record.subkey] = 0
                serp_clickthrough[serp_hash][record.subkey] = 0
                serp_testtag_snippet_crc[serp_hash][record.subkey] = fields["snippets_crc"]
                
            serp_shows[serp_hash][record.subkey] += 1
            
            if "pos" in fields:
                pos_dict = {}
                for pos in fields["pos"]:
                    pos = int(fields["page"])*int(fields["page-size"]) + int(pos)
                    if pos < MAX_POSITION_NUMBER and (not pos in pos_dict):
                        serp_testtag_clickthrough[serp_hash][record.subkey][pos] += 1
                    pos_dict[pos] = None                
                              
        for serp_hash, testtag_clickthrough_dict in serp_testtag_clickthrough.iteritems():
            if len(testtag_clickthrough_dict.keys()) < 2:
                continue
                
            if min(serp_shows[serp_hash].values()) > MIN_SERP_SHOW_NUMBER:
                experiment_test_tag = repr(SHORT_SNIPPET_TEST_TAG)
                is_clicked = False
                experiment_clickthrough = range(len(testtag_clickthrough_dict[experiment_test_tag]))
                for index in range(len(testtag_clickthrough_dict[experiment_test_tag])):
                    if testtag_clickthrough_dict[experiment_test_tag][index] > 0:
                        is_clicked = True
                    experiment_clickthrough[index] = repr(float(testtag_clickthrough_dict[experiment_test_tag][index]) / float(serp_shows[serp_hash][experiment_test_tag]))
                    
                test_tag = repr(PRODUCTION_TEST_TAG)
                production_clickthrough = range(len(testtag_clickthrough_dict[test_tag]))
                
                for index in range(len(testtag_clickthrough_dict[test_tag])):
                    if testtag_clickthrough_dict[test_tag][index] > 0:
                        is_clicked = True
                    production_clickthrough[index] = repr(float(testtag_clickthrough_dict[test_tag][index]) / float(serp_shows[serp_hash][test_tag]))
            
                if not is_clicked:
                    continue            

                formatter = yx_log_parser.LogFieldStringFormatter()
                formatter.add_list_field("url", serp_urls[serp_hash])
                
                formatter.add_list_field("experiment_ctr", experiment_clickthrough)
                formatter.add_list_field("ctr", production_clickthrough)
                
                formatter.add_field("experiment_serp_shows", repr(serp_shows[serp_hash][test_tag]))
                formatter.add_field("serp_shows", repr(serp_shows[serp_hash][experiment_test_tag]))
                
                formatter.add_field("experiment_snippets_crc", serp_testtag_snippet_crc[serp_hash][experiment_test_tag])
                formatter.add_field("snippets_crc", serp_testtag_snippet_crc[serp_hash][test_tag])
                
                yield Record(serp_query[serp_hash], serp_hash, formatter.to_string())
                    
def calculate_ctr_dif_histogram(serp_ctr_dif_file_name, pos_ctr_dif_same_file_name, pos_ctr_dif_dif_file_name):
    query_records = {}
    query = {}
    serp_ctr_dict = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        query[query_hash] = record.key
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    serp_ctr_dif = []
    max_position = 10
    position_ctr_dif_different_snip = map(lambda x: [], range(0, max_position))
    position_ctr_dif_same_snip = map(lambda x: [], range(0, max_position))
    
    for key, fields_list in query_records.iteritems():
        if len(fields_list) != 2:
            continue
            
        if fields_list[0]["testtag"] == "35":
            serp_ctr1 = float(fields_list[0]["serp_ctr"])
            serp_ctr2 = float(fields_list[1]["serp_ctr"])
            production_serp_index = 0
            test_serp_index = 1
        else:
            serp_ctr1 = float(fields_list[1]["serp_ctr"])
            serp_ctr2 = float(fields_list[2]["serp_ctr"])
            production_serp_index = 1
            test_serp_index = 0
            
        serp_ctr_dif.append(serp_ctr1 - serp_ctr2)
        
        for pos in range(0, max_position):
            ctr1 = float(fields_list[production_serp_index]["ctr"][pos])
            ctr2 = float(fields_list[test_serp_index]["ctr"][pos])
            
            if pos < len(fields_list[production_serp_index]["snippets_crc"]):
                if fields_list[production_serp_index]["snippets_crc"][pos] != fields_list[test_serp_index]["snippets_crc"][pos]:
                    position_ctr_dif_different_snip[pos].append(ctr1 - ctr2)
                else:
                    position_ctr_dif_same_snip[pos].append(ctr1 - ctr2)
    
    with open(serp_ctr_dif_file_name, "w") as file:
        for ctr in serp_ctr_dif:
            file.write("%f " % ctr)

    with open(pos_ctr_dif_same_file_name, "w") as file:
        for pos_ctr_list in position_ctr_dif_same_snip:
            for ctr in pos_ctr_list:
                file.write("%f " % ctr)
            file.write("\n")
            
    with open(pos_ctr_dif_dif_file_name, "w") as file:
        for pos_ctr_list in position_ctr_dif_different_snip:
            for ctr in pos_ctr_list:
                file.write("%f " % ctr)
            file.write("\n")
            
def calculate_clicks_portion_dif_histogram(same_snip_file_name, dif_snip_file_name):
    query_records = {}
    query = {}
    serp_ctr_dict = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped_4q", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        query[query_hash] = record.key
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    different_snip = map(lambda x: [], range(0, max_position))
    same_snip = map(lambda x: [], range(0, max_position))
    
    for key, fields_list in query_records.iteritems():
        if len(fields_list) != 2:
            continue
            
        if fields_list[0]["testtag"] == "35":
            serp_ctr1 = float(fields_list[0]["serp_ctr"])
            serp_ctr2 = float(fields_list[1]["serp_ctr"])
            production_serp_index = 0
            test_serp_index = 1
        else:
            serp_ctr1 = float(fields_list[1]["serp_ctr"])
            serp_ctr2 = float(fields_list[2]["serp_ctr"])
            production_serp_index = 1
            test_serp_index = 0
        
        for pos in range(0, max_position):
            ctr1 = float(fields_list[production_serp_index]["ctr"][pos])
            ctr2 = float(fields_list[test_serp_index]["ctr"][pos])

            value = 0
            if ctr1 == 0 and ctr2 != 0:
                value = ctr2/serp_ctr2
            elif ctr1 != 0 and ctr2 == 0:
                value = ctr1/serp_ctr1
            elif ctr1 != 0 and ctr2 != 0:
                value = ctr1/serp_ctr1 - ctr2/serp_ctr2
            
            if fields_list[production_serp_index]["snippets_crc"][pos] != fields_list[test_serp_index]["snippets_crc"][pos]:
                different_snip[pos].append(value)
            else:
                same_snip[pos].append(value)

    with open(same_snip_file_name, "w") as file:
        for pos_ctr_list in same_snip:
            for ctr in pos_ctr_list:
                file.write("%f " % ctr)
            file.write("\n")
            
    with open(dif_snip_file_name, "w") as file:
        for pos_ctr_list in different_snip:
            for ctr in pos_ctr_list:
                file.write("%f " % ctr)
            file.write("\n")

def get_clicks_portion_histogram(production_file_name, test_file_name):
    query_records = {}
    query = {}
    serp_ctr_dict = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        query[query_hash] = record.key
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    production_snip = map(lambda x: [], range(0, max_position))
    test_snip = map(lambda x: [], range(0, max_position))
    
    for key, fields_list in query_records.iteritems():
        if len(fields_list) != 2:
            continue
            
        if fields_list[0]["testtag"] == "35":
            serp_ctr1 = float(fields_list[0]["serp_ctr"])
            serp_ctr2 = float(fields_list[1]["serp_ctr"])
            production_serp_index = 0
            test_serp_index = 1
        else:
            serp_ctr1 = float(fields_list[1]["serp_ctr"])
            serp_ctr2 = float(fields_list[2]["serp_ctr"])
            production_serp_index = 1
            test_serp_index = 0
        
        for pos in range(0, max_position):
            ctr1 = float(fields_list[production_serp_index]["ctr"][pos])
            ctr2 = float(fields_list[test_serp_index]["ctr"][pos])

            value = 0
            if ctr1 == 0 and ctr2 != 0:
                value = ctr2/serp_ctr2
            elif ctr1 != 0 and ctr2 == 0:
                value = ctr1/serp_ctr1
            elif ctr1 != 0 and ctr2 != 0:
                value = ctr1/serp_ctr1 - ctr2/serp_ctr2
            
            if fields_list[production_serp_index]["snippets_crc"][pos] != fields_list[test_serp_index]["snippets_crc"][pos]:
                production_snip[pos].append(ctr1)
                test_snip[pos].append(ctr2)

    with open(production_file_name, "w") as file:
        for pos_ctr_list in production_snip:
            for ctr in pos_ctr_list:
                file.write("%f " % ctr)
            file.write("\n")
            
    with open(test_file_name, "w") as file:
        for pos_ctr_list in test_snip:
            for ctr in pos_ctr_list:
                file.write("%f " % ctr)
            file.write("\n")
            
def get_query_url_by_ctr_portion_dif_tasks(output_file_name):
    query_records = {}
    query = {}
    serp_ctr_dict = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        query[query_hash] = record.key
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    
    with open(output_file_name, "w") as file: 
        for key, fields_list in query_records.iteritems():
            if len(fields_list) != 2:
                continue
                
            if fields_list[0]["testtag"] == "35":
                serp_ctr1 = float(fields_list[0]["serp_ctr"])
                serp_ctr2 = float(fields_list[1]["serp_ctr"])
                production_serp_index = 0
                test_serp_index = 1
            else:
                serp_ctr1 = float(fields_list[1]["serp_ctr"])
                serp_ctr2 = float(fields_list[2]["serp_ctr"])
                production_serp_index = 1
                test_serp_index = 0
            
            for pos in range(0, max_position):
                ctr1 = float(fields_list[production_serp_index]["ctr"][pos])
                ctr2 = float(fields_list[test_serp_index]["ctr"][pos])

                value1 = 0
                value2 = 0
                if ctr1 != 0:
                    value1 = ctr1/serp_ctr1
                if ctr2 != 0:
                    value2 = ctr2/serp_ctr2
                
                #if fields_list[production_serp_index]["snippets_crc"] != fields_list[test_serp_index]["snippets_crc"] and abs(value1 - value2) > 0.1 and pos == 0:
                qyery = (key.split("\t")[0]).strip()                
                file.write("%(0)s\turl1=%(1)s\tctr1=%(2)f\turl2=%(3)s\tctr2=%(4)f\tregion_id=213\n" % {"0": qyery, "1": fields_list[0]["url"][pos], "2": value1, "3": fields_list[0]["url"][pos], "4": value2 })
            
def get_query_url_by_ctr_dif_tasks(output_file_name):
    query_records = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    with open(output_file_name, "w") as file: 
        for query_hash, fields_list in query_records.iteritems():
            if len(fields_list) != 2:
                continue
                
            if fields_list[0]["testtag"] == "35":
                production_serp_index = 0
                test_serp_index = 1
            else:
                production_serp_index = 1
                test_serp_index = 0
                
            for pos in range(0, max_position):
                ctr1 = float(fields_list[production_serp_index]["ctr"][pos])
                ctr2 = float(fields_list[test_serp_index]["ctr"][pos])
                
                if len(fields_list[production_serp_index]["snippets_crc"]) <= pos:
                    continue
                
                if fields_list[production_serp_index]["snippets_crc"][pos] != fields_list[test_serp_index]["snippets_crc"][pos]:
                    if ctr1 > 0.9 or ctr2 > 0.9:
                        break
                    if (pos == 0 and abs(ctr1 - ctr2) > 0.15) or (pos == 1 and abs(ctr1 - ctr2) > 0.01):
                        query = (query_hash.split("\t")[0]).strip()
                        file.write("%(0)s\turl1=%(1)s\tctr1=%(2)f\turl2=%(3)s\tctr2=%(4)f\tregion_id=213\n" % {"0": query, "1": fields_list[0]["url"][pos], "2": ctr1, "3": fields_list[0]["url"][pos], "4": ctr2 })
                        
SIGNIFICANCE_DIF = [0.15, 0.05, 0.035, 0.025, 0.017, 0.013, 0.012, 0.0094, 0.0085, 0.009]
                        
def get_query_url_by_1(output_file_name):
    query_records = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    production_snip_count = 0
    test_snip_count = 0
    
    with open(output_file_name, "w") as file: 
        for query_hash, fields_list in query_records.iteritems():
            if len(fields_list) != 2:
                continue
                
            if fields_list[0]["testtag"] == "35":
                production_serp_index = 0
                test_serp_index = 1
            else:
                production_serp_index = 1
                test_serp_index = 0
            
            pos = 0
            while pos < max_position and fields_list[production_serp_index]["snippets_crc"][pos] == fields_list[test_serp_index]["snippets_crc"][pos] and \
                  abs(float(fields_list[production_serp_index]["ctr"][pos]) - float(fields_list[test_serp_index]["ctr"][pos])) < SIGNIFICANCE_DIF[pos]:
                pos += 1
           
            if pos < max_position and fields_list[production_serp_index]["snippets_crc"][pos] != fields_list[test_serp_index]["snippets_crc"][pos] and \
               abs(float(fields_list[production_serp_index]["ctr"][pos]) - float(fields_list[test_serp_index]["ctr"][pos])) >= SIGNIFICANCE_DIF[pos] and \
               float(fields_list[production_serp_index]["ctr"][pos]) < 0.9 and float(fields_list[test_serp_index]["ctr"][pos]) < 0.9:
                query = (query_hash.split("\t")[0]).strip()
                file.write("%(0)s\turl1=%(1)s\tctr1=%(2)f\turl2=%(3)s\tctr2=%(4)f\tregion_id=213\n" % {"0": query, "1": fields_list[0]["url"][pos], "2": float(fields_list[production_serp_index]["ctr"][pos]), "3": fields_list[0]["url"][pos], "4": float(fields_list[test_serp_index]["ctr"][pos]) } )
                if float(fields_list[production_serp_index]["ctr"][pos]) > float(fields_list[test_serp_index]["ctr"][pos]):
                    production_snip_count += 1
                else:
                    test_snip_count += 1
                    
    print "production snip count: " + repr(production_snip_count)
    print "test snip count: " + repr(test_snip_count)

def get_query_url_by_2(output_file_name):
    query_records = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    production_snip_count = 0
    test_snip_count = 0
    
    with open(output_file_name, "w") as file: 
        for query_hash, fields_list in query_records.iteritems():
            if len(fields_list) != 2:
                continue
                
            if fields_list[0]["testtag"] == "35":
                production_serp_index = 0
                test_serp_index = 1
            else:
                production_serp_index = 1
                test_serp_index = 0
            
            pos = 0
            while pos < max_position and \
                  abs(float(fields_list[production_serp_index]["ctr"][pos]) - float(fields_list[test_serp_index]["ctr"][pos])) < SIGNIFICANCE_DIF[pos]:
                pos += 1
           
            if pos < max_position and fields_list[production_serp_index]["snippets_crc"][pos] != fields_list[test_serp_index]["snippets_crc"][pos] and \
               abs(float(fields_list[production_serp_index]["ctr"][pos]) - float(fields_list[test_serp_index]["ctr"][pos])) >= SIGNIFICANCE_DIF[pos] and \
               (float(fields_list[production_serp_index]["ctr"][pos]) < 0.8 and float(fields_list[test_serp_index]["ctr"][pos]) < 0.8):
                query = (query_hash.split("\t")[0]).strip()
                file.write("%(0)s\turl1=%(1)s\tctr1=%(2)f\turl2=%(3)s\tctr2=%(4)f\tregion_id=213\n" % {"0": query, "1": fields_list[0]["url"][pos], "2": float(fields_list[production_serp_index]["ctr"][pos]), "3": fields_list[0]["url"][pos], "4": float(fields_list[test_serp_index]["ctr"][pos]) } )
                
                if float(fields_list[production_serp_index]["ctr"][pos]) > float(fields_list[test_serp_index]["ctr"][pos]):
                    production_snip_count += 1
                else:
                    test_snip_count += 1
                
    print "production snip count: " + repr(production_snip_count)
    print "test snip count: " + repr(test_snip_count)
    
def get_query_url_by_3(output_file_name):
    query_records = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    production_snip_count = 0
    test_snip_count = 0
    
    with open(output_file_name, "w") as file: 
        for query_hash, fields_list in query_records.iteritems():
            if len(fields_list) != 2:
                continue
                
            if fields_list[0]["testtag"] == "35":
                production_serp_index = 0
                test_serp_index = 1
            else:
                production_serp_index = 1
                test_serp_index = 0
            
            pos = 0
            while pos < max_position and \
                  fields_list[production_serp_index]["snippets_crc"][pos] == fields_list[test_serp_index]["snippets_crc"][pos]:
                pos += 1
                
            if pos < max_position and abs(float(fields_list[production_serp_index]["ctr"][pos]) - float(fields_list[test_serp_index]["ctr"][pos])) < SIGNIFICANCE_DIF[pos] and \
               (float(fields_list[production_serp_index]["ctr"][pos]) < 0.8 and float(fields_list[test_serp_index]["ctr"][pos]) < 0.8):
                query = (query_hash.split("\t")[0]).strip()
                file.write("%(0)s\turl1=%(1)s\tctr1=%(2)f\turl2=%(3)s\tctr2=%(4)f\tregion_id=213\n" % {"0": query, "1": fields_list[0]["url"][pos], "2": float(fields_list[production_serp_index]["ctr"][pos]), "3": fields_list[0]["url"][pos], "4": float(fields_list[test_serp_index]["ctr"][pos]) } )
                
                if float(fields_list[production_serp_index]["ctr"][pos]) > float(fields_list[test_serp_index]["ctr"][pos]):
                    production_snip_count += 1
                else:
                    test_snip_count += 1
                
    print "production snip count: " + repr(production_snip_count)
    print "test snip count: " + repr(test_snip_count)

def get_query_url_by_4(output_file_name):
    query_records = {}
    for record in MapReduce.getSample(srcTable="nordic/user_sessions_parallel_serp_request_grouped", count=None):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        query_hash = record.key + "\t" +record.subkey
        if not query_hash in query_records:
            query_records[query_hash] = []
        query_records[query_hash].append(fields)
    
    max_position = 10
    production_snip_count = 0
    test_snip_count = 0
    
    with open(output_file_name, "w") as file: 
        for query_hash, fields_list in query_records.iteritems():
            if len(fields_list) != 2:
                continue
                
            if fields_list[0]["testtag"] == "35":
                production_serp_index = 0
                test_serp_index = 1
            else:
                production_serp_index = 1
                test_serp_index = 0
            
            pos = 0
            while pos < max_position and fields_list[production_serp_index]["snippets_crc"][pos] == fields_list[test_serp_index]["snippets_crc"][pos] and \
                  abs(float(fields_list[production_serp_index]["ctr"][pos]) - float(fields_list[test_serp_index]["ctr"][pos])) < SIGNIFICANCE_DIF[pos]:
                pos += 1
           
            if pos < max_position and fields_list[production_serp_index]["snippets_crc"][pos] == fields_list[test_serp_index]["snippets_crc"][pos]:
                while pos < max_position and (fields_list[production_serp_index]["snippets_crc"][pos] == fields_list[test_serp_index]["snippets_crc"][pos] or \
                      abs(float(fields_list[production_serp_index]["ctr"][pos]) - float(fields_list[test_serp_index]["ctr"][pos])) < SIGNIFICANCE_DIF[pos]):
                    pos += 1
                    
                if pos < max_position and fields_list[production_serp_index]["snippets_crc"][pos] != fields_list[test_serp_index]["snippets_crc"][pos]:
                    query = (query_hash.split("\t")[0]).strip()
                    file.write("%(0)s\turl1=%(1)s\tctr1=%(2)f\turl2=%(3)s\tctr2=%(4)f\tregion_id=213\n" % {"0": query, "1": fields_list[0]["url"][pos], "2": float(fields_list[production_serp_index]["ctr"][pos]), "3": fields_list[0]["url"][pos], "4": float(fields_list[test_serp_index]["ctr"][pos]) } )
                    if float(fields_list[production_serp_index]["ctr"][pos]) > float(fields_list[test_serp_index]["ctr"][pos]):
                        production_snip_count += 1
                    else:
                        test_snip_count += 1
                    
    print "production snip count: " + repr(production_snip_count)
    print "test snip count: " + repr(test_snip_count)
    
def main():
    MapReduce.useDefaults(server = 'betula00:8013', verbose = True)
    
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100909", dstTables = ["nordic/user_sessions_parallel_serp_request_production"])
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100910", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100911", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100912", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100913", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100914", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100915", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100916", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100917", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100918", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100919", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractRequestAndClicksMap(), srcTable = "user_sessions/20100920", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)

    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100909", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100910", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100911", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100912", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100913", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100914", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100915", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100916", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100917", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100918", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100919", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    MapReduce.runMap(ExtractSnippetInfoMap(), srcTable = "reqans_log/20100920", dstTables = ["nordic/user_sessions_parallel_serp_request_production"], appendMode = True)
    
    MapReduce.runReduce(MergeSessionAndSnippetDataReduce(), srcTable = "nordic/user_sessions_parallel_serp_request_production_s", dstTables = ["nordic/user_sessions_production_parallel_serp_request_result"])
    MapReduce.runReduce(GroupByQueryAndCalcCTRReduce(), srcTable = "nordic/user_sessions_production_parallel_serp_request_result_s", dstTables = ["nordic/user_sessions_parallel_serp_request_production_grouped"])
    
    #get_query_url_by_ctr_dif_tasks("query_url_for_creator.txt")
    #get_query_url_by_1("query_url_for_creator.txt")
    #get_query_url_by_2("query_url_for_creator.txt")
    #get_query_url_by_3("query_url_for_creator.txt")
    #get_query_url_by_4("query_url_for_creator.txt")
    
    #get_query_url_by_ctr_portion_dif_tasks("query_url_for_creator.txt")   
    #get_clicks_portion_histogram("prodcution.txt", "test.txt")
        
    # *** For statistics **
    #calculate_ctr_dif_histogram("serp.txt", "same_snip.txt", "dif_snip.txt")
    #calculate_ctr_dif_histogram("4q_serp.txt", "4q_same_snip.txt", "4q_dif_snip.txt")
    #calculate_clicks_portion_dif_histogram("4q_ctr_portion_same_snip.txt", "4q_ctr_portion_dif_snip.txt")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
    