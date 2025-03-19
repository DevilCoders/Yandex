OWNER(g:cloud-iam)

RECURSE(
    src
    test
    submodules/iam-access-service-client-proto/private-api/yandex/cloud/priv/accessservice/v2
)

RECURSE_FOR_TESTS(ut)
