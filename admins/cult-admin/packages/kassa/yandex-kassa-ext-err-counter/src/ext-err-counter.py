#!/usr/bin/python
from __future__ import division
import sys, os
import argparse
from subprocess import PIPE, Popen
import yaml

argparser = argparse.ArgumentParser(description='Counts error procent for external services')
argparser.add_argument('-l', '--log', action='store', nargs='?', type=str, default='/tmp/ext-services-errors.last1hr', help='Path to logfile with markers')
argparser.add_argument('-t', '--threshold', action='store', nargs='?', type=float, default=10, help='Min value for err_prc, when marker prints. Default: 10')
argparser.add_argument('-ct', '--custom-threshold', action='store', nargs='?', type=yaml.load, help='Custrom threshold for custom marker' )
argparser.add_argument('-at', '--abs-threshold', action='store_true', help='Use threshold as absolut value, not percent')
argparser.add_argument('-o', '--output', action='store', help='Output format [graphite|monrun|debug]')
argparser.add_argument('-mt', '--mtype', action='store', type=str, default='EXT', choices=['EXT', 'GATEWAY', 'CINEMANETWORK'], help='What type of flags provide in monrun output')
argparser.add_argument('-m', '--minr', action='store', type=int, default=0, help='Min count of requests for monrun check. Default: 0')

# Parameters for work
VARIABLES = argparser.parse_args()
THRESHOLD = VARIABLES.threshold
if VARIABLES.custom_threshold:
    CUSTOM_THRESHOLD = VARIABLES.custom_threshold
else:
    CUSTOM_THRESHOLD = {}

MTYPE = VARIABLES.mtype
LOG = VARIABLES.log
MINR = VARIABLES.minr
S1_MARKER = MTYPE + "_INFO Start execute"
E1_MARKER = MTYPE + "_ERROR "
S_MARKER = MTYPE + "_INFO"
E_MARKER = MTYPE + "_ERROR"
# Initialize default main dict
MARKERS = {VARIABLES.mtype: {}}

# Functions
def read_log(log):
	try:
        	with open(log,'r') as f:
                	return f.readlines()
	except IOError:
		print("1;No file: %s" % (log))
		sys.exit(0)

def count_markers(lines):
	for line in lines:
		if S1_MARKER in line or E1_MARKER in line:
			elems = line.split()
			for elem in elems:
				if S_MARKER in elem or E_MARKER in elem:
					try:
						c_marker , c_type , c_state = elem.split("_", 2)
					except:
						continue
					if c_state == "INFO" or c_state == "ERROR":
						if not c_type in MARKERS:
							MARKERS[c_type] = {}
						if not c_marker in MARKERS[c_type]:
							MARKERS[c_type][c_marker] = {}
							MARKERS[c_type][c_marker]["ERROR"] = 0
							MARKERS[c_type][c_marker]["INFO"] = 0
							MARKERS[c_type][c_marker]["ERR_PRC"] = 0

						MARKERS[c_type][c_marker][c_state] = MARKERS[c_type][c_marker].get(c_state, 0) + 1

def get_procents():
	for c_type in MARKERS:
		for c_marker in MARKERS[c_type]:
			try:
				MARKERS[c_type][c_marker]["ERR_PRC"] = round(MARKERS[c_type][c_marker]["ERROR"] * 100 / MARKERS[c_type][c_marker]["INFO"], 2)
			except ZeroDivisionError:
                        	MARKERS[c_type][c_marker]["ERR_PRC"] = 100
			except Exception as err:
                         	print("2;Broken.%s:%s:%s" % (c_type, c_marker, err))
                        	sys.exit(0)
                

def printer():
	if VARIABLES.output == 'debug':
		print("Using log: %s" % LOG)
		print ("Threshold for show: %s" % THRESHOLD)
		print("Using type filter: %s" % MTYPE)
                for key in CUSTOM_THRESHOLD:
                    print("Custrom thresholds for %s: %s" % (key, CUSTOM_THRESHOLD[key]))
		print MARKERS

	if VARIABLES.output == 'graphite':
                for ctype in MARKERS:
			for marker in MARKERS[ctype]:
				print("%s.%s.total %s" % (ctype,marker,MARKERS[ctype][marker]["INFO"]))
				print("%s.%s.error %s" % (ctype,marker,MARKERS[ctype][marker]["ERROR"]))

        if VARIABLES.output == 'monrun':
                result_msg = ''
                for marker in MARKERS[MTYPE]:
                        c_THRESHOLD = THRESHOLD
                        c_MINR = MINR
                        msg = ''
                        if marker in CUSTOM_THRESHOLD:
                            if "THRESHOLD" in CUSTOM_THRESHOLD[marker].keys():
                                c_THRESHOLD = float(CUSTOM_THRESHOLD[marker]['THRESHOLD'])
                            if "MINR" in CUSTOM_THRESHOLD[marker]:
                                c_MINR = int(CUSTOM_THRESHOLD[marker]['MINR'])

                        if not VARIABLES.abs_threshold:
                            if float(MARKERS[MTYPE][marker]["ERR_PRC"]) >= c_THRESHOLD:
                                if int(MARKERS[MTYPE][marker]["INFO"]) >= c_MINR and int(MARKERS[MTYPE][marker]["INFO"]) != 0:
                                    msg = marker + ':' + str(MARKERS[MTYPE][marker]["ERR_PRC"]) + '(' + str(MARKERS[MTYPE][marker]["ERROR"]) + '/' + str(MARKERS[MTYPE][marker]["INFO"]) + ')'
			        elif int(MARKERS[MTYPE][marker]["INFO"]) == 0:
				    msg = marker + ':NoINFOlasthour' + '(' + str(MARKERS[MTYPE][marker]["ERROR"]) + '/' + str(MARKERS[MTYPE][marker]["INFO"]) + ')'
                        else:
                            if float(MARKERS[MTYPE][marker]["ERROR"]) >= c_THRESHOLD:
                                msg = marker + ':' + str(MARKERS[MTYPE][marker]["ERROR"])
                        if msg:
                            result_msg = ';'.join([result_msg,msg])
                if result_msg:
                        print('2%s' % result_msg)
                else:
                        print('0;ok')

# Fire!
if __name__ == "__main__":
        if len(sys.argv)==1:
            argparser.print_help()
            sys.exit(0)
	count_markers(read_log(LOG))
        if not VARIABLES.abs_threshold:
	    get_procents()
	printer()
