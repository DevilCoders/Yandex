# coding: utf-8

from __future__ import unicode_literals

import sys
import os
import random
import re
import functools
import itertools

import click
import sh

from tools.releaser.src.conf import cfg


NO_DEPLOY_COMMENT = 'no'

TRACKER_TICKET_PATTERN = re.compile(r'[A-Z]{2,}-\d+')


def reprish(value):
    """
    A kind-of `repr()`.

    Made mostly to solve the problems of `u` in `u'...'` in python2.
    """
    try:
        import hjson
    except Exception:
        return repr(value)
    return hjson.dumps(value)


def run_with_output(cmd, *args, **kwargs):
    _dry_run = kwargs.pop('_dry_run', False)
    _sudo = kwargs.pop('_sudo', False)

    echo_pieces = []
    if _dry_run:
        click.secho('[DRY RUN] ', fg='green', bold=True, nl=False)

    if _sudo:
        echo_pieces.append('sudo')

    echo_pieces.append(cmd)
    echo_pieces.extend(args)
    echo_pieces.extend([
        '%s=%s' % (key, val) for key, val in kwargs.items()
    ])
    message = ' '.join(echo_pieces)
    click.secho(message, bold=True)

    if _dry_run:
        return None

    if _sudo:
        sh_cmd = getattr(sh.contrib.sudo, cmd)
    else:
        sh_cmd = sh.Command(cmd)
        kwargs.setdefault('_out', sys.stdout)
        kwargs.setdefault('_err', sys.stderr)
        kwargs.setdefault('_in', sys.stdin)
        kwargs.setdefault('_env', dict(os.environ))

    return sh_cmd(*args, **kwargs)


def echo_workflow_step(message, step_num='*'):
    click.secho('[', fg='green', bold=True, nl=False)
    click.secho(str(step_num), bold=True, nl=False)
    click.secho(']', fg='green', bold=True, nl=False)
    click.secho(' %s' % message, bold=True)


def panic(message):
    click.secho(message, fg='red', bold=True)
    raise click.ClickException(message)


def get_image_path(image_url):
    return image_url[len(cfg.REGISTRY_HOST) + 1:]


def get_image_url(image_name):
    """
    ...

    >>> get_image_url('foo', 'bar')
    'registry.yandex.net/tools/foo:bar'
    >>> get_image_url(cfg.REGISTRY_HOST + '/prj/foo', 'bar')
    'registry.yandex.net/prj/foo:bar'
    """
    if image_name.startswith(cfg.REGISTRY_HOST):
        image_url = image_name
    else:
        image_url = '{host}/{image_path}'.format(
            host=cfg.REGISTRY_HOST,
            image_path='{}/{}'.format(cfg.PROJECT, image_name),
        )
    return image_url


@functools.total_ordering
class Last(object):
    """
    Sorting placeholder that considers itself larger than any other type.
    """

    def __ge__(self, other):
        return True

    def __eq__(self, other):
        if type(other) is type(self):
            return True
        return False

    def __repr__(self):
        return str('LAST')


LAST = Last()


def version_sort_key(version):  # raises TypeError
    """
    Versions sorting key

    See also:

      * https://stackoverflow.com/a/12255578
      * `from packaging.version import parse`

    >>> sample = ["1.7.0", "1.7.0rc2", "1.7.0rc1", "1.7.1", "1.8.extra", "1.11.0"]
    >>> sorted(sample, key=version_sort_key)
    ['1.7.0rc1', '1.7.0rc2', '1.7.0', '1.7.1', '1.8.extra', '1.11.0']
    >>> max(sample, key=version_sort_key)  # actually does all the comparisons, unlike sorted()
    '1.11.0'
    >>> min(sample, key=version_sort_key)
    '1.7.0rc1'
    """
    pieces = re.findall(r'\d+|\D+', version)
    # Tricky point: in python3, comparison between strings and integers is
    # properly disallowed;
    # the above regex parses any string into alternating digits/non-digits;
    # so the comparison will work when all the versions start with a digit.
    # However, it might be useful to wrap the ints into a class that can
    # compare itself to strings.
    pieces = [
        int(piece) if piece.isdigit() else piece
        for piece in pieces]
    # Put the shorter ones to the end:
    pieces = pieces + [LAST]
    pieces = tuple(pieces)
    return pieces


def get_version_from_image_url(image_url):  # raises TypeError, ValueError
    # TODO?: urlparse?
    pieces = image_url.rsplit(':', 1)
    if len(pieces) == 1:
        raise ValueError("No tag in the image_url", image_url)
    tag = pieces[-1]
    # TODO?: parse as version by the current format?
    if not tag:
        raise ValueError("Empty tag in the image_url", image_url)
    return tag


def maybe_download_certificate(dry_run=False):
    CA_DIR, CA_FILE_NAME = cfg.INTERNAL_CA_PATH.rsplit('/', 1)
    CA_DOWNLOADED_FILE_NAME = 'YandexInternalRootCA.crt'
    CA_URL = 'https://crls.yandex.net/%s' % CA_DOWNLOADED_FILE_NAME

    if os.path.exists(cfg.INTERNAL_CA_PATH):
        return

    msg = '\n'.join([
        'Do you want to download Yandex internal CA to %s?' % cfg.INTERNAL_CA_PATH,
        'Read more at https://wiki.yandex-team.ru/security/ssl/sslclientfix/',
    ])
    if not dry_run and not click.confirm(msg):
        return

    if not os.path.exists(CA_DIR):
        run_with_output('mkdir', '-p', CA_DIR, _sudo=True, _dry_run=dry_run)

    run_with_output('curl', '-O', CA_URL, _dry_run=dry_run)
    run_with_output(
        'mv', './' + CA_DOWNLOADED_FILE_NAME, cfg.INTERNAL_CA_PATH,
        _sudo=True, _dry_run=dry_run
    )


def get_oauth_token_or_panic():
    oauth_token = cfg.OAUTH_TOKEN
    if not oauth_token:
        raise ValueError(
            'You need to specify RELEASER_OAUTH_TOKEN environment variable '
            'or oauth_token setting in ~/.release.hjson'
        )
    return oauth_token


def get_full_environment_url(qloudinst, environment_object):
    return '%s/%s' % (cfg.get_qloud_url(qloudinst), environment_object.url)


def _get_changelog_until_version(changelog_records, prev_version, all_if_not_found=False):
    result = []
    for version, changelog_record in changelog_records:
        if version == prev_version:
            return result  # found the `upto`, return the previous collected
        result.append((version, changelog_record))
    if all_if_not_found:
        return result  # return the entire source
    return []  # `prev_version` not found, return nothing.


def get_deploy_comment(
        deploy_comment_format, changelog_records, from_version=None, new_version=None,
        default_version='<unknown>', default_changelog='<null>'):
    if deploy_comment_format == NO_DEPLOY_COMMENT:
        return None

    changelog_records = iter(changelog_records)

    # Note: assuming the `changelog_records` are from newest to the oldest.

    # Make a fallback record for cases when `version` / `from_version` were not found.
    try:
        top_record = next(changelog_records)
    except StopIteration:
        return deploy_comment_format.format(
            version=default_version, changelog=default_changelog)
    # else:
    # Put it back:
    changelog_records = itertools.chain([top_record], changelog_records)

    if new_version:  # skip until 'target' version is found
        new_changelog_record = next(
            (changelog_record
             for version, changelog_record in changelog_records
             if version == new_version),
            None)
        if new_changelog_record is None:
            # `new_version` was not found,
            # `changelog_records` is now empty,
            # fallback to old behaviour.
            changelog_records = iter([top_record])
        else:
            # Tricky point: `changelog_records` is now read up to and including
            # the `new_version` record.
            # put it back (still need new_version's changelog):
            changelog_records = itertools.chain(
                [(new_version, new_changelog_record)],
                changelog_records)

    # Second fallback:
    relevant_records = [next(changelog_records)]
    if from_version:
        # Additionally append records up to `from_version` (not including),
        # but only if `from_version` is actually found.
        relevant_records.extend(_get_changelog_until_version(changelog_records, from_version))

    relevant_records_processed = []
    for idx, (version, changelog_record) in enumerate(relevant_records):
        # # Убираем две последние строки с датой и выпускающим версию.
        # # ... может выреать лишнего.
        # # TODO: Требуется более приличное решение.
        # # https://github.yandex-team.ru/tools/releaser/issues/131
        # changelog_record = changelog_record.rsplit('\n', 3)[0]
        if idx > 0:
            changelog_record = '{}:\n{}'.format(version, changelog_record)
        relevant_records_processed.append((version, changelog_record))

    relevant_records = relevant_records_processed

    version = None
    if relevant_records:
        version = relevant_records[0][0]
    version = version or default_version
    changelog = '\n\n'.join(changelog_record for version, changelog_record in relevant_records)
    changelog = changelog or default_changelog

    return deploy_comment_format.format(
        version=version,
        changelog=changelog,
    )


def parse_ticket_key(string):
    match = TRACKER_TICKET_PATTERN.search(string)
    if not match:
        return None
    return match.group(0)


try:
    from shlex import quote as sh_quote
except ImportError:

    # This is a minimally extended version of the python3's `shlex.quote` for use in python2, copied from
    # https://github.yandex-team.ru/statbox/python-sbdutils/blob/1eb739231088a19b17f5fa58870ae5224b801a25/sbdutils/base.py#L1068-L1101
    # to avoid additional dependencies.

    _sh_find_unsafe = re.compile(r"[^\w@%+=:,./-]").search

    def sh_quote(string, quote_method=1, **kwargs):
        r"""
        Quote a value for copypasteability in a posix commandline.

        Py3.3's shlex.quote piece backport, extended.

        >>> sh_quote("'one's one'", quote_method=2)
        "\\''one'\\''s one'\\'"
        """
        if not string:
            return "''"
        if _sh_find_unsafe(string) is None:
            return string

        # Substring that is entirely useless at either end.
        extraneous_at_sides = "''"

        if quote_method == 1:
            # use single quotes, and put single quotes into double quotes
            # the string `'$'b'` is then quoted as `"'"'$'"'"'b'"'"`
            quoted_quote = "'\"'\"'"
        elif quote_method == 2:
            # the string `'$'b'` is quoted as `\''$'\''b'\'`
            quoted_quote = r"'\''"
        else:
            raise ValueError("No such method")
        result = "'{}'".format(string.replace("'", quoted_quote))

        # Remove the possible quoted empty strings at the ends.
        if result.startswith(extraneous_at_sides):
            result = result[len(extraneous_at_sides):]
        if result.endswith(extraneous_at_sides):
            result = result[:-len(extraneous_at_sides)]
        return result


def sh_join_simple(args, sep=' '):
    return ' '.join(sh_quote(arg) for arg in args)


def sh_join(args, pre_args=None, shellwrap=False, shellwrap_ext=False, sep=' '):
    if not args and not pre_args:
        return ''
    pre_args = list(pre_args or [])
    if shellwrap_ext:
        shellwrap = True
        args = list(args)
        while args:
            arg = args[0]
            if arg == '--':
                args.pop(0)
            elif arg.startswith('-'):
                pre_args.append(args.pop(0))
            else:
                break
    pre_cmd = sh_join_simple(pre_args, sep=sep)
    cmd = sh_join_simple(args, sep=sep)
    if shellwrap and cmd:
        cmd = sh_quote(cmd)
    return '{}{}{}'.format(
        pre_cmd,
        sep if cmd and pre_cmd else '',
        cmd)


def parse_components(text):
    """
    Парсит сисок компонетов, разделенных через запятую.
    Для каждого компонента, в скобках может быть указано в каких DC и сколько
    инстансов поднять. Пример:

        backend(IVA=1,MYT=1),front(MOSCOW=1)

    Возвращает словарь - component -> options
    Если опции не заданы, то они будут пустым словарём.

    >>> components = parse_components('backend(IVA=1,MYT=1),front(MOSCOW=1),etc')
    >>> components == dict(backend=dict(IVA='1', MYT='1'), front=dict(MOSCOW='1'), etc=dict()) or components
    True
    >>> components = parse_components('api,celery')
    >>> components == dict(api={}, celery={}) or components
    True
    >>> parse_components('wrong=there(,here')
    Traceback (most recent call last):
        ...
    ValueError: Failed to parse options from 'wrong=there(,here'
    >>> parse_components('wrong=inside(here,there=1)')
    Traceback (most recent call last):
        ...
    ValueError: Failed to parse options values in 'here'
    >>> parse_components('')
    {}
    """
    # ... might as well do full `ast.parse(text)` to support more cases.
    if not text:
        return {}
    text = text.strip()
    if not text:
        return {}
    pieces = re.split(', *', text)

    result = {}

    while pieces:
        piece = pieces.pop(0)
        if '(' not in piece:
            result[piece] = {}
            continue
        name, opts_piece = piece.split('(', 1)
        opts_pieces = [opts_piece]
        while not opts_pieces[-1].endswith(')'):
            if not pieces:
                raise ValueError("Failed to parse options from {!r}".format(text))
            opts_pieces.append(pieces.pop(0))
        assert opts_pieces[-1][-1] == ')'
        opts_pieces[-1] = opts_pieces[-1][:-1]
        opts_res = {}
        for opt in opts_pieces:
            opt_pieces = re.split(' *= *', opt, 1)
            if len(opt_pieces) != 2:
                raise ValueError("Failed to parse options values in {!r}".format(opt))
            opt_key, opt_val = opt_pieces
            opts_res[opt_key] = opt_val
        result[name] = opts_res

    return result


def ssh_to_random_host(host_urls, shellwrap, dry_run, cmd_args, command, verbose=True):
    if command:
        if cmd_args:
            raise click.UsageError("`--command` is mutually exclusive with the command in additional arguments.")
        cmd_args = [command]  # should generally work, but maybe perhaps needs to be shellsplit.

    if verbose:
        click.echo('Running instances:\n{}\n'.format('\n'.join(host_urls)), err=True)

    random_host = random.choice(host_urls)
    if verbose:
        click.echo('Selected host: {}'.format(random_host), err=True)

    cmd_base = ['ssh', random_host]
    cmd_string = sh_join(cmd_args, pre_args=cmd_base, shellwrap_ext=shellwrap)

    if dry_run:
        run_with_output(cmd_string, _dry_run=dry_run)
    else:
        os.system(cmd_string)


def pssh(host_urls, shellwrap, dry_run, pssh_cmd, cmd_args, verbose=True):
    """

    Пример use-case для shellwrap:

        $ releaser pssh -s -- -v -P -- python -c 'import sys; print(sys.path)'

    Без этого флага квотирование приходится делать руками:

       $ releaser pssh -- -v -P -- "python -c 'import sys; print(sys.path)'"

    что может быть крайне неудобно когда кавычек становится больше разных.
    """
    if verbose:
        click.echo('Running instances:\n{}\n'.format('\n'.join(host_urls)), err=True)

    cmd_base = [pssh_cmd]
    for host in host_urls:
        cmd_base += ['-H', host]
    cmd_string = sh_join(cmd_args, pre_args=cmd_base, shellwrap_ext=shellwrap)

    if dry_run:
        run_with_output(
            cmd_string, _dry_run=dry_run)
    else:
        os.system(cmd_string)
