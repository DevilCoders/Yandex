#!/usr/bin/env python3


import hashlib
import shutil
import os

from datetime import datetime
from pytz import timezone
from zipfile import ZipFile, ZIP_DEFLATED


from .base import file_resource


def perform_local_copy(src_files_locations, dst_files_locations, logger=None):
    if logger:
        logger.info("Performing local copy of files")
    copied_files = set()
    src_files, file_names = get_file_list(src_files_locations, logger=logger)
    dst_files, _ = get_file_list(dst_files_locations, logger=logger)
    for file_name in file_names:
        to_copy_or_not_to_copy = make_copy_decision(
            file_name, src_files, dst_files, logger=logger
        )
        if to_copy_or_not_to_copy:
            for dst_file in dst_files_locations:
                if file_name in dst_file:
                    shutil.copy2(src_files[file_name].file_path, dst_file)
            copied_files.add(file_name)
    if logger:
        logger.info("Process of local copy is finished")
    return copied_files


def enlist_local_files_and_calculate_hashes(files_locations, logger=None):
    if logger:
        logger.info("buiding list of files and hashes ...")
    existing_files, file_names = get_file_list(files_locations, logger=logger)
    if logger:
        logger.info("buiding list of files and hashes is completed")
    local_files_with_hashes = {
        "existing_files": existing_files,
        "file_names": file_names,
    }
    return local_files_with_hashes


def get_file_list(file_locations, logger=None):
    file_names = set()
    existing_files = dict()
    for file_path in file_locations:
        file_name = os.path.basename(file_path)
        if os.path.isfile(file_path):
            if logger:
                logger.debug(
                    "starting calculate hash for local file: {}".format(file_path)
                )

            file_hash = calculate_file_md5_hash(file_path, logger)
            modification_date = datetime.fromtimestamp(
                os.path.getmtime(file_path), tz=timezone("Europe/Moscow")
            )
            existing_files[file_name] = file_resource(
                file_name, file_path, file_hash, modification_date
            )
        else:
            logger.debug(
                "local file ({}) does not present, can't calculate hash for it".format(
                    file_path
                )
            )
        file_names.add(file_name)
    return existing_files, file_names


def calculate_file_md5_hash(file_path, logger=None):
    blocksize = 65536
    hasher = hashlib.md5()
    if logger:
        logger.debug("opening file: {}".format(file_path))
    with open(file_path, "rb") as file_to_be_hashed:
        if logger:
            logger.debug("hashing window is: {}".format(blocksize))
        buffer = file_to_be_hashed.read(blocksize)
        while len(buffer) > 0:
            hasher.update(buffer)
            buffer = file_to_be_hashed.read(blocksize)
    return hasher.hexdigest()


def make_copy_decision(file_name, src_files, dst_files, logger=None):
    to_copy_or_not_to_copy = True
    if file_name in src_files and file_name in dst_files:
        if (
            src_files[file_name].modification_date
            <= dst_files[file_name].modification_date
        ):
            if logger:
                logger.debug(
                    "Source file ({}) modification date ({}) less or equal than destination file ({}) modification date ({})\n"
                    "Destination file is up to date".format(
                        src_files[file_name].file_path,
                        src_files[file_name].modification_date,
                        dst_files[file_name].file_path,
                        dst_files[file_name].modification_date,
                    )
                )
            to_copy_or_not_to_copy = False
        else:
            if logger:
                logger.debug(
                    "Source file ({}) modificattion date ({}) more than destination file ({}) modification date ({})\n"
                    "Destination file seems to be outdated. Replacing destination file with local file".format(
                        src_files[file_name].file_path,
                        src_files[file_name].modification_date,
                        dst_files[file_name].file_path,
                        dst_files[file_name].modification_date,
                    )
                )
    elif file_name in src_files:
        if logger:
            logger.debug(
                "Destination directory does not contain copy of {} ".format(
                    src_files[file_name].file_path
                )
            )
            logger.debug(
                "Copying local file({}) to destination directory...".format(
                    src_files[file_name].file_path
                )
            )
    elif file_name not in src_files and file_name in dst_files:
        if logger:
            logger.debug(
                "Respective source file does not exist. Destination file ({}) will not be overriden".format(
                    dst_files[file_name].file_path
                )
            )
        to_copy_or_not_to_copy = False
    else:
        if logger:
            logger.debug(
                "source and destination file relevant to ({}) does not exist".format(
                    file_name
                )
            )
        to_copy_or_not_to_copy = False
    return to_copy_or_not_to_copy


def archive_files(files_locations, logger=None):
    archived_files = list()
    if logger:
        logger.info("archiving files ...")
    for file_path in files_locations:
        archived_files.append(zip_file(file_path, logger=logger))
    if logger:
        logger.info("files archiving is finished")
    return archived_files


def zip_file(file_path, logger=None):
    zip_archive_file_path = file_path + ".zip"
    with ZipFile(zip_archive_file_path, "w", ZIP_DEFLATED) as zip_archive_file:
        if logger:
            logger.debug(
                "file {} is going to be added to {} archive file".format(
                    os.path.basename(file_path), os.path.dirname(zip_archive_file_path)
                )
            )
        zip_archive_file.write(file_path)
    if logger:
        logger.info("done")
    return zip_archive_file_path


if __name__ == "__main__":
    files_locations = [
        "/var/opt/yc/rkn/config_node/react.rules",
        "/var/opt/yc/rkn/config_node/bird.conf.static_routes",
    ]
    local_files_with_hashes = enlist_local_files_and_calculate_hashes(files_locations)[
        "existing_files"
    ]["bird.conf.static_routes"]
    print(vars(local_files_with_hashes))
