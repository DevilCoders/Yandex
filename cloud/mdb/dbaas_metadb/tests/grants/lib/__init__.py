import re
from pathlib import Path
import logging

SYSTEM_USERS = ['monitor']


def get_grants_users(grants_dir: str) -> set[str]:
    users = (p.name[: -len(p.suffix)] for p in Path(grants_dir).glob('*.sql'))
    return {u for u in users if u not in SYSTEM_USERS}


def get_pillar_users(pillar_path: str) -> set[str]:
    with open(pillar_path) as fd:
        include_section = re.search(r'^include:\n([ ]+-[ \w.{}-]+\n)+', fd.read(), re.MULTILINE)
        if not include_section:
            raise RuntimeError(f'- include section not found in {pillar_path}')
        logging.debug('include_section: %s', include_section.group())
        return set(m.group(2) for m in re.finditer(r'\.pgusers(\.\w+)?\.(\w+)', include_section.group()))
