#!/skynet/python/bin/python

from __future__ import print_function

class ParseContext(object):
    __slots__ = ['stage', 'stack', 'global_sections']

    class STAGES:
        NONE = 'NONE'
        PROCESS_MACROSES = 'PROCESS_MACROSES'
        PROCESS_MODULES = 'PROCESS_MODULES'

    # make object singletone
    def __new__(cls):
        if not hasattr(cls, 'instance'):
            cls.instance = super(ParseContext, cls).__new__(cls)

            # information fields
            cls.instance.stage = ParseContext.STAGES.NONE
            cls.instance.stack = []

            """
                Dict of <modules as python dict/list> -> (varname, python_dict) . Every global section add
                "varname = { ... }" before "instance = {" section. All <python_dict> in "instance = {" then
                replaced by varname. Example:
                varname = { a = 123; b = "dddd"; };
                instance = {
                    something = {
                        somevar = varname;
                        ...
                    };
                    ...
                };

            """
            cls.instance.global_sections = dict()

        return cls.instance

    def reset(self):
        self.stage = ParseContext.STAGES.NONE
        self.stack = []
        self.instance.global_sections.clear()

    def __str__(self):
        result = '==============================================================\n'
        result += 'Parse context at stage %s.\nStack:\n' % self.stage
        result += ''.join('    %s\n' % x for x in self.stack)
        result += '==============================================================\n'
        return result

    """
        Generate variable name for global_sections
    """

    def generate_global_section_name(self, prefix):
        filtered_varnames = map(lambda x: x.name, self.global_sections.itervalues())
        filtered_varnames = filter(lambda x: x.startswith(prefix), filtered_varnames)

        if len(filtered_varnames) == 0:
            max_index = 0
        else:
            max_index = max(map(lambda x: int(x[len(prefix):]), filtered_varnames)) + 1

        return "%s%s" % (prefix, max_index)


class NewStage(object):
    __slots__ = ['saved', 'newstage']

    def __init__(self, newstage):
        self.saved = ParseContext().stage
        self.newstage = newstage

    def __enter__(self):
        ParseContext().stage = self.newstage

    def __exit__(self, type, value, traceback):
        ParseContext().stage = self.saved


class StackEntry(object):
    __slots__ = ['entry']

    def __init__(self, entry):
        self.entry = entry

    def __enter__(self):
        ParseContext().stack.append(self.entry)

    def __exit__(self, type, value, traceback):
        # assert (len(ParseContext().stack) and ParseContext().stack[-1] == self.entry)
        ParseContext().stack.pop()


if __name__ == '__main__':
    xx = ParseContext()
    with NewStage(ParseContext.STAGES.PROCESS_MODULES):
        print(xx)
        print(xx.stage)
        with StackEntry('first entry'):
            print(xx)
            print(xx.stage)
            with StackEntry('second entry'):
                print(xx)
            print(xx)
    print(xx)
