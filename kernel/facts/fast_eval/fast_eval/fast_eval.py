from __future__ import print_function
import argparse
from plumbum import local
import json
import os

"""
1. prepare runtime aliases resources

cd ~/arcadia/quality/functionality/facts/classifier_resources && ya package --raw-package --tar  \
./online_aliases.package.json && ln -s CREATED_DIRECTORY online-alias-resources

+ copy additional resources for new factors to ~/arcadia/quality/functionality/facts/classifier_resources/online-alias-resource

2. download base pool
for example here https://hitman.yandex-team.ru/projects/OnlineAliases/online_aliases_auto_learn_formula from latest run
yt read --format yamr //home/facts/robot-fact-snip/nirvana/3c463772-f18b-4e6a-81e6-9b063d295e3c/sorted__PEY3cvGLTmqB5psGPSlePA > ~/runtime_aliases_pool.tsv

3. add new factors code to build_pool and check it:

with loading all resources:
 ./fast_eval -p ~/runtime_aliases_pool.tsv -o ~/runtime_aliases_pool_patched.tsv --description 'test script with full resources' -j32 -f FI_TEST_NEW_FEATURE --experiment EXPERIMENT_ID_IN_FML

with loading minimum set of resources for new factors:
 ./fast_eval -p ~/runtime_aliases_pool.tsv -o ~/runtime_aliases_pool_patched.tsv --description 'test script with full resources' -j32 -f FI_TEST_NEW_FEATURE --experiment EXPERIMENT_ID_IN_FML \
 -c ./empty_resources.json  -d ../build_pool

"""


def get_args():
    parser = argparse.ArgumentParser(description='Fast eval for new features')
    parser.add_argument('-p', help='Prepared pool', required=True)
    parser.add_argument('-o', help='Output pool', default="~/new_pool.tsv")
    parser.add_argument('--description', help='New features description for fml', required=True)
    parser.add_argument('-d', default='../../../../quality/functionality/facts/classifier_resources/online-alias-resources', help='Directory with runtime aliases resources')
    parser.add_argument('-c', help='Config file with paths to all resources')
    parser.add_argument('-g', help='Version from config')
    parser.add_argument('-j', help='Number of threads')
    parser.add_argument('-f', nargs='*', help='Factors to add to the end of the pool')
    parser.add_argument('--build-pool-path', default='../build_pool/build_pool', help='path to binary with build_pool util')
    parser.add_argument('--factors-converter-path', default='../factors_converter/factors_converter', help='path to binary with factors_converter util')
    parser.add_argument('--experiment', help='ID experiment on FML', required=True)
    parser.add_argument('--iterations', default=13000, type=int, help='Number of iterations')
    parser.add_argument('--regularization', default=0.01, type=float, help='Regularization')
    parser.add_argument('--skip-build-pool-builder', action='store_true', help='Skip build new build_pool tool')
    parser.add_argument('--skip-build-factors-converter', action='store_true', help='Skip  build new factors_converter tool')
    return parser.parse_args()


def main():
    cfg = get_args()

    ya = local['ya']
    if not cfg.skip_build_pool_builder:
        print('Build build_pool binary...')
        make_res = ya('make', os.path.dirname(cfg.build_pool_path))
        print(make_res)
    if not cfg.skip_build_factors_converter:
        print('Build factors_converter binary...')
        make_res = ya('make', os.path.dirname(cfg.factors_converter_path))
        print(make_res)

    output_pool = os.path.expanduser(cfg.o)

    build_pool_args = []
    if cfg.d:
        build_pool_args.extend(["-d", cfg.d])
    if cfg.c:
        build_pool_args.extend(["-c", cfg.c])
    if cfg.g:
        build_pool_args.extend(["-g", cfg.g])
    if cfg.j:
        build_pool_args.extend(["-j", cfg.j])
    if cfg.f:
        for factor in cfg.f:
            build_pool_args.extend(["-f", factor])

    print('Build new pool...')
    build_pool = local[cfg.build_pool_path]
    all_params_to_print = ["-p", cfg.p, "-o", output_pool]
    all_params_to_print.extend(build_pool_args)
    print('Run: ' + str(build_pool) + " " + " ".join(all_params_to_print))
    build_pool("-p", cfg.p, "-o", output_pool, *build_pool_args)
    print('Pool created at ' + output_pool)

    print('Upload pool to fml...')
    ya = local['ya']
    upload_res = ya('upload', '--skynet', '--json-output', output_pool)
    print(upload_res)
    js = json.loads(upload_res)
    resource_id = js["resource_id"]
    curl = local['curl']
    uploader = curl['--data-urlencode', 'path=sbr:' + str(resource_id), '--data-urlencode', 'search-type=OTHER',
                    '--data-urlencode', 'description=' + cfg.description,
                    '--data-urlencode', 'owner=' + local.env["USER"], 'https://fml.yandex-team.ru/run/pool/upload']
    print(uploader)
    result = uploader()
    print(result)
    js = json.loads(result)
    pool_id = js["result"]
    print("Fml pool: https://fml.yandex-team.ru/pool/" + str(pool_id))

    print('Run eval features...')
    factors_converter = local[cfg.factors_converter_path]
    file_convert_args = []
    if cfg.f:
        for factor in cfg.f:
            file_convert_args.extend(["-f", factor])
    factors_info = factors_converter(*file_convert_args)
    tested_factors = ''
    for line in factors_info.split('\n'):
        parts = line.rstrip().split('\t')
        if len(parts) > 1:
            if len(tested_factors) > 0:
                tested_factors += ','
            tested_factors += parts[0] + '@' + parts[1]
    eval_features = curl['-X', 'POST', 'https://fml.yandex-team.ru/rest/api/eval/feature/task/run?type=gpu&pool-id=' + str(pool_id) +
                         '&tested-factor=' + tested_factors + '&iterations=' + str(cfg.iterations) + '&regularization=' + str(cfg.regularization) + '&owner=' + local.env["USER"] +
                         '&experiment-id=' + cfg.experiment + '&fold-count=48&notification-events=failed,completed&transmission-mode=email&cc-users=' + local.env["USER"]]
    print(eval_features)
    result = eval_features()
    print(result)


if __name__ == '__main__':
    main()
