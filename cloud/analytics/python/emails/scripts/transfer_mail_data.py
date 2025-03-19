from clan_tools.data_adapters.YQLAdapter import YQLAdapter
import logging.config
from clan_tools.utils.conf import read_conf
from clan_tools.utils.timing import timing
import click
from yql.client.operation import YqlSqlOperationRequest
from clan_emails.data_adapters.EmailsDataAdapter import EmailsDataAdapter
from clan_tools.utils.time import curr_utc_date_iso
import json
config = read_conf('config/logger.yml')
logging.config.dictConfig(config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--prod', is_flag=True)
def main(prod):
    yql_adapter = YQLAdapter()
    folder = "cloud_analytics" if prod else 'cloud_analytics_test'
    result_table_path = f'//home/{folder}/import/emails/emails_delivery_clicks'
    emails_adapter = EmailsDataAdapter(yql_adapter, 
                         append=False,
                         result_table = result_table_path)
    date = curr_utc_date_iso(days_lag=1)    
    req = emails_adapter.collect_data()
    logger.info(f'Request was sent.  Operation {req.operation_id}.')
    with open('output.json', 'w') as f:
        json.dump({"operation_id": req.share_url,
                    "table_path" : result_table_path
        }, f)
    


if __name__ == "__main__":
    main()