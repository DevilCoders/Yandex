"""Common crypto functions"""
import hashlib


def md5(fname):
    """
    http://stackoverflow.com/questions/3431825/generating-an-md5-checksum-of-a-file
    :param fname:
    :return:
    """
    hash_md5 = hashlib.md5()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()
