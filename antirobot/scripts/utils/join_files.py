#!/usr/bin/env python


# Generalized files joining
#
# fName1, fName: names of files to join
# keyGetter1, keyGetter2: functors which takes line from file, and returns a key by hich the files will be joined
# fNameRes: the name of result file
# mergeFunc: a func that takes lines from both files and returns a joined line
#
def JoinFiles(fName1, fName2, keyGetter1, keyGetter2, fNameRes, mergeFunc):
    def GetNextPair(hFile1, hFile2):
        def Next(f, keyGetter):
            line = f.next()
            if not line:
                return '', None

            line = line.rstrip()
            key = keyGetter(line)
            return (line, key)

        try:
            line1, key1 = Next(hFile1, keyGetter1)
            line2, key2 = Next(hFile2, keyGetter2)
            while True:
                if key1 == key2:
                    yield (line1, line2)

                    line1, key1 = Next(hFile1, keyGetter1)
                    line2, key2 = Next(hFile2, keyGetter2)

                elif key1 < key2:
                    line1, key1 = Next(hFile1, keyGetter1)
                else:
                    line2, key2 = Next(hFile2, keyGetter2)

        except:
            yield None

    if not mergeFunc:
        raise Exception("Merge func must be present")

    f1 = open(fName1)
    f2 = open(fName2)

    fRes = open(fNameRes, 'w')

    for pair in GetNextPair(f1, f2):
        if not pair:
            break

        print >>fRes, mergeFunc(pair[0], pair[1])


# Joins two files by the same manner as 'join' utility
# Both files must be tab delimited, and alphabetic sorted by the first field
# mergeFunc is a func which takes two lines from each files and returns merged result
def SimpleJoinFiles(fName1, fName2, fNameRes, mergeFunc=None):
    keyGetter = lambda x: x.split('\t')[0]

    if not mergeFunc:
        mergeFunc = lambda l1, l2: '%s\t%s' % (l1, '\t'.join(l2.split('\t', 1)[1:]))

    JoinFiles(fName1, fName2, keyGetter, keyGetter, fNameRes, mergeFunc)
