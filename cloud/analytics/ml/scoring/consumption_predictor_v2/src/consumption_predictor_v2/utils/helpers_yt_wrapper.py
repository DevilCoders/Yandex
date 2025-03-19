import pandas as pd
from clan_tools.utils.timing import timing

BA_ID_COL_NAME = 'billing_account_id'
BR_MSK_DT_NAME = 'billing_record_msk_date'
Y_TRUE_COL_NAME = 'next_14d_cons'
Y_PRED_COL_NAME = 'next_14d_cons_pred'


@timing
def read_preprocess_catboost_result_yt(yt_adapter, path: str) -> pd.DataFrame:
    df = yt_adapter.read_table(path)

    def decode_id(sample_id):
        ba_id, br_date = sample_id.split('_')
        return {BA_ID_COL_NAME: ba_id, BR_MSK_DT_NAME: br_date}

    df_res = pd.DataFrame([decode_id(x) for x in df['SampleId']], index=df.index)
    df_res[Y_TRUE_COL_NAME] = df['Label'].astype(float)
    df_res[Y_PRED_COL_NAME] = df['RawFormulaVal'].astype(float)
    return df_res
