import json
import os

import requests

import click
from click import command, echo, pass_context
from cloud.mdb.cli.dbaas.internal import vault
from cloud.mdb.cli.common.prompts import numeric_prompt, path_prompt, prompt
from cloud.mdb.cli.dbaas.internal.config import (
    CERTIFICATES,
    DBAAS_TOOL_ROOT_PATH,
    config_option,
    create_profile,
    get_config,
    get_profile_names,
    CONFIG_PRESETS,
    get_environment,
)


@command('init')
@pass_context
def init_command(ctx):
    """Initialize tool."""
    echo('Welcome to initialization process!')

    config = get_config(ctx)
    current_profile_name = config.get('profile')

    options = [
        ('Create a new profile', 'create'),
    ]
    if current_profile_name:
        options.insert(
            0, (f'Re-initialize the current profile ("{current_profile_name}") with new settings', 'recreate')
        )

    mode = numeric_prompt(options, default='create').value

    if mode == 'create':
        _download_certs()
        config_preset = _prompt_config_preset()
        name = _propmt_name(ctx, config_preset=config_preset)
        environment = get_environment(config_preset['environment'])
        auth_config = _prompt_auth(config_preset, environment)
        folder = _prompt_folder(environment)

        create_profile(
            ctx,
            name,
            config_preset=config_preset,
            auth_config=auth_config,
            folder=folder,
        )
        _complete_initialization(f'Profile "{name}" created and activated.')

    elif mode == 'recreate':
        _download_certs()
        config_preset = _prompt_config_preset(current_config_preset_name=config.get('config_preset'))
        environment = get_environment(config_preset['environment'])
        auth_config = _prompt_auth(config_preset, environment, current_auth_config=config.get('iam', {}))
        folder = _prompt_folder(environment, current_folder=config_option(ctx, 'compute', 'folder', None))

        create_profile(
            ctx,
            current_profile_name,
            config_preset=config_preset,
            auth_config=auth_config,
            folder=folder,
        )
        _complete_initialization(f'Profile "{current_profile_name}" updated.')


def _download_certs():
    cert_dir = os.path.expanduser(DBAAS_TOOL_ROOT_PATH)
    os.makedirs(cert_dir, exist_ok=True)
    for cert in CERTIFICATES:
        cert_file = os.path.join(cert_dir, cert['name'])
        if not os.path.exists(cert_file):
            contents = cert.get('contents')
            if not contents:
                response = requests.get(cert['url'])
                response.raise_for_status()
                contents = response.content.decode()

            with open(cert_file, 'w') as f:
                f.write(contents.strip())


def _propmt_name(ctx, config_preset=None):
    default_name = None
    if config_preset:
        default_name = config_preset['environment']

    return prompt(
        'Enter profile name',
        default=default_name,
        blacklist=get_profile_names(ctx),
        blacklist_message='Profile "{value}" already exists.',
    )


def _prompt_config_preset(*, current_config_preset_name=None):
    preset_names = [preset['name'] for preset in CONFIG_PRESETS]
    preset_map = {preset['name']: preset for preset in CONFIG_PRESETS}

    echo('Configuration preset:')
    preset_name = numeric_prompt(preset_names, default=current_config_preset_name).value

    return preset_map[preset_name]


def _prompt_auth(config_preset, env, current_auth_config=None):
    auth_type = _prompt_auth_type(config_preset, current_auth_config)
    if auth_type == 'oauth_token':
        return {'token': _prompt_oauth_token(env, current_auth_config)}
    elif auth_type == 'user_key':
        return {'user_key': _prompt_user_key(current_auth_config)}
    elif auth_type == 'none':
        return {}
    else:
        raise RuntimeError(f'invalid auth type: "{auth_type}"')


def _prompt_auth_type(config_preset, current_auth_config=None):
    auth_type = config_preset.get('auth_type')
    if auth_type:
        return auth_type

    options = [
        ('OAuth token', 'oauth_token'),
        ('User account key', 'user_key'),
    ]

    default = None
    if current_auth_config:
        if current_auth_config.get('token'):
            default = 'oauth_token'
        elif current_auth_config.get('user_key'):
            default = 'user_key'

    echo('Authentication method:')
    return numeric_prompt(options, default=default).value


def _prompt_oauth_token(env, current_auth_config=None):
    url = env.get('oauth_token_url')
    echo(f'Please go to {url} in order to obtain OAuth token.')

    current_token = (current_auth_config or {}).get('token')
    if current_token:
        default = 'current token'
        value = prompt('Enter OAuth token', hide_input=True, default=default)
        return current_token if value == default else value
    else:
        return prompt('Enter OAuth token', hide_input=True)


def _prompt_user_key(current_auth_config=None):
    echo(
        'Please specify user account key created by "yc iam key create --user-account". Detailed instructions'
        ' can be found in https://clubs.at.yandex-team.ru/ycp/3310.'
    )

    current_user_key = (current_auth_config or {}).get('user_key')
    if current_user_key:
        default = 'current user key'
        value = path_prompt('Enter file path', default=default)
        if value == default:
            return current_user_key
    else:
        value = path_prompt('Enter file path')

    with open(os.path.expanduser(value), 'r') as f:
        return json.load(f)


def _prompt_database_credentials(env, current_dsn, dbname, required=True):
    db_config = env.get(dbname)
    if not db_config:
        return None, None

    if current_dsn and not click.confirm(f'Update {dbname} access credentials', default=False):
        return None, None

    user = db_config.get('user')
    if not user:
        if not required and not click.confirm(f'Enable {dbname} access', default=True):
            return None, None

        user = _prompt_dbuser(dbname, db_config['users'])

    user_config = db_config['users'].get(user)
    if user_config:
        secret_id = user_config['secret']
        password = vault.get_secret(secret_id)['password']
    else:
        password = prompt('Enter user password', hide_input=True)

    return user, password


def _prompt_dbuser(dbname, users):
    echo(f'{dbname} user:')

    names = sorted(users.keys())
    for i, name in enumerate(names):
        click.echo(f' [{i + 1}] {name}')

    name = click.prompt('Please specify numeric choice or enter user name', default=1)
    try:
        return names[int(name) - 1]
    except Exception:
        pass
    return name


def _prompt_folder(env, current_folder=None):
    default_folder = env['defaults']['compute'].get('folder')

    url = f'{env["defaults"]["ui"]["url"]}/cloud'
    echo(f'Please go to {url} in order to obtain folder ID.')

    return prompt('Enter folder ID to use as a default', default=(current_folder or default_folder))


def _complete_initialization(message):
    if not os.path.exists(os.path.expanduser(os.path.join(DBAAS_TOOL_ROOT_PATH, 'completion.bash.inc'))):
        message += ' In order to install shell autocompletion, execute `install.sh`.'

    echo(message)
