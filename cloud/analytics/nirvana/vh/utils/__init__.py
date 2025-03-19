import gzip
import os
import tarfile
import tempfile


def tar(files, basedir, prefix='yql_files_'):
    """
    Return path to tar archive.
    """

    def filter(tarinfo):
        tarinfo.mtime = 0
        tarinfo.uname = tarinfo.gname = 'root'
        tarinfo.uid = tarinfo.gid = 0
        return tarinfo

    with tempfile.NamedTemporaryFile(mode='wb', prefix=prefix, delete=False) as f:
        with gzip.GzipFile(filename='data.tar', fileobj=f, mode='wb', mtime=0) as gzipf:
            with tarfile.open(mode='w|', fileobj=gzipf) as tarf:
                for path in files:
                    tarf.add(os.path.join(basedir, path), arcname=path, filter=filter)

    return f.name
