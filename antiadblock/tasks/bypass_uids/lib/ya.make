PY3_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    util.py
    data.py
    model.py
)

RESOURCE(
    contrib/python/matplotlib/py3/matplotlibrc.template matplotlibrc
    yql_templ/prepare_dataset.sql prepare_dataset
    yql_templ/sample_dataset.sql sample_dataset
    yql_templ/evaluate_accuracy.sql evaluate_accuracy
    yql_templ/model_predict.sql model_predict
    yql_templ/update_nginx_uids.sql update_nginx_uids
    yql_templ/make_bypass_uids.sql make_bypass_uids
)

PEERDIR(
    library/python/resource
    contrib/python/matplotlib
    contrib/python/pandas
    contrib/python/scikit-learn
    catboost/python-package/lib
    yql/library/python
    library/python/yt
    yt/yt/python/yt_yson_bindings
)

END()
