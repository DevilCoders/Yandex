#!/usr/bin/env python

import sys
import os
import subprocess
import json
import argparse
import re


class ParseResult:
    def __init__(self, state, data, errs, result):
        self.skip = state #True if test shoud be passed
        self.input = data
        self.errs = errs #errors count
        self.result = result

class ParseText:
    def __init__(self, inputf):
        self.inputf = inputf

    def __iter__(self):
        return self

    def next(self):
        self.line = self.inputf.next()
        if not self.line:
            raise StopIteration

        return self.parse_one_test(self.line)

    def parse_one_test(self, line):
        data = ""
        skip = False;

        if line.strip('\n') == "#data":
            for d in self.inputf:
                if d.strip('\n') == "#errors":
                    line = d
                    break;
                data += d

        err_num = 0
        if line.strip('\n') == "#errors":
            for err in self.inputf:
                if err.strip('\n') == "#document" or err.strip('\n') == "#document-fragment":
                    line = err
                    break;
                err_num += 1

        # Some tests has #document-fragment and it is not interesting tests
        if line.strip('\n') == "#document-fragment":
            for sline in self.inputf:
                if  sline == "\n":
                    break;
            skip = True

        result = ""
        if line.strip('\n') == "#document":
            for doc_part in self.inputf:
                if doc_part == "\n":
                    break;
                result += doc_part

        if len(data) and data[-1] == '\n':
            data = data[:len(data)-1]

        return ParseResult(skip, data, err_num, result)


class ParseJson:
    def __init__(self, inputf):
        input_json = inputf.read()

        decoder = json.JSONDecoder()
        try:
            self.tests = decoder.decode(input_json)["tests"]
        except:
            self.tests = decoder.decode(input_json)["xmlViolationTests"]
        self.count = len(self.tests)

    def __iter__(self):
        self.current = 0
        return self

    def next(self):
        if self.current >= self.count:
            raise StopIteration

        test =  self.tests[self.current]
        self.current += 1

        return ParseResult(False, test["input"], 0, json.dumps(test["output"]))


def run_test(binary, test, verbose):
    class bcolors:
        HEADER = '\033[95m'
        OKBLUE = '\033[94m'
        OKGREEN = '\033[92m'
        WARNING = '\033[93m'
        FAIL = '\033[91m'
        ENDC = '\033[0m'


    # RUN SINGLE TEST
    (data, err_num, result) = test.input, test.errs, test.result
    (out, err) = subprocess.Popen([binary, "--stdin"], stdout=subprocess.PIPE, stdin=subprocess.PIPE).communicate(input=data.encode("utf-8"))

    if out == result:
        if verbose:
            print bcolors.OKGREEN + "Test passed: " + json.JSONEncoder().encode(data) + " " + bcolors.ENDC
#            print bcolors.OKGREEN + result + bcolors.ENDC
        return 0
    else:
        if verbose:
            print bcolors.FAIL + "Test fail: " + json.JSONEncoder().encode(data)
            print bcolors.OKBLUE + result
            print bcolors.FAIL + out + bcolors.ENDC
        return 1


INPUT_MODES = {"text" : 0, "json" : 1} #correlates with TEST_PARSERS
TEST_PARSERS = [ParseText, ParseJson]
BINARIES = ["gumbo_tree/gprintt", "gtokenizer/gtokenizer"]
MATCHERS = [re.compile('.*\.dat'), re.compile('.*\.test')]

def process_one_file(mode, test_data, lcount, verbose=True):
    print "start processing... %s \n" % (test_data)

    proc = INPUT_MODES[mode]

    inputf = open(test_data, 'r')
    (errs, total, correct, skipped) = (0, 0, 0, 0)

    for test in TEST_PARSERS[proc](inputf):
        if lcount > 0 and total >= lcount:
            break;
        total += 1

        if not test.skip:
            errs += run_test(BINARIES[proc], test, verbose)
        else:
            skipped += 1

    correct = total - errs - skipped

    return (total, correct, errs, skipped)


####### MAIN #########
def main(mode, test_data, verbose, lcount):
    (total, correct, errs, skipped) = (0, 0, 0, 0)
    err_list = []

    if os.path.isdir(test_data):
        for data_file in sorted(os.listdir(test_data)):
            p = MATCHERS[INPUT_MODES[mode]]
            if p.match(data_file):
                if os.path.isfile(test_data + data_file):
                    (t, c, e, s) = process_one_file(mode, test_data + data_file, lcount, verbose)
                    total += t
                    correct += c
                    errs += e
                    skipped += s
                    if e > 0:
                        err_list.append((data_file, e))
    elif os.path.isfile(test_data):
        (total, correct, errs, skipped) = process_one_file(mode, test_data, lcount, verbose)
        if errs > 0:
            err_list.append((test_data, errs))

    print "#################################"
    print "Total: %d\nPassed: %d\nErrors %d\nSkipped: %d" % (total, correct, errs, skipped)
    print "\nLooking this tests:"
    for (filename, count) in err_list:
        print "%s (%d)" % (filename, count)


if __name__ == '__main__':
    optparser = argparse.ArgumentParser(prog = sys.argv[0], description="Run html5tests")

    optparser.add_argument("-m", "--mode", help="tests input mode", type=str, choices=["text", "json"], dest="mode")
    optparser.add_argument("-v", help="verbose output", action="store_true", dest="verbose")
    optparser.add_argument(help="input dir or file", type=str,  dest="input")
    optparser.add_argument("-b", "--binary", help="tested binary", type=str,  dest="binary", required=True);
    optparser.add_argument("-l", "--lcount", help="tests count", type=int,  dest="lcount", required=False, default=0);

    opt = optparser.parse_args()

    if not opt.binary is None:
        BINARIES[INPUT_MODES[opt.mode]] = opt.binary

    main(opt.mode, opt.input, opt.verbose, opt.lcount)
