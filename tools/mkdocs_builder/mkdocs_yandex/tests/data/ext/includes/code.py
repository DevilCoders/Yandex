import os


def create_tarball_package(result_dir, package_dir, package_name, package_version, compress=True, codec=None):
    archive_file = '.'.join([package_name, package_version, 'tar'])

    if compress and not codec:
        archive_file += '.gz'

    with exts.tmp.temp_dir() as temp_dir:
        tar_archive = os.path.join(temp_dir, archive_file)
