OWNER(
    g:antirobot
)

PY2_PROGRAM()

PEERDIR(
    antirobot/scripts/antirobot_eventlog
    antirobot/scripts/access_log
    antirobot/scripts/utils

    contrib/python/jsonpickle
    contrib/libs/protobuf
    yt/python/client

    devtools/fleur/imports/arcadia/null
    devtools/fleur/util

    library/python/resource
    library/python/find_root

    yweb/scripts/datascripts/common
)

PY_SRCS(
    tweak/apply_tweaks.py
    tweak/bad_subnets.py

    tweak/botnets/__init__.py
    tweak/botnets/analyzer.py
    tweak/botnets/arclib.py
    tweak/botnets/count_max_ips.py
    tweak/botnets/id2time_ip.py
    tweak/botnets/serialize_log.py

    tweak/clicks.py
    tweak/cookie_req_counts.py
    tweak/dict_ci.py
    tweak/frauds.py
    tweak/hidden_image.py
    tweak/high_rps.py
    tweak/hiload.py
    tweak/patterns.py
    tweak/pool.py
    tweak/rnd_request.py
    tweak/req_entropy.py
    tweak/req_login.py
    tweak/req_words.py
    tweak/suspicious.py
    tweak/spravka_counts.py
    tweak/susp_info.py
    tweak/susp_request.py
    tweak/state_pickle.py
    tweak/too_many_redirects.py
    tweak/tweak_list.py
    tweak/tweak_task.py
    tweak/tweak_vyborka.py
    tweak/utl.py
    tweak/wrong_robots.py

    data_types.py
    split_vyborka.py
    tweak_factor_names.py
    setup_yt.py

    random_captchas/extract_eventlog_data.py
    random_captchas/add_tweaking_factors.py
    random_captchas/convert_factors_to_one_version.py
    random_captchas/make_matrixnet_features.py

    TOP_LEVEL
    make_learn_data.py
)

RESOURCE(
    tweak/patterns.txt /patterns.txt
    antirobot/scripts/learn/bad_subnets /bad_subnets
)

PY_MAIN(make_learn_data)

END()
