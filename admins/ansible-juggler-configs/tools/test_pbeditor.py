#!/usr/bin/env python
# Notes:
# to check test coverage
# > coverage run --source pbeditor.py test_pbeditor.py
# > coverage html && open htmlcov/index.html

import os
import tempfile
import unittest

import ruamel.yaml

import pbeditor


class TestData(object):
    def __init__(self):
        self.test_data = """\
- hosts: localhost
  connection: local
  gather_facts: false
  vars:
    host: test_host
    tags:
    - tag1
    - tag2
"""

    def create_file(self):
        playbook = tempfile.NamedTemporaryFile(delete=False)
        playbook.write(self.test_data)
        playbook.close()
        self.file_name = playbook.name
        return self.file_name

    def delete_file(self):
        if self.file_name is not None:
            os.unlink(self.file_name)


class PlaybookTestCase(unittest.TestCase):
    def test_load_file(self):
        td = TestData()
        file = td.create_file()
        pb = pbeditor.Playbook(file)
        td.delete_file()

        self.assertEqual(pb.content, ruamel.yaml.round_trip_load(td.test_data))

    def test_get_key(self):
        td = TestData()
        file = td.create_file()
        pb = pbeditor.Playbook(file)
        td.delete_file()

        self.assertEqual("test_host", pb.get_key("vars.host"))

    def test_add_key(self):
        td = TestData()
        file = td.create_file()
        pb = pbeditor.Playbook(file)
        td.delete_file()

        pb.add_key("test_key_string", "test_value")
        self.assertEqual("test_value", pb.get_key("test_key_string"))

        for tag, value, expected in [
            ("test_key_list", '["test_value1", "test_value2"]', ["test_value1", "test_value2"]),
            ("vars.host", '["test_host1", "test_host2"]', ["test_host1", "test_host2", "test_host"]),
            ("vars.host", "test_host1", ["test_host", "test_host1"]),
            ("vars.tags", "tag3", ["tag1", "tag2", "tag3"]),
            ("vars.tags", '["tag3", "tag4"]', ["tag1", "tag2", "tag3", "tag4"])
        ]:
            td = TestData()
            file = td.create_file()
            pb = pbeditor.Playbook(file)
            td.delete_file()

            pb.add_key(tag, value)
            self.assertListEqual(expected, pb.get_key(tag))

    def test_remove_key(self):
        td = TestData()
        file = td.create_file()
        pb = pbeditor.Playbook(file)
        td.delete_file()

        pb.remove_key("vars.tags")
        self.assertIsNone(pb.get_key("vars.tags"))

    def test_set_key(self):
        td = TestData()
        file = td.create_file()
        pb = pbeditor.Playbook(file)
        td.delete_file()

        pb.set_key("vars.tags", "tag")
        self.assertEqual("tag", pb.get_key("vars.tags"))


if __name__ == "__main__":
    unittest.main()
