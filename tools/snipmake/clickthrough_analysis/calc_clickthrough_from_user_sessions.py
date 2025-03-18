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

# 1-based number
MAX_POSITION_NUMBER = 6
MAX_SERP_SHOW_NUMBER = 30
       
class ExctractRequestAndClickMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        
        if not field_dictionary.has_key("service") or not field_dictionary.has_key("url") or \
           not field_dictionary["service"] == "www.yandex" or not field_dictionary.has_key("reqid"):
            return
            
        if field_dictionary["type"] == "REQUEST" and not field_dictionary.has_key("testtag"):
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("query", field_dictionary["query"])
            formatter.add_field("url", field_dictionary["url"])
            yield Record(field_dictionary["reqid"], '1_request', formatter.to_string())
        elif field_dictionary["type"] == "CLICK":
            formatter = yx_log_parser.LogFieldStringFormatter()
            formatter.add_field("clicked_url", field_dictionary["url"])
            formatter.add_field("pos", field_dictionary["pos"])
            yield Record(field_dictionary["reqid"], '2_click', formatter.to_string())

class ExctractSnippetInfoMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value, "@@")
        if field_dictionary.has_key("reqid") and field_dictionary.has_key("snippets_type") and \
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
                    clicked_urls_string += "\t" + record.value
                 else:
                    snippet_info += "\t" + record.value
            
        if len(request_value) > 0 and len(snippet_info) > 0:
            yield Record(request_id, '', request_value + clicked_urls_string + snippet_info)

class GroupDataByQueryReduce:
    def __call__(self, request_id, records):
        for record in records:
            field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)    
            # empty serp
            if len(field_dictionary["url"]) > 0:
                formatter = yx_log_parser.LogFieldStringFormatter()
                formatter.add_field("url", field_dictionary["url"])
                if field_dictionary.has_key("clicked_url"):
                    formatter.add_field("clicked_url", field_dictionary["clicked_url"])
                    formatter.add_field("pos", field_dictionary["pos"])
                    
                formatter.add_field("snippets_type", field_dictionary["snippets_type"])
                formatter.add_field("snippets_count", field_dictionary["snippets_count"])
                formatter.add_field("snippets_headline", field_dictionary["snippets_headline"])
                formatter.add_field("snippets_crc", field_dictionary["snippets_crc"])
                if field_dictionary.has_key("extralink_pos"):
                    formatter.add_field("extralink_pos", field_dictionary["extralink_pos"])
                
                yield Record(field_dictionary["query"], hashlib.sha224("@@".join(field_dictionary["snippets_crc"])).hexdigest(), \
                             formatter.to_string())

class CalculatePositionsClickthroughPerQueryAndSerpReduce:
    def __call__(self, query, records):
        serp_shows = {}
        serp_list = {}
        serp_average_pos_clickthrough = {}
        serp_click_count = {}
        
        serp_snippets_type = {}
        serp_extralink_positions = {}    
        
        for record in records:
            field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
            
            current_serp_hash = record.subkey
        
            if not serp_shows.has_key(current_serp_hash):
                serp_shows[current_serp_hash] = 0
                serp_click_count[current_serp_hash] = 0
                serp_list[current_serp_hash] = field_dictionary["url"]
                serp_snippets_type[current_serp_hash] = field_dictionary["snippets_type"]
                if field_dictionary.has_key("extralink_pos"):
                    serp_extralink_positions[current_serp_hash] = field_dictionary["extralink_pos"]
                    
                position_click_cout = range(0, MAX_POSITION_NUMBER)
                serp_average_pos_clickthrough[current_serp_hash] = map(lambda x: 0, position_click_cout)
                
            serp_shows[current_serp_hash] += 1
            
            if field_dictionary.has_key("pos"):
                if not type(field_dictionary["pos"]).__name__ == "list":
                    field_dictionary["pos"] = [field_dictionary["pos"]]
            
                for click_position in field_dictionary["pos"]:
                    click_position = int(click_position)
                    if click_position < MAX_POSITION_NUMBER:
                        serp_average_pos_clickthrough[current_serp_hash][click_position] += 1
                        serp_click_count[current_serp_hash] += 1
        
        for serp_hash, click_count_list in serp_average_pos_clickthrough.iteritems():
            formatter = yx_log_parser.LogFieldStringFormatter()
            
            pos = 0
            for click_count in click_count_list:
                if serp_click_count[serp_hash] > 0:
                    average_position_clickthrough = float(click_count) / float(serp_shows[serp_hash])
                else:
                    average_position_clickthrough = 0
                    
                formatter.add_field("pos%(0)s_click_count" % {"0": pos}, average_position_clickthrough)
                pos += 1
                
            formatter.add_field("url", serp_list[serp_hash])
            formatter.add_field("show_number", serp_shows[serp_hash])
            formatter.add_field("snippets_type", serp_snippets_type[serp_hash])
            
            if serp_extralink_positions.has_key(serp_hash):
                formatter.add_field("extralink_pos", serp_extralink_positions[serp_hash])
            yield Record(query, serp_hash, formatter.to_string())

    
class FindDocumentPairPerQueryAndSerpMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        if int(field_dictionary["show_number"]) < MAX_SERP_SHOW_NUMBER or field_dictionary.has_key("extralink_pos"):
            return
                    
        pos_click_counts = []
        
        for pos in range(0, MAX_POSITION_NUMBER):
            pos_click_counts.append(float(field_dictionary["pos%(0)s_click_count" % {"0": pos}]))
            
        for pos1 in range(0, MAX_POSITION_NUMBER - 1):
            for pos2 in range(pos1 + 1, MAX_POSITION_NUMBER):
                if pos_click_counts[pos1] < pos_click_counts[pos2] and \
                   field_dictionary["snippets_type"][pos1] == "generic" and field_dictionary["snippets_type"][pos2] == "generic":
                   formatter = yx_log_parser.LogFieldStringFormatter()
                   formatter.add_field("preference_pos", repr(pos2))
                   formatter.add_field("preference_pos", repr(pos1))
                   yield Record(record.key, record.subkey, formatter.to_string() + "\t" + record.value)    

class FindUniqueDocumentUrlsMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        
        for position in field_dictionary["preference_pos"]:
            yield Record(field_dictionary["url"][int(position)], '', record.key)
                   
class FindUniqueDocumentUrlsReduce:
    def __call__(self, key, records):
        query = ""
        for record in records:
            query = record.value
            break;

        yield Record(key, '', query)
		
class FindQueryAndDocumentUrlsMap:
    def __call__(self, record):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        
        for position in field_dictionary["preference_pos"]:
            yield Record(record.key, '', field_dictionary["url"][int(position)])
                   
def main():
    MapReduce.useDefaults(server = 'betula00:8013', verbose = True)
    
	# ** Find document preference pairs from user_sessions and reqans logs ** 
	
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20100517", dstTables = ["nordic_user_sessions_request"])
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20100518", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20100519", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20100520", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20100521", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20100522", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractRequestAndClickMap(), srcTable = "user_sessions/20100523", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20100517", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20100518", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20100519", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20100520", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20100521", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20100522", dstTables = ["nordic_user_sessions_request"], appendMode = True)
    MapReduce.runMap(ExctractSnippetInfoMap(), srcTable = "reqans_log/20100523", dstTables = ["nordic_user_sessions_request"], appendMode = True)

    MapReduce.runReduce(MergeSessionAndSnippetDataReduce(), srcTable = "nordic_user_sessions_request", dstTables = ["nordic_user_sessions_request_result"])
    MapReduce.runReduce(GroupDataByQueryReduce(), srcTable = "nordic_user_sessions_request_result", dstTables = ["nordic_user_sessions_per_query"])
    MapReduce.runReduce(CalculatePositionsClickthroughPerQueryAndSerpReduce(), srcTable = "nordic_user_sessions_per_query", dstTables = ["nordic_per_query_serp_result"])
    MapReduce.runMap(FindDocumentPairPerQueryAndSerpMap(), srcTable = "nordic_per_query_serp_result", dstTables = ["nordic_per_query_doc_pairs"])
	
	# ** Get document urls ** 
            
    MapReduce.runMap(FindUniqueDocumentUrlsMap(), srcTable = "nordic_per_query_doc_pairs", dstTables = ["nordic_document_urls_temp"])
    MapReduce.runReduce(FindUniqueDocumentUrlsReduce(), srcTable = "nordic_document_urls_temp", dstTables = ["nordic_document_urls"])
    MapReduce.dropTables(dstTables=["nordic_document_urls_temp"])
	
	# ** Get Query+Url table **
    MapReduce.runMap(FindQueryAndDocumentUrlsMap(), srcTable = "nordic_per_query_doc_pairs", dstTables = ["nordic_query_document_urls"])
		
    return 0

if __name__ == '__main__':
    sys.exit(main())
    