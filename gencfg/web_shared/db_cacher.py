import os
import shutil
import stat

from contrib.filelock import FileLock, FileLockException


class DBCacher(object):
    def __init__(self):
        self.cache_version = "1"
        self.cache_base_dir = "/var/tmp/gencfg_cache"
        self.cache_dir = os.path.join(self.cache_base_dir, self.cache_version)
        # lock file will be just ".lock"
        self.cache_lock_path = os.path.join(self.cache_base_dir, self.cache_version, "")
        self.lock = FileLock(self.cache_lock_path, timeout=2)

    def __get_tag_cache_path(self, tag):
        return os.path.join(self.cache_dir, tag)

    def try_restore_cache(self, db, tag):
        try:
            self.lock.acquire()
        except FileLockException:
            return False
        except OSError:
            # no such file
            return False

        try:
            path = self.__get_tag_cache_path(tag)
            if not os.path.exists(path):
                return False
            shutil.rmtree(db.get_cache_dir_path())
            shutil.copytree(path, db.get_cache_dir_path())
            return True
        finally:
            self.lock.release()

    def try_save_cache(self, db, tag):
        path = self.__get_tag_cache_path(tag)
        if not os.path.exists(os.path.dirname(path)):
            self.__init_dirs()

        try:
            self.lock.acquire()
        except FileLockException:
            return

        try:
            self.__make_shared(db.get_cache_dir_path())
            if os.path.exists(path):
                shutil.rmtree(path)
            shutil.copytree(db.get_cache_dir_path(), path)
        finally:
            self.lock.release()

    def __init_dirs(self):
        if not os.path.exists(self.cache_base_dir):
            os.mkdir(self.cache_base_dir)
            os.chmod(self.cache_base_dir, self.__all_rwx())
        if not os.path.exists(self.cache_dir):
            os.mkdir(self.cache_dir)
            os.chmod(self.cache_dir, self.__all_rwx())
        if not os.path.exists(self.cache_lock_path):
            with open(self.cache_lock_path, "w") as f:
                f.close()
            os.chmod(self.cache_lock_path, self.__all_rw())

    def __make_shared(self, root):
        for name in os.listdir(root):
            path = os.path.join(root, name)
            mode = os.stat(path).st_mode
            if mode & stat.S_IXUSR:
                new_mode = (mode | self.__all_rwx())
            else:
                new_mode = (mode | self.__all_rw())
            os.chmod(path, new_mode)

            if os.path.isdir(path):
                self.__make_shared(path)

    def __all_rw(self):
        return stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH | stat.S_IWOTH

    def __all_rwx(self):
        return stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO
