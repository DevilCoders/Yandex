#! /usr/bin/env python3


class file_resource:
    def __init__(self, file_name, file_path, file_hash, modification_date):
        self.file_name = file_name
        self.file_path = file_path
        self.file_hash = file_hash
        self.modification_date = modification_date
