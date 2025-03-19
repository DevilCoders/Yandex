import os

from behave import given


@given('empty monrun cache')
def step_monrun_cache(context):
    context.monrun_cache = os.path.join(context.root, 'monrun')
    os.mkdir(context.monrun_cache)


@given('"{name:Param}" check is "{value:Param}" modified at "{ts:d}"')
@given('"{name:Param}" check is "{value:Param}"')
def step_store_check(context, name, value, ts=None):
    file_path = os.path.join(context.monrun_cache, name)
    with open(file_path, 'w') as fd:
        fd.write(value)
    if ts is not None:
        os.utime(file_path, (ts, ts))
