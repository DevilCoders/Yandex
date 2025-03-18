#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# $Id$
# $HeadURL$

'''
Shell like curly braces expander.

Full Description:
https://wiki.yandex-team.ru/users/vlegeza/aspam/ygetparam#curlybracesexpander.py
'''

__all__ = ['Expand', 'ExpandToStr', 'IsExpandable', 'DEFAULT_STEP_SIZE']

DEFAULT_STEP_SIZE = 1
DEFAULT_DELIMITER_IN_STRING = ' '


def Expand(string, step=DEFAULT_STEP_SIZE):
    ''' str -> tuple
    API function.
    Tries to separate pre- and post- amble and send amble to expansion.
    Returns tuple of values or tuple of a single element if expansion is
    not applicable.
    '''
    retVal = []
    startPoint = string.find('{')
    if startPoint == -1:
        return (string,)
    endPoint = string.find('}')
    while string.count('{', startPoint, endPoint) \
            != string.count('}', startPoint, endPoint):
        endPoint += 1
        if endPoint > len(string):
            startPoint += 1
            endPoint = startPoint + 1
            if startPoint >= len(string):
                return (string,)
    prefix = string[:startPoint]
    infix = ExpandDoubleDotsAndCommas(string[startPoint:endPoint], step)
    suffix = Expand(string[endPoint:], step)

    # SpinUp
    for i in infix:
        if suffix:
            for j in suffix:
                retVal.append(prefix + i + j)
        else:
            retVal.append(prefix + i)
    return tuple(retVal)


def ExpandDoubleDotsAndCommas(originalString, step=DEFAULT_STEP_SIZE):
    ''' str -> tuple
    Tries to open braces and return tuple of values or tuple of a single
    element if expansion is not applicable.
    '''

    # Strip braces
    string = originalString[1:-1]

    # Expand commas
    parts = DivideByComma(string)
    if len(parts) > 1:
        return reduce(lambda x, y: x + y, map(Expand, parts))

    # Expand double dots
    parts = string.split('..')
    if len(parts) < 2:
        return (originalString,)
    # Extract step size
    if len(parts) == 3:
        if parts[2].isdigit():
            step = int(parts[2])
        else:
            return (originalString,)
    elif len(parts) > 3:
        return (originalString,)

    strFrom = parts[0]
    strTo = parts[1]
    idxAdjust = 1  # Adjust index to include final element into range

    # Positive and Negative Digits
    if strFrom.lstrip('-').isdigit() and strTo.lstrip('-').isdigit():
        length = min(len(strFrom.strip('-')), len(strTo.strip('-')))
        fmt = "{:0" + str(length) + "}"
        if int(strFrom) > int(strTo):
            step *= -1
            idxAdjust = -1
        return map(fmt.format, range(int(strFrom), int(strTo) + idxAdjust, step))
    # Chars
    elif (strFrom.isalpha() and len(strFrom) == 1) \
            and (strTo.isalpha() and len(strTo) == 1):
        if ord(strFrom) > ord(strTo):  # Reverce order
            step *= -1
            idxAdjust = -1
        return map(chr, range(ord(strFrom), ord(strTo) + idxAdjust, step))
    # Unexpandable
    return (originalString,)


def DivideByComma(string):
    ''' str -> list
    Divide ignoring commas inside curly braces.
    '''
    allParts = list()
    firstBrace = string.find('{')
    firstComma = string.find(',')
    if firstBrace == -1:
        return string.split(',')
    if firstComma == -1:
        return [string]
    if firstBrace < firstComma:
        endPoint = firstBrace + 1
        forwardBracesCount = string.count('{', firstBrace, endPoint)
        backwardBracesCount = string.count('}', firstBrace, endPoint)
        while backwardBracesCount != forwardBracesCount:
            endPoint += 1
            if string[endPoint] == '}':
                backwardBracesCount += 1
            elif string[endPoint] == '{':
                forwardBracesCount += 1
        stringParts = string[endPoint:].split(',', 1)
        allParts.append(string[:endPoint] + stringParts[0])
        if len(stringParts) > 1:
            allParts += DivideByComma(stringParts[1])
    else:
        allParts.append(string[:firstComma])
        allParts += DivideByComma(string[firstComma + 1:])
    return allParts


def IsExpandable(string):
    ''' str -> bool '''
    return len(Expand(string)) > 1


def ExpandToStr(string, delim=DEFAULT_DELIMITER_IN_STRING):
    ''' str -> str
        Expand string (shell command) as it could be done by shell. Divide string
        by spaces and combine result into a single string.
    '''
    if not delim:
        return delim.join(reduce(lambda x, y: x + y, map(Expand, (string,))))
    return delim.join(reduce(lambda x, y: x + y, map(Expand, string.split(delim))))
