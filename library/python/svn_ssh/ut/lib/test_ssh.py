import pytest
from library.python import svn_ssh


@pytest.mark.parametrize('input,args,opts,expected', (
    ('', None, [], ''),
    ('ssh', None, ['op1', 'op2'], 'ssh'),
    ('ssh', ['-a1', '-a2'], None, 'ssh'),
    ('ssh -6 op1', None, ['op1', 'op2'], 'ssh -6 op1'),
    ('ssh -6 op1=v1', None, ['op1', 'op2'], 'ssh -6 op1=v1'),
    ('ssh -6 fff -hhh -o op1', None, ['op1', 'op2'], 'ssh -6 fff -hhh'),
    ('ssh -6 fff -hhh -o op1=v1', None, ['op1', 'op2'], 'ssh -6 fff -hhh'),
    ('ssh -o op2=v2 -6 fkfk -fhfhh -o op1=v1', None, ['op1', 'op2'], 'ssh -6 fkfk -fhfhh'),
    ('ssh -a', ['-a'], None, 'ssh'),
    ('ssh -a kkk', ['-a'], None, 'ssh'),
    ('ssh -a kkk -6 -hhh fff', ['-a'], None, 'ssh -6 -hhh fff'),
    ('ssh -a1 kkk -6 -hhh fff -a2', ['-a1', '-a2'], None, 'ssh -6 -hhh fff'),
    ('ssh -a1 kkk -6 -hhh fff -a2 sss', ['-a1', '-a2'], None, 'ssh -6 -hhh fff'),
    ('ssh -a1 kkk -6 -o op1=v1 -hhh fff -a2 sss -o op2', ['-a1', '-a2'], ['op1', 'op2'], 'ssh -6 -hhh fff'),
))
def test_sanitize(input, args, opts, expected):
    assert svn_ssh._sanitize_svn_ssh(input, args, opts) == expected
