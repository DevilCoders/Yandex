import typing as tp
from pathlib import Path
from os.path import join

import vh
from vh.frontend.nirvana import OpPartial
from clan_tools.utils.archive import create_archive
from clan_tools.vh.operations import (extract_op, zip_op, yt_write_file)


def get_package(package_path: str, local_script: bool = True, files_to_copy: tp.Optional[tp.List[str]] = None) -> vh.File:
    """Collects package content to archive. Also adds clan_tools, since it is considered
    as a common library for all Cloud Analytics projects

    :param arcadia_path: Starts after "cloud/analytics/python/". For example "active analytics"
                         Local arcadia folder is asumed mounted to "~/arc/arcadia/".
    :param local_script: Use True for development and False for production, defaults to True
    :param files_to_copy: Files(folders) list relative to package path. Defaults to ['src/', 'scripts/', 'config/'].
                          Use only the most important files.

    :return: tar archive (in common sense)
    """

    local_path = join(Path.home(), 'arc/arcadia/cloud/analytics/')
    if local_script:
        archive_name = join(local_path, package_path, 'deploy.tar')
        package_dir = join(local_path, package_path)
        if files_to_copy is None:
            create_archive(
                package_dir, ['src/', 'scripts/', 'config/'], archive_name)
        else:
            create_archive(package_dir, files_to_copy, archive_name)

        tools_dir = join(local_path, 'python/lib/clan_tools')
        create_archive(tools_dir, ['src/'], archive_name, append=True)
        archive = vh.File(archive_name)
    else:
        package = vh.arcadia_folder(f'cloud/analytics/{package_path}')
        tools = vh.arcadia_folder('cloud/analytics/python/lib/clan_tools')
        merge_archives_op = vh.op(id='31d7bcd4-c5f4-4398-9bf1-070e5466c219')
        archive = merge_archives_op(tar_archives=[package, tools]).tar_archive
    return archive


def prepare_dependencies(package: vh.File, deps_dir: str, drivers: tp.List[str], drivers_local_dir: str = 'scripts') -> tp.List[OpPartial]:
    src = extract_op(archive=package, out_type='binary',
                     path='src').binary_file
    src_zip = zip_op(input=src).output
    result_list = [yt_write_file(
        file=src_zip, path=f'{deps_dir}/dependencies.zip')]
    for driver in drivers:
        driver_file = extract_op(
            archive=package, out_type='exec', path=f'{drivers_local_dir}/{driver}').exec_file
        result_list.append(yt_write_file(
            file=driver_file, path=f'{deps_dir}/{driver}'))
    return result_list
