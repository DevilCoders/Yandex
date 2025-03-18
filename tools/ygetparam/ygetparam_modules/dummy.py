#!/usr/bin/env python
'''
Empty handler that returns passed string back as is.
Usage:
    - Module interface testing
    - Template for new module
'''


class TestException(Exception):
    ''' Base exception class. '''
    pass


class Module(object):
    ''' Module Entry Point object '''

    def __call__(self, string):
        ''' str -> tuple
        Main function.

        "string" is the only thing you can pass to handler, however you may include
        options inside and retrieve them locally during parsing.
        '''
        try:

            # For tests purpose only.
            if string == 'return non tuple':
                return string
            if string == 'return int in tuple':
                return (123456789,)

            # Normal behavior.
            return (string,)
        except Exception as error:
            raise TestException(error)
