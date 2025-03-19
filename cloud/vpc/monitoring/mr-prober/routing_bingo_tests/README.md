## Routing Bingo Tests

Routing Bingo is a third iteration of integration tests for overlay dataplane. They contain various clusters 
which implement common user's scenarios from simple client/server to static-route based VPC peering. 

To save resources, single cluster might be used in multiple tests with different options passed as cluster 
parameters to alter cluster behavior. Each such option set provides a single run within a test. Each test consists 
of one or more probers: all of them are checked in each test run.

#### Directory structure & file syntax

This directory consists of the following files:
- `config.yaml` defines routing-bingo endpoints for multiple Mr.Prober installations (some are installed on hw-labs)
  and multiple cluster sets (one hw-lab might contain separate sets for CI and for development), thus it provides 
  a mapping of cluster slugs to cluster ids in Mr.Prober database.
- `test_*.yaml` defines parameter sets for each test run and sets of probers that need to be checked in the run

Test file consists of the following fields:
- `cluster_slug` - slug of the cluster used for running the test, resolved to id using `clusters` map in config;
- `prober_slugs` - list of prober_slug labels used to produce test results. Each slug might produce multiple sets of 
results if matrix variables are used.
- `prober_result_count` and `prober_interval` - number of results and prober run interval respectively. The test runner
will wait for `(prober_result_count + 0.5) * prober_interval` seconds and expects `prober_result_count` results to 
be reported in Solomon after that time.
- `base_variables` - default mutable variables used in all runs, may be overridden per run.
- `runs` - list of definitions of test runs.

Test run definition consists of the following fields:
- `name` - name of the test run to be used in test reports.
- `skip` - if specified, test will be skipped with the specified reason as string.
- `variables` - cluster variables used to override `base_variables` and to update cluster.

#### Running tests manually

1. Provide API KEY (needed for updating clusters) - taken from https://yav.yandex-team.ru/secret/sec-01f9zraq8t0d5jmcwqt9v1nkpg/explore/versions
```shell
$ export MR_PROBER_API_KEY=...
```

2. Provide SOLOMON_OAUTH_TOKEN. See https://nda.ya.ru/t/n0kcBmt84tFWSq for instructions on retrieving it.
```shell
$ export SOLOMON_OAUTH_TOKEN=...
```

##### Using virtual environment 

To run all tests on environment `testing` use:
```shell 
(mr-prober) .../mr-prober/routing_bingo_tests$ PYTHONPATH=.. ../tools/routing_bingo/main.py -vv -e testing run
```

##### Using docker 

Prepare docker image:
```shell
$ ./build_docker_images.sh -i routing_bingo
```

Run docker:
```shell
$ YANDEX_INTERNAL_ROOT_CA_PATH="/etc/ssl/certs/YandexInternalRootCA.pem"
$ docker run -v "$PWD/routing_bingo_tests:/routing_bingo_tests" --network host --rm -it -e "TERM=xterm-256color" \
    -v "$YANDEX_INTERNAL_ROOT_CA_PATH:$YANDEX_INTERNAL_ROOT_CA_PATH" -w /routing_bingo_tests  \
    --env MR_PROBER_API_KEY --env SOLOMON_OAUTH_TOKEN \
    -- cr.yandex/crpni6s1s1aujltb5vv7/routing_bingo:latest \
    /mr_prober/tools/routing_bingo/main.py -v -e testing run
```

##### Preparing secrets for TeamCity AW

1. Install [skm](https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/#releases) locally

2. Provide tokens:
```shell
export YAV_TOKEN=...
export YC_TOKEN=$(yc --profile=prod iam create-token)
```

3. Encrypt bundle for skm:
```shell
cd secrets/testing/
skm encrypt-bundle --config skm.yaml --bundle secrets.yaml
```
