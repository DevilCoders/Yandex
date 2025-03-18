import yt.wrapper as yt

def SetupYT(proxy, token, writer):
    if writer:
        yt.config['pickling']['create_modules_archive_function'] = writer
    if token:
        yt.config['token'] = token
    if proxy:
        yt.config['proxy']['url'] = proxy

    yt.config['read_retries']['allow_multiple_ranges'] = True
    yt.config['read_retries']['count'] = 5
