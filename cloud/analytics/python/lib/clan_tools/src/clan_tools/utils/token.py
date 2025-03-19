from os.path import expanduser, join


def file2str(path: str) -> str:
    token = None
    with open(path, 'r') as tokenfile:
        token = tokenfile.read().strip('\n')
    return token


def get_token(token_file_path: str = '.yt/token', home_dir: str = '~') -> str:
    '''Reads token from file'''
    home = expanduser(home_dir)
    token = file2str(join(home, token_file_path))
    return token

__all__ = ['file2str', 'get_token']
