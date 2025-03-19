"""
Implementation of Blackbox methods.
"""

from flask import abort


def oauth(config, args):
    """
    OAuth method.
    """
    check_required_args(args, 'oauth_token')

    return get_or_abort(config['oauth']['tokens'], args['oauth_token'], 'OAuth token "{0}" not found.')


def sessionid(config, args):
    """
    Session ID method.
    """
    check_required_args(args, 'sessionid', 'host')

    data = get_or_abort(config['sessions'], args['sessionid'], 'Session with ID "{0}" not found.')

    # TODO: make sure that returning display_name equaling to login is
    # the correct behavior. Internal API expects this but it contradicts
    # Blackbox documentation (https://doc.yandex-team.ru/blackbox/reference/
    # method-sessionid-response.html)
    data.update(display_name=data['login'])

    return data


def get_or_abort(dictionary, key, message, status=422):
    """
    Return a value with given key from dictionary. If the value not present,
    abort is invoked with provided status and message.
    """
    value = dictionary.get(key)
    if not value:
        abort(status, message.format(key))
    return value


def check_required_args(args, *arg_names):
    """
    Check that required arguments with given names are present in argument
    dictionary. If not, abort is invoked.
    """
    for name in arg_names:
        get_or_abort(args, name, 'The required parameter "{0}" is missing.')
