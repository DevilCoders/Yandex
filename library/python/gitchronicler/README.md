```python
from gitchronicler import chronicler

# version from changelog.md
chronicler.get_current_version()
'0.2.97'

# grouped changes from vcs log
chronicler.get_grouped_changes(from_version='0.2.96')
{
    ('Ivan Ivanov', 'ivanov@yandex-team.ru'): [
        Change(
            author_name='Ivan Ivanov',
            author_email='ivanov@yandex-team.ru',
            message=u'TOOLS-100500 Делал всякое',
            link='https://github.yandex-team.ru/tools/cab/commit/5b64c62',
        )
    ]
}

# get data from vcs config
chronicler.get_maintainer()
('Ivan Ivanov', 'ivanov@yandex-team.ru')

# generate changelog.md record collecting commit from last version
chronicler.get_changelog_record()
"""
0.2.97
------
 * TOOLS-100500 Делал всякое                           [ https://github.yandex-team.ru/tools/cab/commit/27a4e9e ]
 * Typo                                                [ https://github.yandex-team.ru/tools/cab/commit/5b64c62 ]

[Ivan Ivanov](http://staff/ivanov@yandex-team.ru) 2016-11-25 18:49:45+03:00
"""

```
