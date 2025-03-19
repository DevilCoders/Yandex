
import tarfile
import os
from typing import List
import logging
from zipfile import ZipFile

logger = logging.getLogger('archive')


def create_archive(home_dir: str, files: List[str], archive_name: str, append: bool = False,  verbose: bool = False, format: str = 'tar') -> None:
    if format=='tar':
        with tarfile.open(archive_name, "a" if append else "w") as tar:
            for file in files:
                filepath = os.path.join(home_dir, file)
                if verbose:
                    logger.debug(f'Add {filepath} to {archive_name}')
                tar.add(filepath, arcname=file)
    elif format=='zip':
        with ZipFile(archive_name, "a" if append else "w") as zip_f:
            for file in files:
                filepath = os.path.join(home_dir, file)
                for folderName, _, filenames in os.walk(filepath):
                    for filename in filenames:
                        # create complete filepath of file in directory
                        filePath = os.path.join(folderName, filename)
                        # Add file to zip
                        if verbose:
                            logger.debug(f'Add {filePath} to {archive_name}')
                        zip_f.write(filePath, arcname=os.path.relpath(filePath, start=home_dir))

__all__ = ['create_archive']
