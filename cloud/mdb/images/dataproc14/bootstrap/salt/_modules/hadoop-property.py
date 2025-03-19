# -*- coding: utf-8 -*-
"""
Module to provide declarative using site properties
:platform: all
"""

from __future__ import absolute_import

import os
import os.path
import stat
import shutil
import logging
import tempfile

log = logging.getLogger(__name__)

try:
    import xml.etree.ElementTree as ET
    import difflib
    from io import StringIO
    MODULES_OK = True
except ImportError:
    MODULES_OK = False

__virtualname__ = 'hadoop-property'


def __virtual__():
    """
    Determine whether or not to load this module
    """
    if MODULES_OK:
        return __virtualname__
    return False


def __xml_indent(elem, level=0):
    """
    Hack for indenting xml without external dependecies
    """
    i = "\n" + level*"    "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "    "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            __xml_indent(elem, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i


def __parse_property(properties, property_name):
    value, desc, append = None, None, False
    if isinstance(properties[property_name], dict):
        value = str(properties[property_name]['value'])
        desc = properties[property_name].get('desc', None)
        append = properties[property_name].get('append', False)
    else:
        value = str(properties[property_name])
    return value, desc, append


def __atomic_write(path, data):
    if os.path.exists(path):
        fh = tempfile.NamedTemporaryFile(delete=False)
        st = os.stat(path)
        fh.write(data)
        fh.flush()
        fh.close()
        shutil.copystat(path, fh.name)
        os.chown(fh.name, st[stat.ST_UID], st[stat.ST_GID])
        os.rename(fh.name, path)
    else:
        with open(path, 'w') as fh:
            fh.write(data)


def config_site_properties_present(
        config_path,
        properties,
        dry_run=False,
        splitter=',',
        joiner=','):
    """
    Method rewrites properties in *-site.xml configs if
    they are have not expectable values.
    Uses for configs like hive-site.xml, hdfs-site.xml, yarn-site.xml.
    Forward append option for concatinating values.
    Input:
        properties -- dict of
            {
                property_name --> property_value
            }
                or
            {
                value -> property_value (required),
                desc -> property_description (optional, default: None),
                append --> boolean (optional, default: false)
            }
    """
    has_changes = False

    # Update existing properites
    try:
        tree = ET.parse(config_path)
        root = tree.getroot()
        config_before = ET.tostring(root, encoding='utf-8').decode()
    except IOError:
        tree = ET.ElementTree(ET.fromstring('<configuration></configuration>'))
        root = tree.getroot()
        config_before = ''
    new_props = set(properties.keys())
    current_props = set()
    for child_node in root.iter('property'):
        name_node = child_node.find('name')
        value_node = child_node.find('value')
        desc_node = child_node.find('desc')
        # If we found property-node without name-node, than just skip
        if name_node is None:
            continue
        current_props.add(name_node.text)
        if name_node.text in new_props:
            # Get arguments for property_name in current name xml-node
            p_name = name_node.text
            p_value, p_desc, p_append = __parse_property(properties, p_name)
            # Create new xml-node <value> if it doesn't exists
            if value_node is None:
                value_node = ET.Element('value')
                child_node.append(value_node)
            # Replace property value if it not iterable
            if not p_append and value_node.text != p_value:
                value_node.text = p_value
                has_changes = True
            if p_append:
                # If property value is iterable,
                # than try to find value in existing values, and add to the end
                current_values = \
                    [x.strip() for x in value_node.text.split(splitter)]
                new_values = []
                for p_value_item in p_value.split(splitter):
                    if p_value_item not in current_values:
                        new_values.append(p_value_item)
                if new_values:
                    value_node.text = \
                        str(joiner.join(current_values + new_values))
                    has_changes = True
            # Replace description xml-node if we get new one
            if p_desc:
                if desc_node is None:
                    desc_node = ET.Element('description')
                    child_node.append(desc_node)
                if desc_node.text != p_desc:
                    desc_node.text = p_desc
                    has_changes = True
    # Append new properties
    for p_name in new_props - current_props:
        child_node = ET.Element('property')
        name_node = ET.SubElement(child_node, 'name')
        name_node.text = p_name

        p_value, p_desc, p_append = __parse_property(properties, p_name)
        if p_value:
            value_node = ET.SubElement(child_node, 'value')
            value_node.text = str(p_value)
            has_changes = True
        if p_desc:
            desc_node = ET.SubElement(child_node, 'description')
            desc_node.text = str(p_desc)
            has_changes = True
        if has_changes:
            root.append(child_node)

    __xml_indent(root)
    config_after = ET.tostring(root, encoding='utf-8').decode()
    difference = difflib.unified_diff(
        config_before.splitlines(),
        config_after.splitlines())
    if not dry_run and has_changes:
        __atomic_write(config_path, ET.tostring(root, encoding='utf-8'))
    return (has_changes, '\n'.join(difference))
