import logging
import pandas as pd
from clan_tools.data_adapters.ChartsAdapter import ChartsAdapter
logger = logging.getLogger(__name__)


def chart_id_by_url(url):
    pages = url.split('/')
    return pages[pages.index("wizard")+1].split('-')[0]


def wizard_table_to_yt(datalens_url, yt_path, yt_adapter):
    charts_adapter = ChartsAdapter()
    chart_id = chart_id_by_url(datalens_url)
    columns = None
    rows = []
    page = 1
    while True:
        response = charts_adapter.fetch_chart_data(chart_id, page=page)
        if 'data' not in response:
            logger.error(f'No data for {datalens_url} and page {page}')
            break
        response_data = response['data']
        if not response_data:
            break
        if columns is None:
            columns = response_data['head']
        rows_page = response_data['rows']
        rows.extend(rows_page)
        if len(rows_page)<100:
            break
        page += 1
    if not columns:
        logger.error(f"Can't get columns for {datalens_url}")
        return
    columns_names = [col['name'].lower().replace(' ', '_') for col in columns]
    records = [[cell['value'] for cell in row['cells']] for row in rows]
    df = pd.DataFrame.from_records(records, columns=columns_names)
    logger.debug(df.dtypes)
    yt_adapter.save_result(yt_path, schema=None, df=df, append=False)
