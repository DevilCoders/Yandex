OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
)

PY_SRCS(__init__.py)

RESOURCE(
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/referrer_reward_balance_withdrawals/resources/query.sql ods/yt/billing/referrer_reward_balance_withdrawals/resources/query.sql
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/referrer_reward_balance_withdrawals/resources/parameters.yaml ods/yt/billing/referrer_reward_balance_withdrawals/resources/parameters.yaml
)

END()
