import json
import logging.config
import typing as tp

import click
import numpy as np
import pandas as pd

from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from src.mql_marketing.utils import find_key_words, K_sorensen_levenstein, list_of_all_words

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def add_spax_name(path_to_update: str,
                  column_company_name: str,
                  strength: float = 0.5,
                  path_to_save: tp.Optional[str] = None,
                  path_to_keys: str = '//home/cloud_analytics/ml/mql_marketing/result/company_names_mapping/spark_company_keys',
                  verbose_amount: int = 20) -> None:

    yt_adapter = YTAdapter()

    logger.info('Step 1. Loading table to update...')
    df_update = yt_adapter.read_table(path_to_update)

    logger.info('Step 2. Loading table with keys...')
    df_keys = yt_adapter.read_table(path_to_keys)

    logger.info('Step 3. Prepare data for looking keys...')
    # name_keys
    name_keys = df_update[[column_company_name]].drop_duplicates().copy()
    name_keys['keys'] = name_keys[column_company_name].apply(find_key_words)
    # name_keys_long
    name_keys_long = []
    for ind in name_keys.index:
        for key in name_keys.loc[ind, 'keys']:
            name_keys_long.append({
                column_company_name: name_keys.loc[ind, column_company_name],
                'key': key,
                'ngram_keys': name_keys.loc[ind, 'keys'],
                'word_keys': list_of_all_words(name_keys.loc[ind, column_company_name])
            })
    name_keys_long = pd.DataFrame(name_keys_long)

    logger.info('Step 4. Calculating affinity scores...')
    tdf = name_keys_long.merge(df_keys, on='key', how='inner')
    logger.info(f' -> Total steps: {tdf.shape[0]}')
    logging_steps = np.linspace(0, tdf.shape[0], verbose_amount+2).astype(int)[1:-1]
    for ind in tdf.index:
        tdf.loc[ind, 'score_1'] = K_sorensen_levenstein(tdf.loc[ind, 'ngram_keys'],
                                                        tdf.loc[ind, 'spark_keys'])
        tdf.loc[ind, 'score_2'] = K_sorensen_levenstein(tdf.loc[ind, 'word_keys'],
                                                        tdf.loc[ind, 'one_word_spark_keys'])
        if ind in logging_steps:
            logger.info(f' -> Processed:   {ind}')

    logger.info('Step 5. Search of best matchings...')
    df_spax_found = []
    logger.info(f' -> Total steps: {name_keys.shape[0]}')
    logging_steps = np.linspace(0, name_keys.shape[0], verbose_amount+2).astype(int)[1:-1]
    for ii, temp_name in enumerate(name_keys[column_company_name]):
        temp_tdf = tdf[tdf[column_company_name]==temp_name]
        score_1_max = temp_tdf['score_1'].max()
        score_2_max = temp_tdf['score_2'].max()
        temp_tdf = temp_tdf[(temp_tdf['score_1']==score_1_max) & (temp_tdf['score_2']==score_2_max)]
        temp_tdf = temp_tdf[[column_company_name, 'spark_name', 'mal_name', 'inn']].drop_duplicates()
        if (temp_tdf.shape[0] == 1) and (score_2_max >= strength) and ((score_1_max+score_2_max)>=1.0):
            df_spax_found.append(temp_tdf)
        if ii in logging_steps:
            logger.info(f' -> Processed:   {ii}')

    df_spax_found = pd.concat(df_spax_found, axis=0, ignore_index=True)
    result = df_update.merge(df_spax_found, on=column_company_name, how='left')

    logger.info('Step 6. Save results...')
    yt_schema = yt_adapter.get_pandas_default_schema(result)
    if path_to_save is None:
        yt_adapter.yt.remove(path_to_update)
        yt_adapter.save_result(result_path=path_to_update, schema=yt_schema, df=result, append=False)
    else:
        yt_adapter.save_result(result_path=path_to_save, schema=yt_schema, df=result, append=False)
    return result


@click.command()
def add_spax() -> None:

    puids_table = '//home/cloud_analytics/ml/mql_marketing/by_puids'
    res_table = puids_table + '_spax'
    add_spax_name(path_to_update=puids_table, path_to_save=res_table, column_company_name='company_name', strength=0.5)

    with open('output.json', 'w') as fp:
        json.dump({"Status": "Success"}, fp)

if __name__ == "__main__":
    add_spax()
