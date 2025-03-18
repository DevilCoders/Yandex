#!/usr/local/bin/pythonf
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

# 1-based number
MAX_POSITION_NUMBER = 6
MIN_SERP_SHOW_NUMBER = 20
       
class ExctractRequestAndClickMap:
    def __call__(self, record):
        fields = yx_log_parser.get_field_value_dictionary(record.value)
        
        if not fields.has_key("service") or not fields.has_key("url") or \
           not fields["service"] == "www.yandex" or not fields.has_key("reqid"):
            return
            
        if fields["type"] == "REQUEST" and not fields.has_key("testtag"):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("query", fields["query"])
            formatter.add_field("url", fields["url"])
            yield Record(fields["reqid"], '1_request', formatter.to_string())
        elif fields["type"] == "CLICK" and fields.has_key("pos"):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("clicked_url", fields["url"])
            formatter.add_field("pos", fields["pos"])
            yield Record(fields["reqid"], '2_click', formatter.to_string())

class ExctractSnippetInfoMap:
    def __call__(self, record):
        fields = yx_log_parser.get_field_value_dictionary(record.value, "@@")
        if fields.has_key("reqid") and fields.has_key("snippets_type") and \
           fields.has_key("snippets_count") and fields.has_key("snippets_crc") and \
           fields.has_key("snippets_headline"):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("extralink_pos", self.get_extralinks_positions(record.value))
            formatter.add_field("snippets_type", fields["snippets_type"])
            formatter.add_field("snippets_count", fields["snippets_count"])
            formatter.add_field("snippets_headline", fields["snippets_headline"])
            formatter.add_field("snippets_crc", fields["snippets_crc"])
            if not isinstance(fields["reqid"], list):
                yield Record(fields["reqid"], '3_reqans', formatter.to_string())
            
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
            yield Record(fields["query"], '', request_value + clicked_urls_string + snippet_info)        
            
class GroupByQueryAndCalcPositionInpdependedCTRReduce:
    def __call__(self, query, records):
        serp_urls = {}
        serp_snippet_src = {}
        serp_query = {}
        serp_clickthrough = {}
        serp_testtag_snippet_crc = {}
        
        for record in records:
            fields = yx_log_parser.get_field_value_dictionary(record.value)
            if ("extralink_pos" in fields) and fields["extralink_pos"].count("0") != 0:
                continue
                
            if ("wiz-name" in fields) or fields["snippets_type"].count("generic") != len(fields["snippets_type"]):
                continue
                
            serp_hash = hashlib.sha224("@@".join(fields["snippets_crc"])).hexdigest()
            if not serp_hash in serp_clickthrough:
                serp_urls[serp_hash] = fields["url"]
                serp_query[serp_hash] = fields["query"]
                serp_clickthrough[serp_hash] = []
            
            if "pos" in fields:
                pos_click_list = map(lambda x: 0, range(MAX_POSITION_NUMBER))
                for pos in fields["pos"]:
                    #pos = int(fields["page"])*int(fields["page-size"]) + int(pos)
                    pos = int(pos)
                    if pos < MAX_POSITION_NUMBER:
                        pos_click_list[pos] = 1
                serp_clickthrough[serp_hash].append(pos_click_list)
                              
        for serp_hash in serp_clickthrough:
            position_ip_ctr_list, max_ip_shows = self.CalculatePositionInpdependedCTR(serp_clickthrough[serp_hash])
            position_ctr_list, max_shows = self.CalculateCTR(serp_clickthrough[serp_hash])
            
            for index in range(len(position_ip_ctr_list)):
                position_ip_ctr_list[index] = repr(position_ip_ctr_list[index])
                position_ctr_list[index] = repr(position_ctr_list[index])
            
            if max_shows > MIN_SERP_SHOW_NUMBER:
                formatter = yx_log_parser.LogFieldStringFormatter()
                formatter.add_list_field("url", serp_urls[serp_hash])
                formatter.add_list_field("ip_ctr", position_ip_ctr_list)
                formatter.add_list_field("ctr", position_ctr_list)
                yield Record(serp_query[serp_hash], serp_hash, formatter.to_string())
                
    def CalculatePositionInpdependedCTR(self, serp_pos_clicks_list):
        pos_click_counts = map(lambda x: 0, range(MAX_POSITION_NUMBER))
        pos_shows = map(lambda x: 0, range(MAX_POSITION_NUMBER))
        
        for serp_pos_clicks in serp_pos_clicks_list:
            max_position_show_threshold = -1
            index = 0
            for is_click in serp_pos_clicks:
                if is_click:
                    max_position_show_threshold = index
                index += 1
                
            index = 0
            for is_click in serp_pos_clicks:
                if is_click:
                    pos_click_counts[index] += 1
                if index <= max_position_show_threshold:
                    pos_shows[index] += 1
                index += 1
                
        ctr_list = map(lambda x: 0, range(MAX_POSITION_NUMBER))
        
        for index in range(MAX_POSITION_NUMBER):
            if pos_click_counts[index] > 0:
                ctr_list[index] = float(pos_click_counts[index]) / pos_shows[index]
            
        return ctr_list, max(pos_shows)
        
    def CalculateCTR(self, serp_pos_clicks_list):
        pos_click_counts = map(lambda x: 0, range(MAX_POSITION_NUMBER))
        
        for serp_pos_clicks in serp_pos_clicks_list:
            index = 0
            for is_click in serp_pos_clicks:
                if is_click:
                    pos_click_counts[index] += 1
                index += 1
                
        ctr_list = map(lambda x: 0, range(MAX_POSITION_NUMBER))
        
        for index in range(MAX_POSITION_NUMBER):
            if pos_click_counts[index] > 0:
                ctr_list[index] = float(pos_click_counts[index]) / len(serp_pos_clicks_list)
            
        return ctr_list, len(serp_pos_clicks_list)
    
def get_tasks_by_ip_ctr(table_name, output_file_name):
    with open(output_file_name, "w") as file: 
        for record in MapReduce.getSample(srcTable=table_name, count=None):
            fields = yx_log_parser.get_field_value_dictionary(record.value)
            pos_ctr_list = []
            for ctr in fields["ip_ctr"]:
                pos_ctr_list.append(float(ctr))
                
            for pos1 in range(0, 2 - 1):
                for pos2 in range(pos1 + 1, 2):
                    if pos_ctr_list[pos1] < pos_ctr_list[pos2]:
                        file.write("%(0)s\turl1=%(1)s\tctr1=%(2)f\turl2=%(3)s\tctr2=%(4)f\tregion_id=213\n" % {"0": record.key, "1": fields["url"][pos1], "2": pos_ctr_list[pos1], "3": fields["url"][pos2], "4": pos_ctr_list[pos2]} )
            
                               
def main():
    MapReduce.useDefaults(server = 'betula00:8013', verbose = True)
	
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101130", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101131", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101102", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101103", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101104", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101105", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101106", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101107", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101108", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101117", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20101118", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)

    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101130", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101131", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101102", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101103", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101104", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101105", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101106", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101107", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101108", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101117", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20101118", dstTables = ["nordic/single_serp_user_sessions_request"], appendMode = True)
    
    MapReduce.runReduce(MergeSessionAndSnippetDataReduce(), srcTable = "nordic/single_serp_user_sessions_request_s", dstTables = ["nordic/single_serp_user_sessions_request_result"])
    MapReduce.runReduce(GroupByQueryAndCalcPositionInpdependedCTRReduce(), srcTable = "nordic/single_serp_user_sessions_request_result", dstTables = ["nordic/nordic_per_query_serp_ctr_result"])
    
    get_tasks_by_ip_ctr("nordic/nordic_per_query_serp_ctr_result", "single_serp_tasks.txt")
		
    return 0

if __name__ == '__main__':
    sys.exit(main())
    