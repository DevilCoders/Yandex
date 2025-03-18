import sys

if sys.version_info[0] < 3:
    def byteify(input):
        if isinstance(input, dict):
            return {byteify(key): byteify(value) for key, value in input.iteritems()}
        if isinstance(input, list):
            return [byteify(element) for element in input]
        if isinstance(input, unicode):  # noqa
            return input.encode('utf-8')
        return input
else:
    def byteify(input):
        return input
