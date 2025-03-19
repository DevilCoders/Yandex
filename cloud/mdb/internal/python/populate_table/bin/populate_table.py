import argparse
import logging

from cloud.mdb.internal.python.populate_table.internal.base import get_conn, populate
from cloud.mdb.internal.python.populate_table.internal.logs import conf_logging


def main():
    """
    main
    """
    conf_logging()
    logger = logging.getLogger(__name__)
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--dsn', type=str, help='Connection DSN', default='dbname=dbaas_metadb')
    parser.add_argument('-t', '--table', type=str, help='Target table', default='dbaas.flavors')
    parser.add_argument('-f', '--input_file', type=str, help='Data file', default='flavors.json')
    parser.add_argument('-k', '--key', type=str, help='Key names (separated by \',\')', default='name')
    parser.add_argument('-r', '--refill', action='store_true', help='Drop table contents', default=False)
    args = parser.parse_args()
    conn = get_conn(args.dsn)
    with conn as txn:
        cursor = txn.cursor()
        logger.info('Populating table %s, refill=%s', args.table, args.refill)
        population = populate(cursor, args.table, args.input_file, args.key, args.refill)
        logger.info(
            'Populated table %s: %d created, %s removed, %d updated',
            args.table,
            population.created,
            population.deleted if population.deleted != population.ALL else 'all',
            population.updated,
        )
