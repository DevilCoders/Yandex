from xml.dom.minidom import parse

from xml.etree.ElementTree import ElementTree

import sys, pprint
from collections import defaultdict

class YaCatalogParser(object):
    def __init__(self, filename):
        """ filename: path to YaCatalog.xml """
        tree = ElementTree()
        tree.parse(filename)

        self.c2p = {}
        for cat in tree.findall('cat/cat'):
            self.c2p[cat.get('id')]= cat.get('parent_id')


    def get_path(self, cat):
        path = []
        while cat:
            path.append(cat)
            cat = self. c2p[cat]
        return path

    def select_general(self, categories):
        """ Port of arcadia/web/report/lib/YxWeb/Util/Category.pm to Python """

        paths = [list(reversed(self.get_path(cat))) for cat in categories]

        max_path_len = max(len(path) for path in paths)

        combined_path = []
        for i in xrange(max_path_len):
            element_freqs = defaultdict(int)
            for path in paths:
                if i < len(path):
                    element_freqs[path[i]] += 1

            var = defaultdict(list)
            for element, freq in element_freqs.iteritems():
                var[freq].append(element)

            combined_path.append(var)

        level_by_intersection = {}
        for level, var in enumerate(combined_path):
            level_by_intersection[max(var.keys())] = level

        max_intersection = max(level_by_intersection.keys())

        for intersection in reversed(xrange(2, len(categories) + 1)):
            level = level_by_intersection.get(intersection, 0)
            if level > 0:
                ids = combined_path[level][intersection]

                if len(ids) > 1 and level > 1:
                    ids = combined_path[1] [ max( combined_path[1].keys() ) ]

                return max(ids)

        return max(categories)

if __name__ == '__main__':
    if ',' in sys.argv[1]:
        cats = sys.argv[1].split(',')
    else:
        cats = sys.argv[1].split()

    parser = YaCatalogParser("YaCatalog.xml")
    print " ".join(cats), '==>', parser.select_general(cats)



