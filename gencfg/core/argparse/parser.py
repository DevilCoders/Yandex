import sys
from argparse import ArgumentParser

import core.argparse.types


class ArgumentParserExt(ArgumentParser):
    def __init__(self, *args, **kwargs):
        self.ext_args_info = []

        # Some utilities accept --db option as custom database. On the other hand,
        # some functions, that 'create' parameters
        # use CURDB. Thus we should spook CURDB by our db during arguments parsing
        self.db_argument_parser = ArgumentParser()
        self.db_argument_found = False

        ArgumentParser.__init__(self, *args, **kwargs)

    def add_argument(self, *args, **kwargs):
        if 'type' in kwargs:
            var_type = kwargs['type']
        elif 'action' in kwargs and kwargs['action'] in ['store_true', 'store_false']:
            var_type = bool
        elif 'action' in kwargs and kwargs['action'] in ['count']:
            var_type = int
        else:
            var_type = str

        if var_type == core.argparse.types.gencfg_db:
            # a piece of shit: have to parse db at first
            self.db_argument_parser.add_argument(*args, **kwargs)
            self.db_argument_found = True
        else:
            ArgumentParser.add_argument(self, *args, **kwargs)

        if 'default' in kwargs:
            var_default = kwargs['default']
        else:
            var_default = None

        if 'dest' in kwargs:
            var_name = kwargs['dest']
        elif len(args) > 0:
            last_arg = args[-1]
            first = 0
            while first < len(last_arg) and last_arg[first] == '-':
                first += 1
            var_name = last_arg[first:].replace('-', '_')
        else:
            raise Exception("Not found dest var while adding argument")

        self.ext_args_info.append((var_name, var_type, var_default))

    def parse_json(self, obj, convert_var_types=True):
        class JsonOptions(object):
            pass

        options = JsonOptions()
        for key, value in obj.items():
            setattr(options, key, value)

        try:
            # prepare new curdb
            if self.db_argument_found:
                from core.db import CURDB, get_real_db_object, set_current_thread_CURDB
                old_curdb = CURDB._get_current_object()

                item = filter(lambda (var_name, var_type, var_default): var_type == core.argparse.types.gencfg_db,
                              self.ext_args_info)[0]
                var_name, var_type, var_default = item
                options_curdb = var_type(getattr(options, var_name, var_type(var_default)))

                set_current_thread_CURDB(options_curdb)

            # process args
            for var_name, var_type, var_default in self.ext_args_info:
                if var_name in obj and getattr(options, var_name) is not None:
                    if convert_var_types:
                        setattr(options, var_name, var_type(getattr(options, var_name)))
                else:
                    if var_default is not None:
                        setattr(options, var_name, var_type(var_default))
                    else:
                        setattr(options, var_name, None)
        finally:
            # restore curdb
            if self.db_argument_found:
                set_current_thread_CURDB(old_curdb)

        return options

    def parse_cmd(self):
        if len(sys.argv) == 1:
            sys.argv.append('-h')
            options = self.parse_args()
        elif self.db_argument_found:
            options, unparsed_args = self.db_argument_parser.parse_known_args()
            options_db_name, options_db = options.__dict__.iteritems().next()

            # replace CURDB by what was loaded from <--db> argument
            from core.db import get_real_db_object, set_current_thread_CURDB
            old_curdb = get_real_db_object()

            set_current_thread_CURDB(options_db)

            options = self.parse_args(unparsed_args)
            setattr(options, options_db_name, options_db)

            set_current_thread_CURDB(old_curdb)
        else:
            options = self.parse_args()

        return options
