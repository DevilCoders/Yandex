import pytest
import json
import os
import contextlib
import six


run_counter = 0


class ParameterException(Exception):
    pass


def job_context_fixture(parameters=None, inputs=None, outputs=None):
    @pytest.yield_fixture()
    def wrapped():
        with create_job_context(parameters, inputs, outputs) as jc:
            yield jc

    return wrapped


def dump_file(data, filename):
    with open(filename, 'w') as out:
        if isinstance(data, six.string_types):
            out.write(data)
        else:
            json.dump(data, out)


def get_job_context_dict(params={}, input_items={}, outputs={}):
    inputs = {name: [vv['unpackedFile'] for vv in value] for name, value in six.iteritems(input_items)}
    return {
        "meta": {
            "workflowUid": "TEST",
            "workflowURL": "https://nirvana.yandex-team.ru/flow/TEST",
            "operationUid": "TEST-%s" % run_counter,
            "description": "test fixture",
            "owner": "tester",
            "blockUid": "TEST-BLOCK-%s" % run_counter,
            "blockCode": "block$%s" % run_counter,
            "blockURL": "https://nirvana.yandex-team.ru/flow/TEST/FlowchartBlockOperation/TEST-BLOCK-%s" % run_counter,
            "processUid": "TEST-PROCESS",
            "processURL": "https://nirvana.yandex-team.ru/process/TEST-PROCESS",
        },
        "status": {
            "errorMsg": "./TEST-%s.error_msg.txt" % run_counter,
            "successMsg": "./TEST-%s.success_msg.txt" % run_counter,
            "log": "./TEST-%s.user_status.log" % run_counter
        },
        "parameters": params,
        "inputs": inputs,
        "outputs": outputs,
        "inputItems": input_items,
        "outputItems": {},
        "ports": {},
        "secrets": {}
    }


def write_job_context(parameters=None, inputs=None, outputs=None, wd=None):
    if wd is None:
        wd = os.getcwd()
    wd = os.path.abspath(wd)
    canon_input_items = {}
    for name, values in six.iteritems(inputs):
        canon_input_items[name] = []
        if not isinstance(values, list):
            values = [values]
        basic = {
            'wasUnpacked': False,
            'dataType': 'text'
        }
        for i, input_item in enumerate(values):
            if isinstance(input_item, six.string_types):
                input_item = os.path.abspath(input_item)
                item = basic.copy()
                item.update({
                    'unpackedFile': input_item,
                    'unpackedDir': os.path.dirname(input_item),
                })
                canon_input_items[name].append(item)
            elif isinstance(input_item, dict):
                value = basic.copy()
                value.update(input_item)
                type_ = value.pop('type')
                if type_ == 'content':
                    filename = os.path.join(wd, '%s_%s' % (name, i))
                    data = value.pop('data')
                    dump_file(data, filename)

                    value['unpackedFile'] = filename
                    value['unpackedDir'] = os.path.dirname(filename)
                canon_input_items[name].append(value)
            else:
                raise ParameterException('dont know what to do with data %s for name %s' % (input_item, name))

    for name, values in six.iteritems(canon_input_items):
        for value in values:
            value['unpackedDir'] = os.path.join(wd, value['unpackedDir'])
            value['unpackedFile'] = os.path.join(wd, value['unpackedFile'])

    outputs_dict = {}
    for output in outputs or []:
        if isinstance(output, six.string_types):
            output = {'name': output, 'file': os.path.join(wd, 'output_%s' % output)}

        if output.get('file'):
            outputs_dict[output['name']] = [output['file']]
        elif output.get('content'):
            generated_file = os.path.join(wd, 'output_%s' % output['name'])
            dump_file(output['content'], generated_file)
            outputs_dict[output['name']] = [generated_file]

    global run_counter
    run_counter += 1
    params = parameters or {}
    jc_dict = get_job_context_dict(params, canon_input_items, outputs_dict)
    with open(os.path.join(wd, 'job_context.json'), 'w') as out:
        json.dump(jc_dict, out)
    return jc_dict


@contextlib.contextmanager
def create_job_context(parameters=None, inputs=None, outputs=None, wd=None):
    yield write_job_context(parameters=parameters, inputs=inputs, outputs=outputs, wd=wd)
