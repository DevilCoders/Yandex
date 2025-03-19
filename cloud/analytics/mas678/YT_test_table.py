<<<<<<< cloud/analytics/mas678/YT_test_table.py
=======
import sys
clan_tools_path = '/home/mas678/arc/arcadia/cloud/analytics/python/lib/clan_tools/src'
if clan_tools_path not in set(sys.path):
    sys.path.append(clan_tools_path)
import clan_tools.data_adapters.YTAdapter as Yta
yta = Yta.YTAdapter(token="t")
import pandas as pd
df = pd.read_csv("train.csv")
print(df.head())
schema = pd.io.json.build_table_schema(df)["fields"]
schema = yta.apply_type(None, df)
print(schema)
yta.save_result(result_path = "//home/cloud_analytics/tmp/mas678/test_table", schema = schema, df = df)
>>>>>>> cloud/analytics/mas678/YT_test_table.py
