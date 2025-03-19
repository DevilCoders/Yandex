import json

from . import fail


def toStr(v):
    if type(v) == str:
        return v
    else:
        return json.dumps(v)

# HACK: Treat all metadata with name *configs as JSON.


def shouldParseAsJson(p):
    return len(p) == 2 and p[0] == "metadata" and p[1].endswith("configs")


# A very cheap and dirty of comparing strings similarity by comparing their histograms.
# HACK: Have a global (sic!) cache of string ratios.
strMatchRatioCache = {}


def computeStrMatchRatio(s1, s2):
    if s1 == "" and s2 == "":
        return 1.0
    if (s1, s2) in strMatchRatioCache:
        return strMatchRatioCache[(s1, s2)]
    sorted1 = sorted(s1)
    sorted2 = sorted(s2)
    i1 = 0
    i2 = 0
    ratio = 0.0
    while i1 < len(sorted1) and i2 < len(sorted2):
        if sorted1[i1] < sorted2[i2]:
            i1 += 1
        elif sorted1[i1] > sorted2[i2]:
            i2 += 1
        else:
            i1 += 1
            i2 += 1
            ratio += 1.0
    ret = ratio / max(len(s1), len(s2))
    strMatchRatioCache[(s1, s2)] = ret
    return ret


def computeStrDiff(before, after, path):
    if before == after:
        return [], 1.0

    if before is None:
        before = ""
    if after is None:
        after = ""

    if shouldParseAsJson(path):
        # Try to parse JSON.
        try:
            beforeJson = json.loads(before)
            afterJson = json.loads(after)
            return computeDiff(beforeJson, afterJson, path)
        except:
            # Not a JSON
            pass

    # Try to split values in a number of lines.
    if "\n" in before or "\n" in after:
        beforeLines = before.split("\n")
        afterLines = after.split("\n")
        return computeListDiff(beforeLines, afterLines, path)

    ratio = computeStrMatchRatio(before, after)
    return [(path, before, after)], ratio


def computeNumberDiff(before, after, path):
    if before == after:
        return [], 1.0
    else:
        return [(path, before, after)], 0.0


SIMILAR_RATIO_THRESHOLD = 0.75
# Find the best matches both in row diffMatrix[i1][i2:] and in column diffMatrix[i1:][i2], return the indices
# and corresponding ratios.


def bestMatches(diffMatrix, i1, i2):
    bestRatio1 = 0.0
    for i in range(i2, len(diffMatrix[i1])):
        _, r = diffMatrix[i1][i]
        bestRatio1 = max(r, bestRatio1)
    bestRatio2 = 0.0
    for i in range(i1, len(diffMatrix)):
        _, r = diffMatrix[i][i2]
        bestRatio2 = max(r, bestRatio2)
    # Do not use bestRatio1/2, find the closest match which is "good enough".
    i1match = i2
    r1match = 0.0
    i2match = i1
    r2match = 0.0
    for i in range(i2, len(diffMatrix[i1])):
        _, r = diffMatrix[i1][i]
        if r >= bestRatio1 * SIMILAR_RATIO_THRESHOLD:
            i1match = i
            r1match = r
            break
    for i in range(i1, len(diffMatrix)):
        _, r = diffMatrix[i][i2]
        if r >= bestRatio2 * SIMILAR_RATIO_THRESHOLD:
            i2match = i
            r2match = r
            break
    return i1match, r1match, i2match, r2match


FINE_DIFF_THRESHOLD = 0.25


def computeListDiff(before, after, path):
    if before is None:
        before = []
    if after is None:
        after = []
    maxLen = max(len(list(i for i in before if i != "")),
                 len(list(i for i in after if i != "")))
    if maxLen == 0:
        return [], 1.0

    # Compute similarity matrix.
    diffMatrix = []
    for b in before:
        row = []
        for i, a in enumerate(after):
            if a == b:
                row.append(([], 1.0))
            else:
                row.append(computeDiff(b, a, path + (i,)))
        diffMatrix.append(row)

    ret = []
    ratio = 0.0
    i1 = 0
    i2 = 0
    while i1 < len(before) and i2 < len(after):
        # Ignore the empty strings.
        if before[i1] == "":
            i1 += 1
            continue
        if after[i2] == "":
            i2 += 1
            continue

        elemPath = path + (i2,)

        i1match, r1match, i2match, r2match = bestMatches(diffMatrix, i1, i2)
        if i1match == i2 and i2match == i1:
            diff, r = diffMatrix[i1][i2]
            if r > FINE_DIFF_THRESHOLD:
                ret += diff
            else:
                # The elements are too different, output in short form.
                ret.append((elemPath, toStr(before[i1]), toStr(after[i2])))
            ratio += r
            i1 += 1
            i2 += 1
        else:
            # Move forward the index with a lesser match value or the one which belongs to the longer list
            # (this is a hack to support the case when one list contains the second list plus a number of
            # new items).
            if r1match < r2match or (r1match == r2match and len(before) - i1 > len(after) - i2):
                ret.append((elemPath, toStr(before[i1]), None))
                i1 += 1
            else:
                ret.append((elemPath, None, toStr(after[i2])))
                i2 += 1

    while i1 < len(before):
        if before[i1] != "":
            ret.append((path + (i2,), toStr(before[i1]), None))
        i1 += 1
    while i2 < len(after):
        if after[i2] != "":
            ret.append((path + (i2,), None, toStr(after[i2])))
        i2 += 1

    ratio /= float(maxLen)
    return ret, ratio


def escapeKeyForPath(key):
    if "." in key or "/" in key:
        return '"' + key + '"'
    else:
        return key


def computeMapDiff(before, after, path):
    if before is None:
        before = {}
    elif after is None:
        after = {}
    allKeys = set(before.keys()) | set(after.keys())
    if len(allKeys) == 0:
        return [], 1.0
    ret = []
    keyRatio = 0.0
    valueRatio = 0.0
    for key in allKeys:
        if path == "":
            keyPath = (escapeKeyForPath(key),)
        else:
            keyPath = path + (escapeKeyForPath(key),)
        if key not in before:
            ret.append((keyPath, None, toStr(after[key])))
        elif key not in after:
            ret.append((keyPath, toStr(before[key]), None))
        else:
            diff, r = computeDiff(before[key], after[key], keyPath)
            ret += diff
            keyRatio += 1.0
            valueRatio += r
    ratio = (keyRatio + valueRatio) * 0.5 / float(len(allKeys))
    return ret, ratio


def computeDiff(before, after, path):
    if isinstance(before, dict) or isinstance(after, dict):
        return computeMapDiff(before, after, path)
    elif isinstance(before, list) or isinstance(after, list):
        return computeListDiff(before, after, path)
    elif isinstance(before, str) or isinstance(after, str):
        return computeStrDiff(str(before), str(after), path)
    elif isinstance(before, int) or isinstance(before, float) or isinstance(after, int) or isinstance(after, float):
        return computeNumberDiff(before, after, path)
    elif before is None and after is None:
        return [], 1.0
    else:
        raise fail.FailException("Do not know how to compute diff for path {0} (types {1}, {2})".format(
            path, type(before), type(after)))
