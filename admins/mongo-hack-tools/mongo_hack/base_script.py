import argparse
import sys


class BaseScript (object):

    script_description  = ''
    argument_names      = []

    def __init__(self):
        try:
            self.parse_args()
            self.check_if_okay()
            self.run()
            print "\n\nSUCCESS\n\n"
        except BaseException as x:
            print "\n\nERROR: %s\n\n" % x
            raise

    def parse_args(self):
        parser = argparse.ArgumentParser(description=self.script_description)
        for arg in self.argument_names:
            if arg.endswith('+'):
                parser.add_argument(arg[:-1], nargs='+')
            else:
                parser.add_argument(arg)
        self.args = parser.parse_args()

    def gather_info(self):
        return {}

    def check_if_okay(self):
        data = self.gather_info()

        maxlen = 0
        for item in data:
            if len(item[0]) > maxlen:
                maxlen = len(item[0])
        tpl = '  %%%ds: %%s' % maxlen

        print
        print self.script_description
        print
        for item in data:
            print tpl % item
        print
        if raw_input ('PROCEED (y/N)? ') not in ('y', 'Y'):
            print 'Exit due to console request'
            sys.exit(-1)
        print
