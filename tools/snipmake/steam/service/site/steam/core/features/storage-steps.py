# -*- coding: utf-8 -*-

from lettuce import *
from nose.tools import assert_equals
from core.actions.storage import Storage
from core.settings import TEMP_ROOT
import os
import tempfile


@before.all
def set_storage():
    world.storage = Storage


@step('Given I have a string "([^"]*)"')
def get_content(step, content):
    world.content = content


@step('When I store it')
def store_content(step):
    handle, tmp_name = tempfile.mkstemp(dir=TEMP_ROOT)
    os.write(handle, world.content)
    os.close(handle)
    world.content_id = world.storage.store_raw_file(tmp_name)
    os.remove(tmp_name)


@step('Then I get back the same content')
def get_same_content(step):
    assert_equals(world.storage.get_raw_file(world.content_id),
                  world.content)
