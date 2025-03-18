# -*- coding: utf-8 -*-
import six


def unbeautify_fields(lookup, mapping):
    raw_lookup = dict((mapping.get(k, k), v) for k, v in six.iteritems(lookup))
    return raw_lookup


def beautify_fields(lookup, mapping):
    # !warning: key and vals swaps here! vals must be unique
    unmapping = dict((v, k) for k, v in six.iteritems(mapping))
    return unbeautify_fields(lookup, unmapping)
