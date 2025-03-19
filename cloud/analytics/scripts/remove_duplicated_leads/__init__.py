import logging

import click
import yt.wrapper as yt
from cloud.analytics.scripts.common.leads_dyn_table import Lead
from tqdm import tqdm

LOGGER = logging.getLogger()


@click.command()
@click.argument('src_table')
@click.argument('dst_table')
@click.option('--proxy', help='YT Proxy', default='hahn')
@click.option('--loglevel', help='Python logging level', default="debug")
def main(src_table, dst_table, proxy, loglevel):
    numeric_level = getattr(logging, loglevel.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % loglevel)
    logging.basicConfig(level=numeric_level)

    yt.config['proxy']['url'] = proxy

    dt_client = yt.YtClient(proxy, config={"backend": "rpc"})

    for row in tqdm(yt.read_table(src_table),
                    total=yt.get(src_table + '/@row_count')):
        delete_dwh_ids = [{'dwh_id': dwh_id} for dwh_id in (set(row['dwh_ids']) - set((row['dwh_id'], )))]
        del row['dwh_ids']
        row['email'] = row['normalized_email']
        del row['normalized_email']
        del row['qty']
        with dt_client.Transaction(type='tablet'):
            LOGGER.debug('Writing row %s:%s', row['dwh_id'], row)
            lead = Lead(dst_table, dt_client)
            lead.from_dict(row)
            lead.save(with_transaction=False)
            LOGGER.debug('Deleting dwh_ids: %s', list(delete_dwh_ids))
            dt_client.delete_rows(dst_table, delete_dwh_ids)
