#! /usr/bin/env python3


import os
import pandas
from zipfile import ZipFile

from datetime import datetime
from xml.etree import ElementTree


def check_timestamp_in_dump_file_on_local_filesystem(file_path, logger=None):
    dump_file_timestamps = dict()
    if logger:
        logger.debug("Parsing file: {}".format(file_path))
    if os.path.isfile(file_path):
        windows_1251_encoding = ElementTree.XMLParser(encoding="cp1251")
        tree = ElementTree.parse(file_path, parser=windows_1251_encoding)
        root = tree.getroot()
        dump_file_timestamps["update_timestamp"] = pandas.to_datetime(
            root.attrib["updateTime"], utc=True
        )
        dump_file_timestamps["urgent_update_timestamp"] = pandas.to_datetime(
            root.attrib["updateTimeUrgently"], utc=True
        )
        if logger:
            logger.debug(
                "Update timestamp of local file {} is {}".format(
                    file_path, dump_file_timestamps["update_timestamp"].isoformat()
                )
            )
            logger.debug(
                "Urgent update timestamp of local file {} is {}".format(
                    file_path,
                    dump_file_timestamps["urgent_update_timestamp"].isoformat(),
                )
            )
            logger.debug("Parsing of file  {} is finished".format(file_path))
    else:
        dump_file_timestamps["update_timestamp"] = datetime.fromtimestamp(0)
        dump_file_timestamps["urgent_update_timestamp"] = datetime.fromtimestamp(0)
        logger.debug("Local file {} could not be found".format(file_path))
    return dump_file_timestamps


def get_register_dump_file_from_zip_archive(
    dump_file_path, zip_archive_content, logger=None
):
    if logger:
        logger.info("saving archive file of local filesystem...")
    zip_archive_file_path = "/tmp/rkn.zip"
    with open(zip_archive_file_path, "wb") as zip_archive_file:
        zip_archive_file.write(zip_archive_content)
    if logger:
        logger.info("done")
    if logger:
        logger.debug("archive file saved at: {}".format(zip_archive_file_path))
    unzip(zip_archive_file_path, dump_file_path, logger=logger)


def unzip(zip_archive_file_path, dump_file_path, logger=None):
    if logger:
        logger.info("extracting dump file from zip archive...")
    with ZipFile(zip_archive_file_path, "r") as zip_archive_file:
        if logger:
            logger.debug(
                "file {} is going to be extracted to {}".format(
                    os.path.basename(dump_file_path), os.path.dirname(dump_file_path)
                )
            )
        zip_archive_file.extract(
            os.path.basename(dump_file_path), os.path.dirname(dump_file_path)
        )
    if logger:
        logger.info("done")
    if logger:
        logger.debug(
            "extracted: {} file with {} bytes size".format(
                dump_file_path, os.path.getsize(dump_file_path)
            )
        )
