import itertools
import operator
import optparse
import random
import re
import copy
import sys
import urllib
import urlparse
import hashlib
import math
import yx_log_parser
import uuid

from mapreducelib import MapReduce, Record

## ****** Click time ******

def _get_next_clicktime_histogram_map_(record):
    field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
    if not field_dictionary.has_key("clicked_url") or field_dictionary.has_key("extralink_pos"):
        return
        
    if not isinstance(field_dictionary["clicked_url"], list):
        return
        
    values = []        
    click_time = field_dictionary["click_time"]
    click_time.sort()
    for id in range(len(click_time) - 1):
        time1 = int(click_time[id])
        time2 = int(click_time[id + 1])
        values.append(abs(time1-time2))
        
    yield Record("1", '', repr(values) + "\t" + repr(click_time))

def _get_next_clicktime_histogram_reduce_(key, records):
    all_values = []
    for record in records:
        all_values.extend(eval(record.value))
    yield Record("1", '', repr(all_values))
       
def get_next_clicktime_histogram(source_table_name):
    map_table_name = source_table_name + uuid.uuid1().hex
    reduce_table_name = source_table_name + uuid.uuid1().hex    
    MapReduce.runMap(_get_next_clicktime_histogram_map_, srcTable = source_table_name, dstTables = [map_table_name])
    #MapReduce.runReduce(_get_next_clicktime_histogram_reduce_, srcTable = map_table_name, dstTables = [reduce_table_name])    
    #MapReduce.dropTables(dstTables=[map_table_name])

## ****** First Click time (by downloading source table) ******

def get_first_clicktime_histogram(source_table_file_name, output_file_name):
    with open(source_table_file_name, "r") as file:
        for line in file:
            fields = yx_log_parser.get_field_value_dictionary(line)
            #fields
            
        
    
    
## ****** Calc document pair clickthrough histogram ******

def get_ctr_weight(ctr1, ctr2, pos1, pos2):
    #return (ctr1-ctr2)*(1+math.log(pos1-pos2))
    return ctr1-ctr2

def get_document_pair_clickthrough_histogram(source_table_name):
    values = []
    for record in MapReduce.getSample(srcTable = source_table_name):
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        pos1 = field_dictionary["preference_pos"][0]
        pos2 = field_dictionary["preference_pos"][1]
        
        ctr1 = field_dictionary["pos%(0)s_click_count" % {"0": pos1}]
        ctr2 = field_dictionary["pos%(0)s_click_count" % {"0": pos2}]
        
        # values.append(get_ctr_weight(float(ctr1), float(ctr2), int(pos1), int(pos2)))
        values.append(ctr1)
        values.append(ctr2)
    return values
    
## ****** Get clickthrough per position ******

def get_clickthrough_per_position_list(source_table_name):
    clickthrough_per_position_list = {}
    
    map_table_name = source_table_name + repr(uuid.uuid1().hex)
    reduce_table_name = source_table_name + repr(uuid.uuid1().hex)
    
    MapReduce.runMap(_get_preference_pair_percent_list_map_, srcTable = source_table_name, dstTables = [map_table_name])
    MapReduce.runReduce(_get_clickthrough_per_position_list_reduce_, srcTable = map_table_name, dstTables = [reduce_table_name])
    
    for record in MapReduce.getSample(srcTable = reduce_table_name):
        clickthrough_per_position_list = eval(record.value)
        
    MapReduce.dropTables(dstTables=[map_table_name, reduce_table_name])
    
    return clickthrough_per_position_list
    
def _get_clickthrough_per_position_list_reduce_(key, records):
    clickthrough_per_position_list = {}
    clickthrough_per_position_list["pos0_click_count"] = 0
    clickthrough_per_position_list["pos1_click_count"] = 0
    clickthrough_per_position_list["pos2_click_count"] = 0
    clickthrough_per_position_list["pos3_click_count"] = 0
    clickthrough_per_position_list["pos4_click_count"] = 0
    clickthrough_per_position_list["pos5_click_count"] = 0
    clickthrough_per_position_list["pos6_click_count"] = 0
    clickthrough_per_position_list["pos7_click_count"] = 0
    clickthrough_per_position_list["pos8_click_count"] = 0
    clickthrough_per_position_list["pos9_click_count"] = 0
    
    count = 0
    for record in records:
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        clickthrough_per_position_list["pos0_click_count"] += float(field_dictionary["pos0_click_count"])
        clickthrough_per_position_list["pos1_click_count"] += float(field_dictionary["pos1_click_count"])
        clickthrough_per_position_list["pos2_click_count"] += float(field_dictionary["pos2_click_count"])
        clickthrough_per_position_list["pos3_click_count"] += float(field_dictionary["pos3_click_count"])
        clickthrough_per_position_list["pos4_click_count"] += float(field_dictionary["pos4_click_count"])
        clickthrough_per_position_list["pos5_click_count"] += float(field_dictionary["pos5_click_count"])
        clickthrough_per_position_list["pos6_click_count"] += float(field_dictionary["pos6_click_count"])
        clickthrough_per_position_list["pos7_click_count"] += float(field_dictionary["pos7_click_count"])
        clickthrough_per_position_list["pos8_click_count"] += float(field_dictionary["pos8_click_count"])
        clickthrough_per_position_list["pos9_click_count"] += float(field_dictionary["pos9_click_count"])
        count += 1
        
    clickthrough_per_position_list["pos0_click_count"] = clickthrough_per_position_list["pos0_click_count"] / float(count)
    clickthrough_per_position_list["pos1_click_count"] = clickthrough_per_position_list["pos1_click_count"] / float(count)
    clickthrough_per_position_list["pos2_click_count"] = clickthrough_per_position_list["pos2_click_count"] / float(count)
    clickthrough_per_position_list["pos3_click_count"] = clickthrough_per_position_list["pos3_click_count"] / float(count)
    clickthrough_per_position_list["pos4_click_count"] = clickthrough_per_position_list["pos4_click_count"] / float(count)
    clickthrough_per_position_list["pos5_click_count"] = clickthrough_per_position_list["pos5_click_count"] / float(count)
    clickthrough_per_position_list["pos6_click_count"] = clickthrough_per_position_list["pos6_click_count"] / float(count)
    clickthrough_per_position_list["pos7_click_count"] = clickthrough_per_position_list["pos7_click_count"] / float(count)
    clickthrough_per_position_list["pos8_click_count"] = clickthrough_per_position_list["pos8_click_count"] / float(count)
    clickthrough_per_position_list["pos9_click_count"] = clickthrough_per_position_list["pos9_click_count"] / float(count)
                    
    yield Record(key, '', repr(clickthrough_per_position_list))
    
## ****** Get pair preference statistic ******
def get_preference_pair_percent_list(source_table_name):
    change_preference_pair_list = {}
    
    map_table_name = source_table_name + repr(uuid.uuid1().hex)
    reduce_table_name = source_table_name + repr(uuid.uuid1().hex)
    
    MapReduce.runMap(_get_preference_pair_percent_list_map_, srcTable = source_table_name, dstTables = [map_table_name])
    MapReduce.runReduce(_get_preference_pair_percent_list_reduce_, srcTable = map_table_name, dstTables = [reduce_table_name])
    
    for record in MapReduce.getSample(srcTable = reduce_table_name):
        change_preference_pair_list = eval(record.value)
        
    MapReduce.dropTables(dstTables=[map_table_name, reduce_table_name])
    
    changing_count = 0
    for key in change_preference_pair_list:
        changing_count += int(change_preference_pair_list[key])
        
    for key in change_preference_pair_list:
        change_preference_pair_list[key] = float(change_preference_pair_list[key]) / float(changing_count)
        
    return change_preference_pair_list
        
def _get_preference_pair_percent_list_map_(record):
    yield Record("1", '', record.value)    
        
def _get_preference_pair_percent_list_reduce_(key, records):        
    change_preference_pair_list = {}
    
    for record in records:
        field_dictionary = yx_log_parser.get_field_value_dictionary(record.value)
        pos1 = field_dictionary["preference_pos"][0]
        pos2 = field_dictionary["preference_pos"][1]
        
        str_preference = pos1 + ">" + pos2
        
        if not change_preference_pair_list.has_key(str_preference):
            change_preference_pair_list[str_preference] = 1
        else:
            change_preference_pair_list[str_preference] += 1
            
    yield Record(key, '', repr(change_preference_pair_list))
    