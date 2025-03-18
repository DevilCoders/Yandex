import argparse
import copy
import json
import logging
import os
import sys
import subprocess
import traceback
import yaml

import core.error
from yalibrary.yandex import sandbox
import yalibrary.display
import yalibrary.vcs
import yalibrary.tools
from library.python import oauth
import library.python.svn_version as sv
from tools.sandboxctl.src import ui
from sandbox.common.types.task import ReleaseStatus as RS
import sandbox.common.types.notification as ctn
import sandbox.common.types.task as ctt
import test.util.shared
from devtools.ya.test.dependency import sandbox_storage

DOC_URL = 'https://doc.yandex-team.ru/sandboxctl'
PRETTY_FMT = ['oneline', 'short', 'medium', 'full', 'manifest']
API_VERSION = '1'
KIND_TASK = 'Task'
KIND_RESOURCE = 'SandboxResource'
RELEASE_STATUSES = [RS.TESTING, RS.PRESTABLE, RS.STABLE, RS.UNSTABLE, RS.CANCELLED]

# From https://oauth.yandex-team.ru/client/0fb8f816d99a43dabb4ab5e9956d35a6
CLIENT_ID = "0fb8f816d99a43dabb4ab5e9956d35a6"
CLIENT_SECRET = "03e89d8b728c40c2a86d09007ae0529d"


class version_action(argparse.Action):
    def __call__(self, parser, args, values, option_string=None):
        print(sv.svn_version())
        parser.exit()


def setup_logger(name, verbose):
    DEF_LOG_FORMAT = '%(asctime)s %(levelname)s %(module)s:%(lineno)d %(message)s'
    if not verbose:
        l = logging.ERROR
    elif verbose == 1:
        l = logging.WARNING
    elif verbose == 2:
        l = logging.INFO
    else:
        l = logging.DEBUG
    logging.basicConfig(level=l, format=DEF_LOG_FORMAT)
    return logging.getLogger(name)

log = logging.getLogger('')


def kv_str(kv):
    k, v = kv.split('=', 1)
    return (k, v.strip('\"'))


def notify_str(arg):
    DEF_STATUSES = "EXCEPTION,SUCCESS,EXPIRED,FAILURE,TIMEOUT,NO_RES"
    transport = 'email'
    receipents = []
    statuses = DEF_STATUSES

    tok = arg.split(':', 2)
    if len(tok) == 3:
        transport, receipents, statuses = tok
    elif len(tok) == 2:
        transport, receipents = tok
    elif len(tok) == 1:
        receipents = tok[0]

    t_lst = [e for e in ctn.Transport]
    if transport not in t_lst:
        raise argparse.ArgumentTypeError("Bad transport type: %s, candidates:%s" % (transport, t_lst))

    r_lst = receipents.split(',')
    if len(r_lst) == 0:
        raise argparse.ArgumentTypeError("No recepients")

    s_lst = statuses.split(',')
    if len(s_lst) == 0:
        raise argparse.ArgumentTypeError("No statuses")
    as_lst = [e for e in ctt.Status.Group.ALL]
    for s in s_lst:
        if s not in as_lst:
            raise argparse.ArgumentTypeError("Bad status type: %s, candidates:%s" % (s, as_lst))

    return {'transport': transport, 'statuses': s_lst, 'recipients': r_lst}


def submit_task(sbc, task_params):
    is_privileged = False
    # Convert task definition to rest_api compatible structure
    if 'custom_fields' in task_params:
        custom_fields = []
        for k, v in task_params['custom_fields'].iteritems():
            custom_fields.append({'name': k, 'value': v})
            if k == 'privileged':
                is_privileged = v
        task_params['custom_fields'] = custom_fields

        if 'tasks_archive_resource' in task_params and is_privileged:
            log.error('Binary tasks not support privileged mode yet: https://st.yandex-team.ru/SANDBOX-5906')
    if 'priority' in task_params:
        cls, sub = task_params['priority'].split(':')
        task_params['priority'] = {'class': cls, 'subclass': sub}

    log.debug("Create task creation with parameters %s", json.dumps(task_params, indent=4))
    task_id = sbc.create_task(**task_params)
    sbc.start_task(task_id)
    return task_id


def fill_task_manifest(task, task_context):
    task_params = {
        'priority': ":".join([task['priority']['class'], task['priority']['subclass']]),
        'type': task['type'],
        'description': task["description"],
        "requirements": task["requirements"]
    }
    # Remove runtime variables from context
    cleanup_ctx = ["task_version", "tasks_archive_resource", "tasks_version"]
    ctx = copy.deepcopy(task_context)
    for k, v in ctx.iteritems():
        if k.startswith("__"):
            cleanup_ctx.append(k)
    for e in cleanup_ctx:
        if e in ctx:
            del ctx[e]

    ctx.update(task['input_parameters'])
    task_params["custom_fields"] = ctx

    data = format_manifest(KIND_TASK, task_params)
    return data


def fill_task_info(sbc, args, task_id, out):
    task = sbc.get_task(task_id)
    task_context = sbc.get_task_context(task_id)
    task_url = sbc.get_task_url(task_id)

    if args.pretty_fmt == "manifest":
        return fill_task_manifest(task, task_context)

    out_data = {
        'meta': {
            'id': task_id,
            'type': task['type'],
            'url': task_url,
            'status': task['status'],
            'info': ui.html2text(task['results']['info']).splitlines()
        },
        'resources': []
    }

    if args.pretty_fmt == 'oneline':
        return out_data

    data = sbc.get_task_resources(task_id)
    for r in data['items']:
        rl = {'description': r['description'],
              'skynet_id': r['skynet_id'],
              'url': r['url'],
              'type': r['type'],
              'state': r['state'],
              'id': r['id'],
              'proxy': r['http']['proxy']}
        out_data['resources'].append(rl)

    if args.pretty_fmt == 'short':
        # Dumpt context if task fail
        if not sandbox.is_task_succeed(task):
            out_data['context'] = task_context
        return out_data

    out_data['context'] = task_context
    if args.pretty_fmt == 'medium':
        return out_data
    # HANDLE: args.pretty_fmt == 'full'
    out_data['task'] = sbc.get_task(task_id)
    return out_data


def do_dump_info(args, data, out):
    if args.out_json:
        json.dump(data, out, indent=4, sort_keys=True)
        return
    elif args.out_yaml:
        yaml.safe_dump(data, out, default_flow_style=False, width=79, explicit_start=True)
        return

    yaml.safe_dump(data, out, default_flow_style=False, width=79, explicit_start=True)


def dump_task_info(sbc, args, task_id, out):
    if args.quiet:
        out.write(str(task_id))
        return

    data = fill_task_info(sbc, args, task_id, out)
    if args.pretty_fmt == 'oneline':
        meta = data['meta']
        out.write('id: {} url: {} status: {}\n'.format(meta['id'], meta['url'], meta['status']))
        return
    do_dump_info(args, data, out)


def do_load_manifest(fname):
    with open(fname) as f:
        if fname.endswith('.json'):
            return json.load(f)
        return yaml.safe_load(f)


def load_manifest(fname, kind):
    log.debug('Load manifest file %s, for %s' % (fname, kind))
    data = do_load_manifest(fname)

    if not data.get('apiVersion') == API_VERSION:
        Exception("Unsupported manifest version:%s, expect:%s" % (data.get('apiVersion'), API_VERSION))
    if not data.get('kind') == kind:
        Exception("Unsupported manifest kind:%s, expcect:%s" % (data.get('kind'), kind))
    return data['params']


def format_manifest(kind, params):
    data = {'apiVersion':  API_VERSION,
            'kind': kind,
            'params': params}
    return data


def save_manifest(fname, kind, params):
    data = format_manifest(kind, params)
    with open(fname, "w+") as f:
        if fname.endswith('.json'):
            json.dump(data, f, indent=4, sort_keys=True)
        else:
            yaml.safe_dump(data, f, default_flow_style=False, width=79, explicit_start=True)


def task_bin_upload(task_bin, url="", token="", taskbox=True):
    cmd = [task_bin, 'upload']
    if url:
        cmd += ['--url', url]
    if token:
        cmd += ['--token', token]
    if taskbox:
        cmd += ['--enable-taskbox']

    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    if p.wait():
        raise Exception("Can't  upload binary task, cmd: %s failed" % cmd)
    jdata = p.stdout.readline().strip()
    data = json.loads(jdata)
    return data['resource']['id']


def _get_task(sbc, args, out, task_id=None, wait_statuses=sandbox.DONE_STATUSES, check=True):
    log.debug('request task %d url' % task_id)
    task_url = sbc.get_task_url(str(task_id))
    if not args.wait:
        dump_task_info(sbc, args, task_id, out)
        return
    log.debug('Wait completion for task: %s' % task_url)

    ts_printer = ui.get_task_state_printer(args.display)
    try:
        task = sbc.wait_task(task_id, ts_printer, None, wait_statuses)
        log.debug('Task: %s completes with status: %s', task['id'], task['status'])
    except KeyboardInterrupt:
        check = False
        log.info("\nCtrl-C interrupt, skip wait")

    dump_task_info(sbc, args, task_id, out)
    if check and not sandbox.is_task_succeed(task):
        raise sandbox.SandboxExecuteTaskException("Task {} has failed with status {}".format(task["id"], task["status"]))


def create_task_handler(parser, sbc, args, out):
    if args.in_file:
        task_params = load_manifest(args.in_file, KIND_TASK)
    else:
        task_params = {}

    if args.task_type:
        task_params['type'] = args.task_type
    if not task_params.get('type'):
        raise argparse.ArgumentTypeError("Task type is not set")

    if args.desc:
        task_params['description'] = args.desc

    if args.priority:
        task_params['priority'] = args.priority

    ctx = dict(args.ctx_kv_list)
    for s in args.ctx_json_list:
        c = json.loads(s)
        ctx.update(c)

    ctx.setdefault('notify_if_finished', '')

    task_params.setdefault('custom_fields', {})
    task_params['custom_fields'].update(ctx)

    for s in args.body_json_list:
        c = json.loads(s)
        task_params.update(c)

    if args.notify_list:
        task_params.update({'notifications': args.notify_list})

    if args.require_list:
        task_params.update({'requirements': dict(args.require_list)})

    # Always load task's owner from input arguments
    task_params['owner'] = args.owner

    # Always prefer bin task argument from comandline to one which comes from manifest
    if args.task_bin:
        tsk_resource_id = task_bin_upload(args.task_bin, args.service_url, args.token)
        task_params['tasks_archive_resource'] = tsk_resource_id
        log.debug('Use SANDBOX_TASK_BINARY resource_id:%s', tsk_resource_id)

    if args.out_file:
        save_manifest(args.out_file, KIND_TASK, task_params)

    if args.dry_run:
        args.wait = False
        log.info('Dry run mode, skip real task creation, exit')
        return

    task_id = submit_task(sbc, task_params)
    if not args.quiet:
        log.info('Starting task %s %s' % (task_params['type'], sbc.get_task_url(task_id)))
    return _get_task(sbc, args, out, task_id)


def obj2dict_clr(d, o, d_name, o_name):
    if hasattr(o, o_name) and getattr(o, o_name):
        d[d_name] = getattr(o, o_name)
        setattr(o, o_name, None)


class _Arc(object):
    url_prefix = "arcadia-arc://#"

    def __init__(self):
        self._path = yalibrary.tools.tool('arc')
        log.debug('Arc tool: %s', self._path)

    def run(self, *args, **kwargs):
        log.debug('Running arc: %s', ' '.join(args))
        p = subprocess.Popen([self._path] + list(args), stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)
        out, err = p.communicate()
        if p.returncode != 0:
            log.debug('Error running \'arc %s\': exit code %d\n%s', ' '.join(args), p.returncode, err)
            raise Exception('Failed to run arc tool')
        return out

    def get_info(self):
        out = self.run('info', '--json')
        info = json.loads(out)
        log.debug('arc_info: {}'.format(info))
        return info

    def rev_parse(self, revspec):
        return self.run('rev-parse', revspec)

    def normalize_url(self, url=None):
        info = self.get_info()
        if not url:
            # By default use current remote HEAD
            return _Arc.url_prefix + info['hash']
        if url.startswith(_Arc.url_prefix):
            # Url aready correct, nothing to do
            return url
        # At this point url provided contains revspec, convert it to hash
        return _Arc.url_prefix + self.rev_parse(url)


def setup_vcs_ctx(args):
    vcs_types, vcs_root, start_path = yalibrary.vcs.detect()
    if not vcs_types:
        log.debug('Failed to detect version control system')
        return
    vcs_type = vcs_types[0]
    log.debug("vcs: type:{} root: {}, start_path: {}".format(vcs_type, vcs_root, start_path))
    # Update artifacts path relative to vcs_root
    a_list = []
    for art in args.artifact_list:
        a_list.append(os.path.relpath(os.path.join(start_path, art), vcs_root))
    args.artifact_list = a_list
    if vcs_type == 'arc':
        try:
            arc = _Arc()
            args.checkout_arcadia_from_url = arc.normalize_url(args.checkout_arcadia_from_url)
        except Exception as e:
            log.error("Fail to setup arcadia url err: {}".format(str(e)))


def create_yamake_handler(parser, sbc, args, out):
    if not args.task_type:
        args.task_type = 'YA_MAKE_TGZ'

    ctx = dict(use_aapi_fuse=True)

    if not args.no_vcs_detect:
        setup_vcs_ctx(args)

    # Generate target list based on artifacts
    for art in args.artifact_list:
        tgt = os.path.dirname(art)
        if tgt not in args.target_list:
            args.target_list.append(tgt)

    args.artifact_list = ";".join(args.artifact_list)
    args.target_list = ";".join(args.target_list)
    obj2dict_clr(ctx, args, 'arts', 'artifact_list')
    obj2dict_clr(ctx, args, 'targets', 'target_list')
    obj2dict_clr(ctx, args, 'arcadia_patch', 'arcadia_patch')
    obj2dict_clr(ctx, args, 'definition_flags', 'definition_flags')
    obj2dict_clr(ctx, args, 'result_rt', 'result_rt')
    obj2dict_clr(ctx, args, 'result_rd', 'result_rd')
    obj2dict_clr(ctx, args, 'build_output_ttl', 'ttl')
    obj2dict_clr(ctx, args, 'test', 'test')
    obj2dict_clr(ctx, args, 'result_single_file', 'result_single_file')
    obj2dict_clr(ctx, args, 'checkout_arcadia_from_url', 'checkout_arcadia_from_url')

    # Combine tests results
    if ctx.get('test'):
        ctx['junit_report'] = True

    data = json.dumps(ctx, indent=4, sort_keys=True)
    args.ctx_json_list = [data]
    if not args.quiet:
        log.info('ctx: {}'.format(data))

    if not args.desc:
        args.desc = 'Manual run: ya make {}'.format(ctx['targets'])

    create_task_handler(parser, sbc, args, out)


def get_task_handler(parser, sbc, args, out):
    return _get_task(sbc, args, out, args.task_id)


def suspend_task_handler(parser, sbc, args, out):
    args.wait = True
    args.quiet = True

    task = sbc.get_task(args.task_id)
    if task['status'] not in sandbox.SUSPENDED_STATUSES:
        sbc.suspend_task(args.task_id)
    return _get_task(sbc, args, out, args.task_id, sandbox.SUSPENDED_STATUSES, False)


def resume_task_handler(parser, sbc, args, out):
    sbc.resume_task(args.task_id)
    return _get_task(sbc, args, out, args.task_id)


def stop_task_handler(parser, sbc, args, out):
    sbc.stop_task(args.task_id)
    return _get_task(sbc, args, out, args.task_id, sandbox.DONE_STATUSES, False)


def release_task_handler(parser, sbc, args, out):
    if not args.wait:
        log.error("Release should be done only synchronously, please add --wait")
        sys.exit(1)

    # Wait task to finish
    null_f = open(os.devnull, 'w')
    _get_task(sbc, args, null_f, args.task_id, sandbox.DONE_STATUSES + ('NOT_RELEASED',))
    sbc.rest.release({
        'task_id': args.task_id,
        'subject': args.release_subj,
        'type': args.release_type,
    })
    _get_task(sbc, args, out, args.task_id, ('RELEASED', 'NOT_RELEASED'))


def make_resource_handler(parser, sbc, args, out):
    # Client is not required here
    assert sbc is None

    resource_params = dict(path=args.res_path, required=args.res_required, type=args.res_type, desc=args.res_desc)
    attr = {}
    for k, v in args.res_attr_list:
        attr[k] = v
    if (len(attr)):
        resource_params['attr'] = attr
    if not args.out_file:
        raise argparse.ArgumentTypeError("-o/--outfile FILE is required for this command")

    save_manifest(args.out_file, KIND_RESOURCE, resource_params)


def dump_one_resource_info(args, out, res, full_path=True):
    if full_path:
        path_desc = "full_path"
        path = os.path.join(res['path'], res['file_name'])
    else:
        path_desc = "path"
        path = os.path.basename(res['file_name'])
    if args.quiet:
        if hasattr(args, 'quiet_field'):
            out.write('{}\n'.format(res[args.quiet_field]))
        else:
            out.write(path + "\n")
        return
    if args.pretty_fmt == 'oneline':
        out.write('id: {} task_id: {} {}: {} {}\n'.format(res['id'],
                                                             res['task']['id'],
                                                             path_desc, path,
                                                             res['skynet_id']))
        return
    data = res
    if args.pretty_fmt == 'manifest':
        data = format_manifest(KIND_RESOURCE, res)

    do_dump_info(args, data, out)


# TODO: rewrite crappy formatting loop to case based one
def dump_resources_info(args, out, resources, full_path=True, collapse_list=True):
    assert isinstance(resources, list)

    if args.pretty_fmt == 'full':
        if collapse_list and len(resources) == 1:
            resources = resources[0]
        return do_dump_info(args, resources, out)

    # Dump only stable resource attributes
    UNSTABLE_FIELDS = ['http', 'rsync',  'sources']
    if args.pretty_fmt != 'oneline':
        UNSTABLE_FIELDS += ['time']
    for res in resources:
        for k in UNSTABLE_FIELDS:
            if k in res:
                del res[k]
        dump_one_resource_info(args, out, res, full_path)


def obj2dict(d, src, name):
    if hasattr(src, name):
        d[name] = getattr(src, name)


def list_resource_handler(parser, sbc, args, out):
    kwargs = {}
    attr = {}
    for k, v in args.res_attr_list:
        attr[k] = v
    if (len(attr)):
        kwargs['attrs'] = attr

    if args.json_query:
        with open(args.json_query) as f:
            jdata = json.load(f)
        rq = None
        for name in jdata.iterkeys():
            if not args.json_query_name:
                args.json_query_name = name
            if args.json_query_name == name:
                rq = jdata[name]
        if not rq:
            raise argparse.ArgumentTypeError("Cant find entry: {} in jqdata: {}".format(args.json_query_name, jdata))
        log.debug('reqource query pattern: {}'.format(rq))
        if 'resource_type' in rq:
            args.type = rq['resource_type']
        if 'attributes' in rq:
            kwargs['attrs'] = rq['attributes']

    obj2dict(kwargs, args, 'type')
    obj2dict(kwargs, args, 'id')
    obj2dict(kwargs, args, 'arch')
    obj2dict(kwargs, args, 'state')
    obj2dict(kwargs, args, 'owner')
    obj2dict(kwargs, args, 'client')
    obj2dict(kwargs, args, 'task_id')
    obj2dict(kwargs, args, 'accessed')
    obj2dict(kwargs, args, 'created')
    obj2dict(kwargs, args, 'depandant')
    obj2dict(kwargs, args, 'limit')
    log.debug('list_resources dict: %s' % str(kwargs))

    res = sbc.list_resources(**kwargs)
    args.quiet_field = 'id'
    dump_resources_info(args, out, res, False)


def get_resource_handler(parser, sbc, args, out):
    try:
        test.util.shared.setup_logging(log.getEffectiveLevel(), None)
        download_methods = ["skynet", "http", "http_tgz"]
        log.debug("Download methods are: %s", download_methods)
        storage = sandbox_storage.SandboxStorage(args.root, download_methods=download_methods)
        # FIXME: Add helper to SandboxStorage.set_sb_client and log.debug direct assignement
        storage._sandbox_client = sbc
        log.debug("Getting resource %s", args.resource_id)
        resource = storage.get(args.resource_id, decompress_if_archive=False, rename_to=args.rename_to)
        res_obj = json.loads(str(resource))
        dump_resources_info(args, out, [res_obj])
    except Exception as e:
        log.exception("Error while downloading resource %s", args.resource_id)
        if core.error.is_temporary_error(e):
            log.exception("Error is considered to be temporary - exit with INFRASTRUCTURE_ERROR")
            return core.error.ExitCodes.INFRASTRUCTURE_ERROR
        return 1
    return 0


def del_resource_handler(parser, sbc, args, out):
    try:
        test.util.shared.setup_logging(log.getEffectiveLevel(), None)
        if args.force:
            for res in args.resource_id:
                sbc.drop_resource_attr(res, "ttl")
        sbc.rest.batch.resources["delete"].update(args.resource_id)
    except Exception as e:
        log.exception("Fail to delete attr  %s", args.resource_id)
        if core.error.is_temporary_error(e):
            log.exception("Error is considered to be temporary - exit with INFRASTRUCTURE_ERROR")
            return core.error.ExitCodes.INFRASTRUCTURE_ERROR
        return 1
    return 0


def init_argparse():
    parser = argparse.ArgumentParser(description='sandboxctl (rev:%s)' % sv.svn_revision(),
                                     epilog=DOC_URL)

    generic = argparse.ArgumentParser(add_help=False)

    generic.add_argument("-v", '--verbose', dest="do_verbose",
                         action="count", default=2,
                         help="increases log verbosity for each occurence.")
    generic.add_argument('-V', '--version', action=version_action, nargs=0)
    generic.add_argument('-q', '--quiet', dest='quiet', default=False, action='store_true')
    generic.add_argument('-n', '--dry-run', dest='dry_run', default=False, action='store_true')
    client_opts = argparse.ArgumentParser(add_help=False)

    client_opts.add_argument("-S", '--service', dest="service_url",
                             default=os.environ.get('SANDBOX_URL', sandbox.DEFAULT_SANDBOX_URL),
                             help="Sandbox URL, default: %(default)s")
    client_opts.add_argument("-T", '--token', dest="token",
                             default=os.environ.get('SANDBOX_TOKEN', ''),
                             help='OAuth token for sandbox: default from $SANDBOX_TOKEN or requested by ssh key')

    out_opts = argparse.ArgumentParser(add_help=False)
    out_opts.add_argument('-o', '--outfile', dest='out_file', type=str, help='out manifest file')
    out_opts.add_argument('-J', '--json', dest='out_json', default=False, action='store_true')
    out_opts.add_argument('-Y', '--yaml', dest='out_yaml', default=False, action='store_true')
    out2_opts = argparse.ArgumentParser(add_help=False)
    out2_opts.add_argument("--pretty", dest="pretty_fmt", type=str, default='short', choices=PRETTY_FMT,
                           help="pretty format")
    out2_opts.add_argument('-w', '--wide', '--full', dest='pretty_fmt', action='store_const', const='full')
    out2_opts.add_argument('--oneline', dest='pretty_fmt', action='store_const', const='oneline')
    out2_opts.add_argument('--medium', dest='pretty_fmt', action='store_const', const='medium')
    out2_opts.add_argument("--manifest", "--clone", dest="pretty_fmt", action="store_const", const="manifest",
                           help="Dump task info in reusable task manifest format")

    in_opts = argparse.ArgumentParser(add_help=False)
    in_opts.add_argument('-i', '--infile', dest='in_file', type=str, help='input manifest file')

    wait_opts = argparse.ArgumentParser(add_help=False)
    wait_opts.add_argument('-W', '--wait', dest='wait', default=False, action='store_true')

    task_opts = argparse.ArgumentParser(add_help=False)
    task_opts.add_argument('task_id', type=int, help='Task ID')

    subparsers = parser.add_subparsers(title="Possible extra actions", dest='command')

    def_task_opts = [generic, client_opts, out_opts, out2_opts, in_opts, wait_opts]
    def_taskid_opts = def_task_opts + [task_opts]

    create_opts = argparse.ArgumentParser(add_help=False)
    create_opts.add_argument('-N', '--name', dest='task_type', type=str,
                             help='Task type name')
    create_opts.add_argument('-D', '--desc', dest='desc', type=str,
                             metavar='TEXT',
                             help='Task description')
    create_opts.add_argument('-U', '--owner', dest='owner',
                             default=os.environ.get('SANDBOX_USER', os.environ.get('USER')),
                             help="Task owner user, default: %(default)s (from $SANDBOX_USER or $USER)")
    create_opts.add_argument('--notify', dest='notify_list', action='append', type=notify_str, default=[],
                             help='Add notifications in format $TRANSPORT:$USERS:$STATUSES, example: "email:guest,guest2:SUCCESS,FAILURE,TIMEOUT,NO_RES"')
    create_opts.add_argument("--require", "-r", dest="require_list", action="append", type=kv_str, default=[],
                             metavar='TYPE=VALUE',
                             help="Add requirement, example: client_tags='(PORTO & LINUX)' platform='linux_ubuntu_16.04_xenial' cpu_model='E5-2650 v2' host='sas1-1337'")
    create_opts.add_argument('-p', '--priority', dest='priority', default='SERVICE:NORMAL', type=str,
                             help='Task priority: {BACKGROUND,SERVICE,USER}:{LOW,NORMAL,HIGH}, default: %(default)s')
    create_opts.add_argument('-b', '--sandbox-task-binary', dest='task_bin', default="", type=str,
                             help='SANDBOX_TASK_BINARY to use')
    create_opts.add_argument('-c', '--ctx-opt', dest='ctx_kv_list', action='append', type=kv_str, default=[],
                             metavar='PARAM=VALUE',
                             help='Append key value to task context, example -c param1=val1')
    create_opts.add_argument('-C', '--ctx-jsonstr', dest='ctx_json_list', action='append', type=str, default=[],
                             metavar='JSON',
                             help='Json string context, example -C {"key1":"val1"}')
    create_opts.add_argument('--inject-body', dest='body_json_list', action='append', type=str, default=[],
                             help='Inject Json string directly to task body, example --inject-body {"requirements": " {"cores": "8"}}')

    create = subparsers.add_parser(name="create", description='Create task',  parents=def_task_opts + [create_opts])
    create.set_defaults(handle=create_task_handler, token_required=True)

    # Special type of create_task, create ya_make task
    yamake = subparsers.add_parser(name="ya-make", description='Run YA_MAKE job',  parents=def_task_opts + [create_opts])
    yamake.set_defaults(handle=create_yamake_handler, token_required=True)
    yamake.add_argument('artifact_list', nargs="*", type=str, default=[], help="Artifact to build ops:arts")
    yamake.add_argument('--target', dest='target_list', action='append', type=str, default=[], help="Targets to build opt:targets")
    yamake.add_argument('-A', '--test', action='store_true', help='Run all tests opt:tests')
    yamake.add_argument('--no-vcs-detect', action='store_true', help='Do not try to detect vcs context')
    yamake.add_argument('--patch', dest='arcadia_patch', type=str, help='Apply patch opt: arcadia_patch')
    yamake.add_argument('-u', '--checkout-url', dest='checkout_arcadia_from_url', type=str, help='Checkout url for arcadia opt:checkout_arcadia_from_url')
    yamake.add_argument('--definition-flags', type=str, help='Pass "-D $OPT" option to ya make')
    yamake.add_argument('--type', dest='result_rt', type=str, default='ARCADIA_PROJECT_TGZ', help='Build artifact type opt:result_rt')
    yamake.add_argument('--ttl', type=str, help='Build artifact ttl opt:build_output_ttl')
    yamake.add_argument('--single', dest='result_single_file', action='store_true', help='Store result as single resource.tar.gz archive opt:target_list')

    get_task = subparsers.add_parser(name="get_task", description='get task info',  parents=def_taskid_opts)
    get_task.set_defaults(handle=get_task_handler)

    suspend_task = subparsers.add_parser(name="suspend", description='Suspend task',  parents=def_taskid_opts)
    suspend_task.set_defaults(handle=suspend_task_handler, token_required=True)

    resume_task = subparsers.add_parser(name="resume", description='Resume task',  parents=def_taskid_opts)
    resume_task.set_defaults(handle=resume_task_handler, token_required=True)

    stop_task = subparsers.add_parser(name="stop", description='Stop task',  parents=def_taskid_opts)
    stop_task.set_defaults(handle=stop_task_handler, token_required=True)

    release_task = subparsers.add_parser(name="release", description='Release task',  parents=def_taskid_opts)
    release_task.set_defaults(handle=release_task_handler, token_required=True)
    release_task.add_argument('-D', '--desc', dest='release_subj', default='', type=str, help='Task release description')
    release_task.add_argument('--release-type', dest='release_type', default=RS.UNSTABLE, type=str,
                              help='Release type', choices=RELEASE_STATUSES)

    make_resource = subparsers.add_parser(name="make_resource", description='Create resource manifest',  parents=[generic, out_opts])
    make_resource.set_defaults(handle=make_resource_handler, client_required=False)
    make_resource.add_argument('res_path', type=str, help='Resource path')
    make_resource.add_argument('--required', dest='res_required', default=False, action='store_true')
    make_resource.add_argument('-t', '--type', dest='res_type', default='OTHER_RESOURCE', type=str, help='Resource type')
    make_resource.add_argument('-D', '--desc', dest='res_desc', default='', type=str, help='Resource description')
    make_resource.add_argument('-A', '--attr', dest='res_attr_list', action='append', type=kv_str, default=[],
                               help='Add resource attribute as key val , example -A attr_name1=attr_val1')

    get_resource = subparsers.add_parser(name="get_resource", description='Fetch resource',  parents=[generic, client_opts, out_opts, out2_opts])
    get_resource.set_defaults(handle=get_resource_handler)
    get_resource.add_argument("-R", "--out-root", dest="root", help="Output root", type=str, default=os.path.join(os.getenv("HOME") or "", '.ya'))
    get_resource.add_argument("-O", "--out", dest='rename_to', help="Save downloaded resource as", type=str)
    get_resource.add_argument("resource_id", help="Resource id to fetch", type=str)

    del_resource = subparsers.add_parser(name="del_resource", description='Delete resource',  parents=[generic, client_opts, out_opts, out2_opts])
    del_resource.set_defaults(handle=del_resource_handler)
    del_resource.add_argument('--force', dest='force', default=False, action='store_true')
    del_resource.add_argument("resource_id", metavar='ID', type=int, nargs='+', help="Resource id to delete")

    list_resource = subparsers.add_parser(name="list_resource", description='List resource',  parents=[generic, client_opts, out_opts, out2_opts])
    list_resource.set_defaults(handle=list_resource_handler)
    list_resource.add_argument('-A', '--attr', dest='res_attr_list', action='append', type=kv_str, default=[],
                               help='Add resource attribute as key val , example -A attr_name1=attr_val1')

    list_resource.add_argument('--order', dest='order', default='+', type=str, help='Sort order', choices=["+", "-"])
    list_resource.add_argument('--id', dest='id', action='append', type=str, default=[], help="Resource id")
    list_resource.add_argument('--type', dest='type', action='append', type=str, default=[], help="Resource type")
    list_resource.add_argument('--state', dest='state', action='append', type=str, default=[], help="Resource state")
    list_resource.add_argument('--task_id', dest='task_id', action='append', type=int, default=[], help="Resource task_id")
    list_resource.add_argument('-U', '--owner', dest='owner', help='Resouce owner')
    list_resource.add_argument('--client', dest='client', type=str, help='Host where resource is located')
    list_resource.add_argument('--dependant', dest='dependant', type=int, help='Resource dependant')
    list_resource.add_argument('--limit', dest='limit', type=int, help='Number of resources', default=10)
    list_resource.add_argument('--jq', dest='json_query', type=str, help='File with json search pattern, for example ya.make.autoupdate')
    list_resource.add_argument('--jq-name', dest='json_query_name', type=str, help='Lookup name in --jq file')

    return parser


def main(cmdline=None):
    global log
    if cmdline is None:
        cmdline = sys.argv[1:]

    parser = init_argparse()
    args = parser.parse_args(sys.argv[1:])
    log = setup_logger('sandboxctl', args.do_verbose)
    # TODO: Move display config to separate function
    #    if args.quiet:
    #        args.display = yalibrary.display.DevNullDisplay
    fmt = yalibrary.formatter.Formatter(yalibrary.formatter.TermSupport())
    args.display = yalibrary.display.Display(sys.stderr, fmt)

    try:
        if not getattr(args, 'client_required', True):
            # Command which not required communication with sandboxclient
            sandbox_client = None
        else:
            if args.token:
                sandbox_client = sandbox.SandboxClient(sandbox_url=args.service_url, token=args.token)
            elif getattr(args, 'token_required', False):
                sandbox_token = oauth.get_token(CLIENT_ID, CLIENT_SECRET)
                sandbox_client = sandbox.SandboxClient(sandbox_url=args.service_url, token=sandbox_token)
            else:
                sandbox_client = sandbox.SandboxClient(sandbox_url=args.service_url)
            log.debug('Use sandbox_url: %s ' % args.service_url)
        args.handle(parser, sandbox_client, args, sys.stdout)
    except KeyboardInterrupt:
        log.warning("\nCtrl-C interrupt, exit")
        sys.exit(2)
    except Exception as e:
        sys.stderr.write('ERR: %s, increase verbosity for more info\n' % str(e))
        if args.do_verbose >= 2:
            traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
