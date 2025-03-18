import os
from operator import attrgetter
from pathlib import Path
from startrek_client import Startrek

USERAGENT = 'pochemuto'
ATTACHMENTS = Path('attachments')


def main():
    ATTACHMENTS.mkdir(exist_ok=True)
    client = Startrek(useragent=USERAGENT, token=os.environ['AUTH_TOKEN'])
    issues = client.issues.find(filter={'queue': 'CIWELCOME', 'status': 4})

    for issue in issues:
        print(f'Processing {issue.key}')
        last_attachement = max(issue.attachments.get_all(), key=attrgetter('createdAt'))
        path = ATTACHMENTS.joinpath(issue.key)
        path.mkdir(exist_ok=True)
        last_attachement.download_to(path)
        print(f'Downloaded {path}')
    print('Done.')


if __name__ == '__main__':
    main()
