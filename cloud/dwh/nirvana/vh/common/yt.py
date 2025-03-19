import pandas as pd
import typing as tp
from dateutil.parser import parse
from datetime import timezone
import yt.wrapper as yt
import json

schema_type = tp.List[tp.Dict[str, tp.Union[str, tp.Dict[str, str]]]]


def time_to_unix(time_str: str) -> int:
    """
    String time to int
    """
    dt = parse(time_str)
    timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
    return int(timestamp)


def _apply_type(raw_schema: tp.Optional[schema_type],
                df: pd.DataFrame) -> tp.List[schema_type]:
    """
    Create schema for dataset in YT.
    Supports datetime, int64, double, list with string and int and float.
    """
    if raw_schema is not None:
        for key in raw_schema:
            if 'list:' not in raw_schema[key] and raw_schema[key] != 'datetime':
                df[key] = df[key].astype(raw_schema[key])
            if raw_schema[key] == 'datetime':
                df[key] = df[key].apply(lambda x: time_to_unix(x))

    schema: tp.List[tp.Dict[str, tp.Union[str, tp.Dict[str, str]]]] = []
    for col in df.columns:
        if raw_schema is not None and raw_schema.get(col) is not None and raw_schema[col] == 'datetime':
            schema.append({"name": col, 'type': 'datetime'})
            continue
        if df[col].dtype == int:
            schema.append({"name": col, 'type': 'int64'})
        elif df[col].dtype == float:
            schema.append({"name": col, 'type': 'double'})
        elif raw_schema is not None and raw_schema.get(col) is not None and 'list:' in raw_schema[col]:
            second_type = raw_schema[col].split("list:")[-1]
            schema.append(
                {"name": col, 'type_v3':
                    {"type_name": 'list', "item":
                        {"type_name": "optional", "item": second_type}}}
            )
        else:
            schema.append({"name": col, 'type': 'string'})
    return schema


def save_table(proxy: str,
               token: str,
               path: str,
               table: pd.DataFrame,
               schema: tp.Optional[schema_type] = None,
               append: str = False):
    yt.config["proxy"]["url"] = proxy
    yt.config["token"] = token
    df = table.copy()
    real_schema = _apply_type(schema, df)
    json_df_str = df.to_json(orient='records')
    json_df = json.loads(json_df_str)
    if not yt.exists(path) or not append:
        yt.create(type="table", path=path, force=True,
                  attributes={"schema": real_schema})
    tablepath = yt.TablePath(path, append=append)
    yt.write_table(tablepath, json_df,
                   format=yt.JsonFormat(attributes={"encode_utf8": False}))
