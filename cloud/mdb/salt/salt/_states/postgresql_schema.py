#!/usr/bin/env python
# -*- coding: utf-8 -*-

try:
    # Import salt module, but not in arcadia tests
    from salt.exceptions import CommandExecutionError, CommandNotFoundError
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}


def __virtual__():
    if 'postgresql_schema.info' in __salt__:
        return 'postgresql_schema'
    else:
        return False


def applied(
    name,
    base='/usr/local/yandex',
    target=None,
    baseline=None,
    termination_interval=None,
    callbacks={},
    runas='postgres',
    conn=None,
    session=None,
    noop=False,
):
    ret = {'name': name, 'result': None, 'comment': '', 'changes': {}}
    if noop:
        ret['comment'] = 'no operation'
        ret['result'] = True
        return ret

    try:
        info = __salt__['postgresql_schema.info'](
            base=base, dbname=name, baseline=baseline, target=target, runas=runas, conn=conn, session=session
        )
    except (CommandNotFoundError, CommandExecutionError) as err:
        ret['result'] = False
        ret['comment'] = 'Error in apply: {0!r}: {1}'.format(name, err)
        return ret
    else:
        has_pending = False
        pending_migrations = {}
        applied_migrations = {}
        for version in info:
            if info[version]['installed_on']:
                applied_migrations[version] = info[version]
            else:
                has_pending = True
                pending_migrations[version] = info[version]

    if __opts__['test']:
        if has_pending:
            ret['result'] = None
            ret['comment'] = 'Not applied some migrations'
            ret['changes'] = {'old': applied_migrations, 'new': pending_migrations}
        else:
            ret['result'] = True
            ret['comment'] = 'No pending migrations'

        return ret

    if has_pending:
        try:
            res = __salt__['postgresql_schema.migrate'](
                base=base,
                dbname=name,
                baseline=baseline,
                termination_interval=termination_interval,
                target=target,
                callbacks=callbacks,
                runas=runas,
                conn=conn,
                session=session,
            )
        except (CommandNotFoundError, CommandExecutionError) as err:
            ret['result'] = False
            ret['comment'] = 'Error in apply: {0!r}: {1}'.format(name, err)
            ret['changes'] = {}
            return ret

        ret['result'] = res['result']
        ret['comment'] = res['steps']
        if ret['result']:
            ret['changes'] = {'old': applied_migrations, 'new': pending_migrations}
    else:
        ret['result'] = True
        ret['comment'] = 'No pending migrations'

    return ret
