import cloud.mdb.salt.salt.components.mongodb.conf.mdb_mongodb_deadlock_detector as mdb_mongodb_deadlock_detector

import copy
import logging


class FailCheck(mdb_mongodb_deadlock_detector.Check):
    name = 'fail'

    def check(self, _):
        return False


class PassCheck(mdb_mongodb_deadlock_detector.Check):
    name = 'pass'

    def check(self, _):
        return True


class SetRetAction(mdb_mongodb_deadlock_detector.Action):
    name = 'set_action'

    def run(self, helpers):
        helpers['self']['test_data_container'].update(
            {
                'action': True,
            }
        )


def initMockMongoDeadlockDetectorConfig(test_data_container=None, config={}):
    ret = {
        'config': copy.deepcopy(config),
        'test_data_container': {},
        'checks': {
            'fail': FailCheck,
            'pass': PassCheck,
        },
        'actions': {
            'set_action': SetRetAction,
        },
        'helpers': {},
    }
    ret['config']['log'] = logging.getLogger('test')

    if test_data_container is not None:
        ret['test_data_container'].update(test_data_container)

    def return_self(*args, **kwargs):
        return ret

    ret['helpers']['self'] = return_self

    return ret
