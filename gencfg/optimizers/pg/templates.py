#!/usr/local/bin/python
import copy


def process_templates(tree):
    # find templates
    templates = {}

    if tree.find('templates'):
        for template in tree.findall('templates/template'):
            templates[template.find('name').text.strip()] = template.find('data')

    for subtree in tree.getiterator():
        for tt in subtree.findall('template'):
            if tt.text.strip() in templates:
                for child in templates[tt.text.strip()]:
                    if subtree.find(child.tag) is None:
                        subtree.append(copy.deepcopy(child))
    return tree
