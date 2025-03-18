for __giveup in False, True:
    try:
        from antirobot.scripts.access_log import MakeMap
    except ImportError:
        if __giveup:
            raise

        import sys
        import os

        arcadia_dir = os.getenv('ARCADIA_DIR') or os.path.expanduser('~/arcadia')
        sys.path.append(arcadia_dir + '/antirobot/scripts')

