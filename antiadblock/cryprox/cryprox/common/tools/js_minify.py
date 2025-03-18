
# Minify javascript file
# (C) Amit Sengupta, Sep 2015
# Written in Python 2.7

from io import StringIO


class Minify:
    EOF = -1

    def __init__(self, js_content=''):

        self.original_data = StringIO()
        self.original_data.write(js_content.decode('utf-8'))
        self.original_data.seek(0)
        self.modified_data = ""  # data after processing
        self.is_error = False  # becomes true when error occurs
        self.error_msg = ""  # error message

        # process js
        self.doProcess()

    # main process
    def doProcess(self):
        last_char = -1  # previous byte read
        this_char = -1  # current byte read
        next_char = -1  # byte read in peek
        end_process = False  # terminate flag
        ignore = False  # if false then add byte to final output
        in_comment = False  # true when current byte is part of a comment
        is_double_slash_comment = False  # true when current comment is //

        while not end_process:
            end_process = self.peek() == Minify.EOF
            if end_process:
                break
            ignore = False
            this_char = self.original_data.read(1)

            if this_char == '\t':
                this_char = ' '
            elif this_char == '\r':
                this_char = '\n'
            if this_char == '\n':
                ignore = True

            if this_char == ' ':
                if last_char == ' ' or self.isDelimiter(last_char):
                    ignore = True
                else:
                    end_process = self.peek() == Minify.EOF
                    if not end_process:
                        next_char = self.peek()
                        if self.isDelimiter(next_char):
                            ignore = True

            if this_char == '/' and last_char not in "\"\'":
                next_char = self.peek()
                if next_char in '/*':
                    ignore = True
                    in_comment = True
                    is_double_slash_comment = True if next_char == '/' else False

            if in_comment:
                while 1:
                    this_char = self.original_data.read(1)
                    next_char = self.peek()
                    if this_char == '*':
                        if next_char == '/':
                            this_char = self.original_data.read(1)
                            in_comment = False
                            break

                    if is_double_slash_comment and this_char == '\n':
                        in_comment = False
                        break

                ignore = True

            if not ignore:
                self.addToOutput(this_char)

            last_char = this_char

        self.modified_data = self.modified_data.replace('const ', 'var ').replace('let ', 'var ')

    # add byte to modified data
    def addToOutput(self, c):
        self.modified_data += str(c)

    # python 2.x does not support peek so make our own
    def peek(self):
        b = self.original_data.read(1)
        self.original_data.seek(self.original_data.tell() - 1)
        return b if b else Minify.EOF

    # check if a byte is a delimiter
    def isDelimiter(self, c):
        return True if c in "(,=:[!&|?+-~*/{\n<^%" else False

    def getMinify(self):
        return self.modified_data
