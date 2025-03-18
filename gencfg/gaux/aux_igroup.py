"""
    Auxiliarily IGroup functions (move this file to igroup folder after refactoring)
"""


def guess_group_location(groupname):
    """
        Guest group location, based on group name
    """
    if groupname.startswith('MSK_'):
        return 'msk'
    elif groupname.startswith('SAS_'):
        return 'sas'
    elif groupname.startswith('MAN_'):
        return 'man'
    elif groupname.startswith('VLA_'):
        return 'vla'
    else:
        return None


def guess_group_dc(groupname):
    """
        Guest group dc, based on group name
    """
    if groupname.startswith('MSK_IVA_'):
        return 'iva'
    elif groupname.startswith('MSK_FOL_'):
        return 'fol'
    elif groupname.startswith('MSK_UGRB_'):
        return 'ugrb'
    elif groupname.startswith('MSK_MYT_'):
        return 'myt'
    else:
        return None
