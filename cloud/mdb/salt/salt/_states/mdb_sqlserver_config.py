"""
State to ensure Microsoft SQL Server configuration compliance
"""

from __future__ import absolute_import, print_function, unicode_literals
import collections
import salt.utils.odict

OPT_ALIASES = {
    'audit_level': 'AuditLevel',
    'in_doubt_xact_resolution': 'in-doubt xact resolution',
    'xp_cmdshell': 'xp_cmdshell',
    'fill_factor_percent': 'fill factor (%)',
}


def __virtual__():
    """
    Only load if the mdb_sqlserver module is present
    """
    return 'mdb_sqlserver.user_role_list' in __salt__


def __opts_parse(name):
    """ """
    if name in OPT_ALIASES:
        opt = OPT_ALIASES[name]
    else:
        opt = name.replace('_', ' ')
    return opt


def spconfigure_comply(name, opts, **kwargs):
    """
    state checks settings in sys.configurations and
    windows registry and adjusts those are not in compliance with opts dictionary
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    # get the current config
    config = __salt__['mdb_sqlserver.spconfigure_get'](**kwargs)
    # settings that needs to be adjusted
    tochange = {}
    # settings that we failed to adjust or those are ambigous
    notchanged = {}

    # if we failed to get config from server
    if not config:
        ret['result'] = False
        ret['comment'] = 'Failed to get instance current configuration'
        return ret

    # dict form of current config. supplimentaly used.
    dconfig = {r[0]: r[1] for r in config}

    if opts is None:
        opts = {}

    opts = {__opts_parse(opt): val for opt, val in opts.items()}

    if opts.get('sqlcollation'):
        del opts['sqlcollation']

    for opt, val in opts.items():
        if type(opts[opt]) == bool:
            opts[opt] = int(val)
            val = int(val)
        val2 = dconfig.get(opt)
        if str(val2) != str(val):
            tochange[opt] = val
        if opt not in dconfig:
            # then we dont know how to change it or it is ambigous
            notchanged[opt] = val
    if notchanged:
        ret['comment'] = 'Ambigous options detected:' + ','.join(notchanged)
        ret['result'] = False
        return ret
    # if we have some job to do
    if tochange:
        if __opts__['test']:
            ret['result'] = None
            ret['comment'] = 'sp_configure drift detected.'
            ret['changes'][name] = 'Present'
            return ret
        # try doing some changes
        config_altered = __salt__['mdb_sqlserver.spconfigure_set'](tochange, **kwargs)
        # if we succeed
        if config_altered:
            # first thing is to check if we actually did something.
            # doublecheck the config
            config2 = __salt__['mdb_sqlserver.spconfigure_get'](**kwargs)
            # yup, this might happen too
            if not config2:
                ret['result'] = False
                ret['comment'] = 'Failed to check instance current configuration after modification'
                return ret
            dconfig = {r[0]: r[1] for r in config2}
            # comparing what we should have with what we've got
            for opt, val in opts.items():
                val2 = dconfig.get(opt)
                # if we still have an option with a different value
                if str(val2) != str(val):
                    notchanged[opt] = val

            if notchanged:
                ret['result'] = False
                ret['comment'] = (
                    'Failed to apply some options for unknown reason. Change command succeded but options remain the same: '
                    + ','.join(notchanged)
                )
                return ret
            ret['result'] = True
            ret['comment'] = 'sp_configure options altered.'
            for change, val in tochange.items():
                ret['changes'][change] = val
            return ret
        else:
            ret['result'] = False
            ret['comment'] = 'Options modification failed'
            return ret
    else:
        ret['comment'] = 'sp_configure fully complies.'
        return ret
