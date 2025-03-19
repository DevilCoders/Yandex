# **clan_tools**

Library of utilities used by [YC Strategy Finance & Analytics
](https://abc.yandex-team.ru/services/cloud_analytics/) group in work processes


## **Getting started**


### **Prerequisites**

It uses python 3.7.
To use most data-adapters you need to have tokens of corresponding clients (or you need YAV token and all other ones could be taken from [Vault](https://yav.yandex-team.ru)). Read more [here](https://wiki.yandex-team.ru/cloud-bizdev/analitika-oblaka/texnicheskaja-informacija/chek-list-analitika/).
Also you can use [requirements.txt](requirements.txt) file, but it is not updated. Feel free to update it.

    pip install -r requirements.txt



### **Installing**

`clan_tools` could be installed via [pypi-repository](https://wiki.yandex-team.ru/pypi/) of Yandex (not preferable)

    pip install -i https://pypi.yandex-team.ru/simple clan_tools

Second (preferable) way to use `clan_tools` is setting System path to library's path in [arcadia](https://docs.yandex-team.ru/devtools/).
E.g. one of possible ways is to set in VSCode "terminal.integrated.profiles.linux"

    "bash": {"args": ["-c", "export `cat .env`; bash"]}

Then just add file `.env` in a root of your project:

    PYTHONPATH=/<path>/<to>/<arcadia>/arcadia/cloud/analytics/python/lib/clan_tools/src

## **Contributing**

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on the process for submitting pull requests to library.

## Versioning

We use [Semantic Versioning](http://semver.org/) for versioning. See change log below for more information about versions.

### **Change log**

Version 0.1.4 - Fixed `run_yql_script` vh-operation - added possibility to add more arguments [@pavelvasilev]

Version 0.1.3 - Updated `CRMHistoricalDataAdapter` according with data-sources from Cloud-DWH ODS-layer [@pavelvasilev]

Version 0.1.2 - Fixed bugs with `df_holidays_in_russia` data and `safe_append_spark` util [@pavelvasilev]

Version 0.1.1 - Minor update for secrets dictionary [@pavelvasilev]

Version 0.1.0 - Big update of `clan-tools`. All code is refactored according to common styling and mypy and flake8 settings are configured [@pavelvasilev]

Version 0.0.39 - Deprecated `YQLAdapter.execute_query`. Added more convenient `YQLAdapter.run_query` and `YQLAdapter.run_query_to_pandas`. [@albina-volk]

Version 0.0.38 - Added `run_yql_script` vh operation for running YQL scripts from file. Also updated `python_dl_base` operation [@albina-volk]

Version 0.0.37 - Added `python_dl_base` vh operation [@albina-volk]

Version 0.0.36 - Updated `spark_op`: adhoc cluster is rebuilt with new porto + updated version of operation [@pavelvasilev]

Version 0.0.35 - Fixed bug in `safe_append_spark`: with safe append when table is not exists [@pavelvasilev]

Version 0.0.34 - Added `safe_append_spark` function to save SparkDataFrame with `'append'` mode with pre-taken `shared lock` [@pavelvasilev]

Version 0.0.33 - Updated `YTAdapter` module: added retries for optimize_chunk_number function (to avoid errors when can not take exclusive lock on table) [@pavelvasilev]

Version 0.0.32 - Added container with new libraries. Added versions to future containers. It is recommended to fix container version in projects. [@pavelvasilev]

Version 0.0.31 - Updated `YTAdapter` module: added get_pandas_default_schema function for lazy generation of possible schema [@pavelvasilev]

Version 0.0.30 - Updated `YTAdapter` module: added leave_last_N_tables function to clear nodes after Solomon_to_YT transfer [@pavelvasilev]

Version 0.0.29 - Updated `CRMHistoricalDataAdapter`: based on valid but temporary tables [@pavelvasilev]

Version 0.0.28 - Updated `YTAdapter` module: added chunk count optimizer function [@pavelvasilev]

Version 0.0.27 - Updated job_layer container: added `yandex-yt-yson-bindings` for yt-wrapper [@pavelvasilev]

Version 0.0.26 - Added read_table function to YTAdapter [@albina-volk]

Version 0.0.25 - Fixed bug with tokens in operations [@pavelvasilev]

Version 0.0.24 - Added Solomon_to_YT and get_mr_directory operations for valhalla module [@pavelvasilev]

Version 0.0.23 - Added CatBoost operations for valhalla module & added most operations description [@pavelvasilev]

Version 0.0.22 - Added script_args possibility for job_layer valhalla operation [@pavelvasilev]

Version 0.0.21 - Logger saves otput to `analytics.log` too by default [@bakuteev]

Version 0.0.17 - User friendly Vault wrapper clan_tools.secrets.Vault [@bakuteev]

Version 0.0.16 - YTAdapter data types without schema are improved [@bakuteev]

Version 0.0.15 - YTAdapter create_paths, spark_op more suitable for our applications, prepare_dependencies for spark [@bakuteev]

Version 0.0.14 - Better ClickHouseAdapter serialization to pandas.
                 IMPORTANT install orjson [@bakuteev]

Version 0.0.13 - TrackerAdapter  bug fix, created_at, updated_at fields added [@bakuteev]

Version 0.0.12 - YtAdapter save_result complex types, TrackerAdapter  get_user_issues generalization [@bakuteev]

Version 0.0.11 - Tokens environment variables are changed to YT_TOKEN and YQL_TOKEN [@bakuteev]

Version 0.0.10 - Added `insert_into` method for ClickHouseYTAdapter [@bakuteev]

Version 0.0.9 - InterFax SPARK Adapter [@bakuteev]

Version 0.0.8 - ClickHouseAdapter rest compression, simple @cache decorator [@bakuteev]

Version 0.0.7 - Added ClickHouseAdapter [@artkaz]

Version 0.0.6 - YTAdapter accepts schema as dict also [@bakuteev]

Version 0.0.5 - TrackerAdapter uses StartTrack client, YTAdapter doesn't
encode to utf8 to save columns with Cyrillic [@bakuteev]

Version 0.0.4 - NileAdapter for porto layers, YQLAdapter allows to
get pandas data frames and run complex queries with files attached [@bakuteev]

Version 0.0.3 - added WikiAdapter [@ktereshin]

Version 0.0.2 - YTAdapter [@bakuteev]

Version 0.0.1 - ClickHouseYTAdapter, initial commit [@bakuteev]

## **Authors**

[YC Strategy Finance & Analitycs
](https://abc.yandex-team.ru/services/cloud_analytics/)
