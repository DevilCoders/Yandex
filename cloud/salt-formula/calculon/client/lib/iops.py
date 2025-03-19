import subprocess as sp
import json
import logging
import os
import jinja2
import sys

IOPS_LOG = logging.getLogger(__name__)
logging.basicConfig(filename='./calc-iops.log', level=logging.DEBUG, format='%(asctime)s %(message)s')


def render(tpl, context):
  IOPS_LOG.info("Rendering fio configs...")
  """Simpy renders message from jinja template
  :param tpl: str, path to template
  :param context: dict, placeholders
  """
  path,filename = os.path.split(tpl)
  return jinja2.Environment(loader=jinja2.FileSystemLoader(path)).get_template(filename).render(context)


def make_fio_initials(disks):
  """Generate benchmark configs
  :param disks: list, disks installed in node
  """
  tpl_path = os.path.abspath('/etc/calculon-client/io_tests.tpl')
  cfg_dir = os.path.abspath('/tmp/fio/jobs')
  if not os.path.exists(cfg_dir):
    os.makedirs(cfg_dir)
  paths = []
  io_types = ['read', 'write']
  profiles = ['', 'rand']
  if os.listdir(cfg_dir):
    for fname in os.listdir(cfg_dir):
      os.remove(os.path.abspath(os.path.join(cfg_dir,fname)))
  for profile in profiles:
    for io_type in io_types:
      fname = os.path.join(cfg_dir, '{}_{}.ini'.format(profile if profile else 'seq', io_type))
      IOPS_LOG.info("Got new file to make: {}. Trying to render...".format(fname))
      data = render(tpl_path, {'type': io_type, 'disks': disks, 'profile': profile})
      with open(fname, 'w') as fio_config:
        fio_config.write(data)
        IOPS_LOG.info("{} successully written".format(fname))
        paths.append(os.path.abspath(fname))
  return paths


def run_test(cfg_path, duration, dump_dir):
  """Run fio with particular load profile
  :param cfg_path: str, path to config
  :param duration: int, test duration in seconds
  :param dump_dir: str, path to artifacts directory
  """
  dump_dir = os.path.abspath(dump_dir)
  if not os.path.exists(cfg_path):
    IOPS_LOG.error("No fio configs")
    sys.exit(1)
  if not os.path.exists(dump_dir):
    os.makedirs(dump_dir)
  test_cmdline = '/usr/bin/fio {} --runtime {} --output {} --output-format=json'
  cmd = None
  dump_name = os.path.join(dump_dir, os.path.basename(cfg_path.replace('ini', 'json')))
  IOPS_LOG.info("Got dump name: {}".format(dump_name))
  test_cmdline = test_cmdline.format(cfg_path, duration, dump_name)
  IOPS_LOG.info("Trying to start test: {}".format(test_cmdline))
  try:
    cmd = sp.Popen(test_cmdline.split(), stdout=sp.PIPE, stderr=sp.PIPE)
    cmd.communicate()
  except sp.CalledProcessError as error:
    IOPS_LOG.error("Can't run '{}' because of: {}".format(test_cmdline, error))
  if cmd:
    if cmd.returncode == 0:
        IOPS_LOG.info("Dumping artifacts to {}".format(dump_name))
  return


def get_data_from_json(job_data, iotype):
  """
  :param iotype, str
  :param job_data: list of dicts
  :returns list of dicts
  """
  data = []
  usefull = {'iops': None}
  for el in job_data:
    usefull['iops'] = int(el[iotype]['iops'])
    data.append(usefull)
  return data


def process_results(disks, dumps_path):
  results = {}
  for path in os.listdir(dumps_path):
    fname = os.path.join(dumps_path, path)
    with open(fname) as dump:
      test_output = None
      try:
        test_output = json.loads(dump.read())
      except Exception as error:
        IOPS_LOG.error("Can't parse test data, malformed JSON: {}".format(error))
      if test_output:
        disks_order = [str(disk['name']) for disk in test_output['disk_util']]
        basename = os.path.basename(fname).replace('.json', '')
        keyname = "iops{}".format(basename.title().replace('_', ''))
        iotype = basename.split('_')[-1]
        data = None
        try:
          data = get_data_from_json(test_output['jobs'], iotype)
        except Exception as error:
          IOPS_LOG.error("Can't get data from json: {}".format(error))
        if data:
          disks_info = {el[0]: el[1] for el in zip(disks_order, data)}
          for disk in disks_order:
            disk_name = ''.join(filter(lambda x: disk in x, disks))
            iops = int(disks_info[disk]['iops'])
            results.setdefault(disk_name, {})
            results[disk_name][keyname] = iops
  for disk, data in results.iteritems():
    res_path = os.path.join('/opt/benchmarks/results/iops/', '{}.json'.format(disk))
    with open(res_path, 'w') as result:
      result.write(json.dumps(data))
  return


def get_iops(duration, artifacts_dir, disks):
  IOPS_LOG.info('Getting iops, results will be in {}, estimate {} seconds'.format(artifacts_dir, duration))
  paths = make_fio_initials(disks)
  for path in paths:
    run_test(path, duration, artifacts_dir)
  process_results(disks, artifacts_dir)
  return
