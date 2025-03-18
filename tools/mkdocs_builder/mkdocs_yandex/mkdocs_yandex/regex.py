"""
Custom tag patterns
"""

from __future__ import absolute_import, unicode_literals

# List {% list <type> %}
RE_LIST = r'''(?x)
        (
        \{%\s*                                                      # Opening
        list\s*                                                     # Tag name
        (?P<type>details|dl|dropdown|radio|tabs)?                   # List type
        \s*%\}                                                      # Closing
        )
        '''
RE_LIST_END = r'(\{%\s*endlist\s*%\})'

# Cut {% cut ["title"] %}
RE_CUT = r'''(?x)
        (
        \{%\s*                                                      # Opening
        cut\s*                                                      # Tag name
        (?P<title>"[^"]*")?                                         # Optional title
        \s*%\}                                                      # Closing
        )
        '''
RE_CUT_END = r'(\{%\s*endcut\s*%\})'

_INC_CODE_COMMON_TAIL = r'''
        (?P<quote>['"])?(?P<ref>[\w\d/\[\]\(\)\.@_-]*)(?P=quote)?\s*     # Link to a file
        (?P<opts>(\w+=(?P<quoteopts>['\"])[^'"]*(?P=quoteopts)\s*)*)     # Optional parameters
        \s*%\}                                                           # Closing
        (?P<after>[^\n]*)
        )
'''

# Include {% include [keep_indents] [notitle] "path/to/file.md | [text](path/to/file.md)" more="options" %}
RE_INC = r'''(?x)
        (
        (?P<before>[^\n]*)
        {%\s*                                                            # Opening
        include\s*                                                       # Tag name
        (?P<keep_indents>keep_indents)?\s*                               # Do not remove indents from inclusion [optional]
        (?P<type>notitle)?\s*                                            # Inclusion type [optional]
''' + _INC_CODE_COMMON_TAIL

# Include code {% code "path/to/file.md | [text](path/to/file.md)" more="options" %}
RE_CODE = r'''(?x)
        (
        (?P<before>[^\n]*)
        {%\s*                                                            # Opening
        code\s*                                                          # Tag name
''' + _INC_CODE_COMMON_TAIL

# Markdown link [text](path/to/file.md "optional title")
RE_MD_LINK = r'''(?x)
        (
        (?<!\!)                                                     # Doesn't start with `!` as it is not image
        \[(?P<text>.*)\]                                            # Text
        \(\s*                                                       # (
        (?P<ref>.*?)                                                # Link
        \s*                                                         #
        (?:(?P<quote>['"])([^'"]*)((?P=quote))\s*)?                 # "Optional title"
        \)                                                          # )
        )
'''

# Link text with modifiers
RE_LINK_TEXT_WITH_OPTS = r'(.*)({\s*#(?P<opts>[A-Z]+)\s*})$'
