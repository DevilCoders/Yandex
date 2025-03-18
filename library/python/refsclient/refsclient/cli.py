# -*- encoding: utf-8 -*-
import logging
import sys
import warnings

import click

from refsclient import Refs, VERSION_STR, HOST_TEST
from refsclient.exceptions import RefsClientException
from refsclient.utils import configure_logging


HOST = None  # type: str
TIMEOUT = None  # type: int
LANG = None  # type: str


def get_refs():
    refs = Refs(host=HOST, timeout=TIMEOUT, lang=LANG)

    url_base = refs._connector._url_base

    if '//localhost' in url_base:
        # Упрощение для локальной отладки.
        refs._connector._url_base = url_base.replace('https', 'http')

    return refs


def print_dict(dict_, indent=0):
    indent_str = ' ' * indent

    for name, info in dict_.items():
        has_dict = isinstance(info, dict)

        click.secho(indent_str + '%s: ' % name, nl=has_dict)

        if has_dict:
            print_dict(info, indent=indent+4)

        else:
            click.secho('%s' % info)


option_fields = click.option('--fields', multiple=True, help='Additional fields to output')
option_archived = click.option('--with_archived', help='Include archive records.', is_flag=True)
option_base_ids = click.option('--base_ids', multiple=True, help='Base ids filter.')
option_parent_ids = click.option('--parent_ids', multiple=True, help='Parent ids filter.')


@click.group()
@click.version_option(version=VERSION_STR)
@click.option('--verbose', help='Be verbose, output more info.', is_flag=True)
@click.option('--host', help='Overrides default Refs host.')
@click.option('--timeout', help='Overrides default request timeout.', type=int)
@click.option('--lang', help='Language to get response in.')
def base(verbose, host, timeout, lang):
    """Refs client command line utility."""

    configure_logging(logging.DEBUG if verbose else logging.INFO)

    if host:
        global HOST
        if host == 't':
            # Поддержка псевдонима.
            host = HOST_TEST

        HOST = host

    if timeout:
        global TIMEOUT
        TIMEOUT = timeout

    if lang:
        global LANG
        LANG = lang


@base.command('generic_get_types')
@click.argument('refname')
def generic_get_types(refname):
    """Print out known types for a given reference."""
    print_dict(get_refs().generic_get_types(refname))


@base.command('generic_get_resources')
@click.argument('refname')
def generic_get_resources(refname):
    """Print out known resource for a given reference."""
    print_dict(get_refs().generic_get_resources(refname))


@base.command('generic_get_type_fields')
@click.argument('refname')
@click.argument('typename')
def generic_get_type_fields(refname, typename):
    """Print out known fields for a given type in a given reference."""
    print_dict(get_refs().generic_get_type_fields(refname, typename))


@base.command('swift_bic_info')
@click.argument('bic', nargs=-1)
@option_fields
def swift_bic_info(bic, fields):
    """Print out information for a given Swift BIC."""
    swift = get_refs().get_ref_swift()
    print_dict(swift.get_bics_info(bic, fields=list(fields)))


@base.command('swift_holidays_info')
@click.option('--countries', multiple=True, help='Country filter. Two-letter code expected. E.g. RU, US.')
@click.option('--since', help='Date since filter. If not set current date is used. E.g. 2019-02-01.')
@click.option('--to', help='Date to filter. If not set current date is used. E.g. 2019-02-01.')
@option_fields
def swift_holidays_info(countries, since, to, fields):
    """Print out information about holidays, weekends etc."""
    swift = get_refs().get_ref_swift()

    holidays = swift.get_holidays_info(
        date_since=since,
        date_to=to,
        countries=list(countries),
        fields=list(fields),
    )

    for holiday in holidays:
        print_dict(holiday)
        click.secho('')


@base.command('cbrf_banks_info')
@click.argument('bic', nargs=-1)
@option_fields
@click.option('--states', multiple=True, help='States filter. Use `any` to show records in any state.')
def cbrf_banks_info(bic, fields, states):
    """Print out information for a given Bank BIC as per Bank of Russia data."""
    cbrf = get_refs().get_ref_cbrf()
    print_dict(cbrf.get_banks_info(bic, fields=list(fields), states=list(states)))


@base.command('cbrf_banks_listing')
@option_fields
def cbrf_banks_listing(fields):
    """Print out information about known banks from Bank of Russia."""
    cbrf = get_refs().get_ref_cbrf()
    for bank in cbrf.banks_listing(fields=list(fields)):
        print_dict(bank)


@base.command('currency_rates_info')
@option_fields
@click.option('--dates', multiple=True, help='Dates filter. If not set current date is used.')
@click.option('--sources', multiple=True, help='Sources filter.')
@click.option('--codes', multiple=True, help='Currency ISO codes filter.')
def currency_rates_info(fields, dates, sources, codes):
    """Print out information about currency rates."""
    currency = get_refs().get_ref_currency()

    rates = currency.get_rates_info(
        dates=list(dates),
        sources=list(sources),
        codes=list(codes),
        fields=list(fields),
    )

    for rate in rates:
        print_dict(rate)
        click.secho('')


@base.command('currency_sources')
@option_fields
def currency_sources(fields):
    """Print out information about known currency rates sources (providers)."""
    currency = get_refs().get_ref_currency()
    print_dict(currency.get_sources(fields=list(fields)))


@base.command('currency_listing')
@option_fields
def currency_listing(fields):
    """Print out information about known currencies."""
    currency = get_refs().get_ref_currency()
    print_dict(currency.get_listing(fields=list(fields)))


@base.command('addr_levels')
@option_fields
def addr_levels(fields):
    """Print out information about known address levels."""
    fias = get_refs().get_ref_fias()
    levels = fias.get_levels(fields=list(fields))
    for level in levels:
        print_dict(level)
        click.secho('')


@base.command('addr_info')
@option_base_ids
@option_parent_ids
@option_archived
@click.option('--levels', multiple=True, help='Levels filter.')
@click.option('--name', help='Names filter.')
@option_fields
def addr_info(base_ids, parent_ids, with_archived, levels, name, fields):
    """Print out information about addresses."""
    fias = get_refs().get_ref_fias()

    addresses = fias.get_addr_info(
        base_ids=list(base_ids),
        parent_ids=list(parent_ids),
        with_archived=with_archived,
        levels=list(levels),
        name=name,
        fields=list(fields)
    )

    for address in addresses:
        print_dict(address)
        click.secho('')


@base.command('house_info')
@option_base_ids
@option_parent_ids
@option_archived
@click.option('--num', help='Number filter.')
@click.option('--num_building', help='Building filter.')
@click.option('--num_struct', help='Struct filter.')
@option_fields
def house_info(base_ids, parent_ids, with_archived, num, num_building, num_struct, fields):
    """Print out information about addresses."""
    fias = get_refs().get_ref_fias()

    houses = fias.get_house_info(
        base_ids=list(base_ids),
        parent_ids=list(parent_ids),
        with_archived=with_archived,
        num=num,
        num_building=num_building,
        num_struct=num_struct,
        fields=list(fields)
    )

    for house in houses:
        print_dict(house)
        click.secho('')


@base.command('room_info')
@option_base_ids
@option_parent_ids
@option_archived
@click.option('--num', help='Number filter.')
@click.option('--num_flat', help='Number flat filter.')
@option_fields
def room_info(base_ids, parent_ids, with_archived, num, num_flat, fields):
    """Print out information about rooms."""
    fias = get_refs().get_ref_fias()

    rooms = fias.get_room_info(
        base_ids=list(base_ids),
        parent_ids=list(parent_ids),
        with_archived=with_archived,
        num=num,
        num_flat=num_flat,
        fields=list(fields)
    )

    for room in rooms:
        print_dict(room)
        click.secho('')


@base.command('stead_info')
@option_base_ids
@option_parent_ids
@option_archived
@option_fields
def stead_info(base_ids, parent_ids, with_archived, fields):
    """Print out information about steads."""
    fias = get_refs().get_ref_fias()

    steads = fias.get_stead_info(
        base_ids=list(base_ids),
        parent_ids=list(parent_ids),
        with_archived=with_archived,
        fields=list(fields)
    )

    for stead in steads:
        print_dict(stead)
        click.secho('')


@base.command('doc_info')
@option_base_ids
@option_fields
def doc_info(base_ids, fields):
    """Print out information about normative docs."""
    fias = get_refs().get_ref_fias()

    docs = fias.get_doc_info(
        base_ids=list(base_ids),
        fields=list(fields)
    )

    for doc in docs:
        print_dict(doc)
        click.secho('')


@base.command('dict_info')
@click.argument('resource')
@option_fields
def dict_info(resource, fields):
    """Print out information about auxiliary directory of addresses."""
    fias = get_refs().get_ref_fias()

    method = getattr(fias, 'get_%s_info' % resource.lower())

    result = method(fields=fields)

    for record in result:
        print_dict(record)
        click.secho('')


def main():
    """
    CLI entry point.
    """
    warnings.filterwarnings('ignore')

    try:
        base(obj={})

    except RefsClientException as e:
        click.secho(u'%s' % e, err=True, fg='red')
        sys.exit(1)


if __name__ == '__main__':
    main()
