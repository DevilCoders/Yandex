#!/usr/bin/env python3


import os
import yaml


def read_content_of_suplementary_files(files_locations, logger=None):
    files_contents = dict()
    for file_path in files_locations:
        if logger:
            logger.debug("Trying to open target file: {}".format(file_path))
        if os.path.isfile(file_path):
            if logger:
                logger.debug("Reading target file: {}".format(file_path))
            with open(file_path, "r") as file_object:
                try:
                    file_contents = yaml.load(file_object)
                except Exception as message:
                    logger.error(
                        "This target file has inpropper content format and will be ignored, it should correspond to yaml format: {}".format(
                            file_path
                        )
                    )
                    continue
            if file_contents is None:
                if logger:
                    logger.error("Target file is empty: {}".format(file_path))
            elif type(file_contents) is not dict:
                if logger:
                    logger.error(
                        "This target file content has inpropper content format and will be ignored, "
                        "it should correspond to yaml representation of dict: {}".format(
                            file_path
                        )
                    )
            else:
                files_contents.update(file_contents)
                if logger:
                    logger.debug(
                        "Content of target files is fetched: {}".format(file_path)
                    )
        else:
            if logger:
                logger.debug("This target  file does not exist: {}".format(file_path))
    if not files_contents:
        if logger:
            logger.info(
                "No information could be fetched from relevant files. Only backup values will be used(if any)"
            )
    return files_contents
