import yaml
from humanfriendly import parse_timespan


def get_config(
    config_path,
    folder_id: str = None,
    max_age: str = None,
    timeout: str = None,
    oauth_token: str = None,
):
    """
    Read configuration from file and merge it with arguments passed from
    command-line.
    """
    with open(config_path) as file:
        return parse_config_file(
            file,
            folder_id=folder_id,
            max_age=max_age,
            timeout=timeout,
            oauth_token=oauth_token,
        )


def parse_config_file(
    file,
    folder_id: str = None,
    max_age: str = None,
    timeout: str = None,
    oauth_token: str = None,
):
    config = yaml.safe_load(file)
    if folder_id:
        config['folder'] = folder_id

    if max_age:
        config['max_age'] = max_age
    config['max_age_sec'] = int(parse_timespan(config['max_age']))

    if timeout:
        config['timeout'] = timeout
    config['timeout_sec'] = int(parse_timespan(config['timeout']))

    if oauth_token:
        config['internal_api']['oauth_token'] = oauth_token

    if not config.get('folder') and (not config.get('label_key') or not config.get('label_value')):
        raise RuntimeError('Configuration is invalid: folder id ' 'or label key and label value must be set')

    return config
