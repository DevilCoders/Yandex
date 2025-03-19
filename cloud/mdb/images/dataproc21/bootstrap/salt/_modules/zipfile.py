# -*- coding: utf-8 -*-
"""
Module to provide declarative using of zip files
:platform: all
"""

from __future__ import absolute_import

import os
import stat
import shutil
import zipfile
import tempfile

__virtualname__ = 'zipfile'


def __virtual__():
    """
    Determine whether or not to load this module
    """
    return __virtualname__


def _render_diff(diff):
    """
    Render fancy diff for salt states
    """
    return ', '.join(diff)


def objects_present(archive_path, content, dry_run=False):
    """
    Method rewrites files inside zip archive if some the are have
    unexpected values.
    Input:
        archive_path: 'path to zip compatible archives (zip, jar, war)'
        content:
            'config/configs.env': 'path to file with new body'
    """
    changes = []
    if not zipfile.is_zipfile(archive_path):
        raise Exception('File %s is not a zip compatible archive'.format(archive_path))
    to_update, new_objects = {}, {}
    for obj, path in content.items():
        with open(path) as fh:
            to_update[obj] = fh.read()
    # read file to detect changes
    with zipfile.ZipFile(archive_path, 'r') as fh:
        names = fh.namelist()
        new_objects = set(to_update.keys()) - set(names)
        if new_objects:
            changes.append('objects {} would be added'.format(new_objects))
        for obj, data in to_update.items():
            if obj in new_objects:
                continue
            if fh.read(obj) != data:
                changes.append('object {} would be updated'.format(obj))
    if dry_run or not bool(changes):
        return bool(changes), _render_diff(changes)
    # rewrite a whole archive to temporary file
    tmp = tempfile.NamedTemporaryFile(delete=False)
    with zipfile.ZipFile(archive_path, 'r') as src:
        with zipfile.ZipFile(tmp.name, 'w') as dst:
            for name in src.namelist():
                # Do not write directory objects (objects without file size and / on end)
                info = src.getinfo(name)
                if info.file_size == 0 and name.endswith('/'):
                    continue
                data = to_update.get(name)
                if data is None:
                    data = src.read(name)
                dst.writestr(info, data)
            for name in new_objects:
                dst.writestr(name, to_update[name], zipfile.ZIP_DEFLATED)
    # Copy file attributes and move it back
    st = os.stat(archive_path)
    shutil.copystat(archive_path, tmp.name)
    os.chown(tmp.name, st[stat.ST_UID], st[stat.ST_GID])
    shutil.move(tmp.name, archive_path)
    return bool(changes), _render_diff(changes)
