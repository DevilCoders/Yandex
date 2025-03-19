import cloud.mdb.salt.salt.components.mongodb.conf.mdb_disk_watcher as mdb_disk_watcher

from collections import namedtuple

statvfs_mock_result = namedtuple('statvfs_mock_result', ['f_bavail', 'f_bsize', 'f_blocks'])


class MockMDBMongoDiskWatcher(mdb_disk_watcher.MDBMongoDiskWatcher):
    def __init__(self, test_data_container=None):
        self.test_data_container = {}
        self.init_test_data_container()
        if test_data_container is not None:
            self.test_data_container.update(test_data_container)

        super().__init__()

    def init_test_data_container(self):
        if self.test_data_container.get('mongodb', None) is None:
            self.test_data_container.update({'mongodb': {}})

        if self.test_data_container['mongodb'].get('locked', None) is None:
            self.test_data_container['mongodb'].update({'locked': False})

        if self.test_data_container.get('fs', None) is None:
            self.test_data_container.update({'fs': {}})

        for i in ['space', 'free']:
            if self.test_data_container['fs'].get(i, None) is None:
                self.test_data_container['fs'].update({i: 0})

        if self.test_data_container.get('files', None) is None:
            self.test_data_container.update({'files': {}})

        if self.test_data_container['files'].get('/var/run/mongodb.pid', None) is None:
            self.test_data_container['files']['/var/run/mongodb.pid'] = 1

    def _check_mongodb_is_locked(self, conn=None):
        return self.test_data_container.get('mongodb', {}).get('locked', False)

    def lock_mongodb(self):
        self.test_data_container['mongodb']['locked'] = True

    def unlock_mongodb(self):
        self.test_data_container['mongodb']['locked'] = False

    def is_script_enabled(self):
        return True

    def freeze_mongodb(self):
        self.test_data_container['mongodb']['frozen'] = True

    def unfreeze_mongodb(self):
        self.test_data_container['mongodb']['frozen'] = False

    def _get_pid_from_file(self, path):
        return self.test_data_container['files'].get(path, None)

    def _put_pid_to_file(self, path, pid=None):
        self.test_data_container['files'][path] = pid

    def get_fs_stat(self):
        return statvfs_mock_result(
            self.test_data_container['fs']['free'],
            1,
            self.test_data_container['fs']['space'],
        )

    def get_test_data_container(self):
        return self.test_data_container

    def update_test_data_container(self, k, v):
        self.test_data_container.update(k, v)
