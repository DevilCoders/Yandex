# coding:utf-8

import pytest
import re

from mkdocs_yandex import regex


INC_CASES = (
    (u'{% include "some/path" %}', True, u'some/path', False, None, ''),
    (u'{% include some/path %}', True, u'some/path', False, None, ''),
    (u'{% include notitle "some/path" %}', True, u'some/path', False, u'notitle', ''),
    (u'{% include notitle some/path %}', True, u'some/path', False, u'notitle', ''),
    (u'{% include keep_indents "some/path" %}', True, u'some/path', True, None, ''),
    (u'{% include keep_indents some/path %}', True, u'some/path', True, None, ''),
    (u'{% include keep_indents notitle "some/path" %}', True, u'some/path', True, u'notitle', ''),
    (u'{% include keep_indents notitle some/path %}', True, u'some/path', True, u'notitle', ''),
    (u'{% include "some/path" lang="yaml" %}', True, u'some/path', False, None, u'lang="yaml" '),
    (u'{% include some/path lang="yaml" %}', True, u'some/path', False, None, u'lang="yaml" '),
    (u'{% include \'some/path\' lang=\'yaml\' lines=\'1-3\'%}', True, u'some/path', False, None, u'lang=\'yaml\' lines=\'1-3\''),
    (u'{% include some/path lang=\'yaml\' lines=\'1-3\'%}', True, u'some/path', False, None, u'lang=\'yaml\' lines=\'1-3\''),
    (u'{% include \'some/path\' lang="yaml" \'lines=1-3\'%}', False, None, False, None, ''),
    (u'{% include \'[title](some/path)\'%}', True, u'[title](some/path)', False, None, ''),
    (u'{% include [title](some/path) %}', True, u'[title](some/path)', False, None, ''),
    (u'{% include [title](some/path)%}\nsome text\nddd', True, u'[title](some/path)', False, None, ''),
    (u'{% include [title](some/path)%}\n{% include [title](some/path/2)%}\nsome text\nddd', True, u'[title](some/path)', False, None, ''),
    (u'{% include "[title](some/path)" %}\n{% include [title](some/path/2)%}\nsome text\nddd', True, u'[title](some/path)', False, None, ''),
    (u'{% include \'[текст](some/path)\'%}', True, u'[текст](some/path)', False, None, ''),
    (u'{% include [текст](some/path) %}', True, u'[текст](some/path)', False, None, ''),
    (u'{% include [текст](some/path)%}\nsome text\nddd', True, u'[текст](some/path)', False, None, ''),
    (u'{% include [текст](some/path)%}\n{% include [текст](some/path/2)%}\nsome text\nddd', True, u'[текст](some/path)', False, None, ''),
    (u'{% include "[текст](some/path)" %}\n{% include [текст](some/path/2)%}\nsome text\nddd', True, u'[текст](some/path)', False, None, ''),
    (u'{% include /some/path %}', True, u'/some/path', False, None, ''),
)


CODE_CASES = (
    (u'{% code "some/path" %}', True, u'some/path', ''),
    (u'{% code some/path %}', True, u'some/path', ''),
    (u'{% code "some/path" lang="yaml" %}', True, u'some/path', u'lang="yaml" '),
    (u'{% code some/path lang="yaml" %}', True, u'some/path', u'lang="yaml" '),
    (u"{% code 'some/path' lang='yaml' lines='1-3'%}", True, u"some/path", u"lang='yaml' lines='1-3'"),
    (u"{% code some/path lang='yaml' lines='1-3'%}", True, u"some/path", u"lang='yaml' lines='1-3'"),
    (u"{% code 'some/path' lang=\"yaml\" 'lines=1-3'%}", False, None, ""),
)

re_inc = re.compile(regex.RE_INC, re.UNICODE)
re_code = re.compile(regex.RE_CODE, re.UNICODE)


@pytest.mark.parametrize('line,matches,path,keep_indents,inc_type,opts', INC_CASES)
def test_inc(line, matches, path, keep_indents, inc_type, opts):
    m = re_inc.match(line)
    if matches:
        assert m is not None, line
    else:
        assert m is None, line
        return

    assert m.group('ref') == path, line
    assert bool(m.group('keep_indents')) == keep_indents, line
    assert m.group('type') == inc_type, line
    assert m.group('opts') == opts, line


@pytest.mark.parametrize('line,matches,path,opts', CODE_CASES)
def test_code(line, matches, path, opts):
    m = re_code.match(line)
    if matches:
        assert m is not None, line
    else:
        assert m is None, line
        return

    assert m.group('ref') == path, line
    assert m.group('opts') == opts, line
