#! /usr/bin/env python2.7
# -*- coding: utf-8 -*-

from optparse import OptionParser
import getpass
import sys
import time
import traceback
import xmlrpclib
import commands
import os

parser = OptionParser()
parser.add_option("-c", "--cgi", dest = "cgi", help = "CGI-parameters", default = "")
parser.add_option("-b", "--basesearch", dest = "basesearch", help = "Path to testing basesearch binary")
parser.add_option("-t", "--task", dest = "task_id", help = "Task from Sandbox with type GET_BASESEARCH_RESPONSES (will be used for initialization reasons)")
parser.add_option("--no-bs", action = "store_true", dest = "no_bs", help = "Binary from iniatioalization task will be executed")
parser.add_option("-s", "--snip", action = "store_true", dest = "snip", help = "Testing one stage queries (snippet phase only)")
parser.add_option("-o", "--owner", dest = "owner", help = "Owner of this task (all notifications will be sent to him)", default = getpass.getuser())
parser.add_option("-n", "--notify", action = "store_true", dest = "notify", help = "Notify about intermediate tasks",)

def is_failed(status):
    return status in ['FAILURE',
                      'NOT_READY',
                      'STOPPED',
                      'DELETED']


def is_in_progress(status):
    return status in ['ENQUEUED',
                      'EXECUTING',
                      'UNKNOWN',
                      'WAIT_CHILD']


def retry_standard_times(f):
    return retry(f, 10, 5, True)


def retry(f, max_attempts, delay, print_errors):
    """Выполнить операцию f.
    В случае ошибки повторить выполнение заданное кол-во раз с заданными
    паузами.
    Если все повторы были неуспешными, то возвратить ошибку"""
    attempt = 0
    while True:
        attempt += 1
        try:
            return f()
        except Exception, e:
            sys.stderr.write('\nError happened: {0}\n'.format(e))
            if attempt >= max_attempts:
                raise Exception("Error while using RPC")
            elif print_errors:
                traceback.print_exc(file=sys.stderr)
            sys.stderr.write('Reconnecting in {0} seconds... {1} attempts left.\n'.format(delay, max_attempts - attempt))
            sys.stderr.flush()
            time.sleep(delay)

def WaitForTask(con, task_id):
    while True:
        status = retry_standard_times(lambda : con.getTaskStatus(task_id))
        if not is_in_progress(status):
            break
        sys.stderr.write('.')
        sys.stderr.flush()
        time.sleep(1)

    sys.stderr.flush()
    if is_failed(status):
        raise Exception("Task {0} is failed.\n".format(task_id))


def LoadNewQueries(queries_id, owner, cgi):
    cgi = cgi.strip()
    if cgi[0] != "&":
        cgi = '&' + cgi

    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    queryUrl = retry_standard_times(lambda : con.getResource(queries_id))['url']
    parts = queryUrl.split('/')
    file = parts[len(parts) - 1]

    commands.getstatusoutput('wget ' + queryUrl)
    queries = open(file, 'r')
    newQueriesPath = 'new_queries.txt'
    newQueries = open(newQueriesPath, 'w')
    while 1:
        query = queries.readline()
        if not query:
            break
        query = query.strip()
        newQueries.write(query + cgi + '\n')
    queries.close()
    newQueries.close()

    skynet_id = commands.getoutput('sky share ' + newQueriesPath)

    if options.notify is not None:
        notify_via = owner
    else:
        notify_via = ''

    new_task_id = retry_standard_times(lambda : con.createTask({'type_name' : 'REMOTE_COPY_RESOURCE',
        'owner' : owner,
        'descr' : 'patch queries from {0} resource'.format(queries_id),
        'ctx' : {'resource_type' : 'PLAIN_TEXT_QUERIES',
            'remote_file_name' : skynet_id,
            'notify_via' : notify_via,
            'notify_if_failed' : owner
            }
        })
    )

    sys.stderr.write('Patched queries are uploading ')
    try:
        WaitForTask(con, new_task_id)
        sys.stderr.write('\nNew queries were uploaded.\n\n')
    except Exception:
        print "Some problems happened during patched queries uploading. See {0} task.".format(TaskUrl(new_task_id))
        sys.exit(1)
    finally:
        commands.getstatusoutput('rm ' + file + ' ' + newQueriesPath)

    return new_task_id

def LoadBinaryFile(fileToBin, owner):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    skynet_id = commands.getoutput('sky share ' + fileToBin)

    if options.notify is not None:
        notify_via = owner
    else:
        notify_via = ''

    new_task_id = retry_standard_times(lambda : con.createTask({'type_name' : 'REMOTE_COPY_RESOURCE',
        'owner' : owner,
        'descr' : 'basesearch binary',
        'ctx' : {'resource_type' : 'BASESEARCH_EXECUTABLE',
            'remote_file_name' : skynet_id,
            'notify_via' : notify_via,
            'notify_if_failed' : owner
            }
        })
    )

    sys.stderr.write('Binary file is uploading ')
    try:
        WaitForTask(con, new_task_id)
        sys.stderr.write('\nBinary file was uploaded.\n')
    except Exception:
        traceback.print_exc(file=sys.stderr)
        print 'Some problems happened during binary uploading. See {0} task.'.format(TaskUrl(new_task_id))
        sys.exit(0)

    return new_task_id

def StartBin(binary_id, config_id, db_id, queries_id, modelId, owner, descr):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')

    if options.notify is not None:
        notify_via = owner
    else:
        notify_via = ''

    return retry_standard_times(lambda : con.createTask({'type_name' : 'GET_BASESEARCH_RESPONSES',
        'owner' : owner,
        'descr' : descr,
        'ctx' : {'executable_resource_id' : binary_id,
            'config_resource_id' : config_id,
            'search_database_resource_id' : db_id,
            'queries_resource_id' : queries_id,
            'notify_via' : notify_via,
            'notify_if_failed' : owner,
            'models_archive_resource_id' : modelId
            }
        })
    )

def StartComparer(oldRunId, newRunId, owner):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    if options.snip is None:
        max_diff_size = 0
    else:
        max_diff_size = 500000000
    return retry_standard_times(lambda : con.createTask({'type_name' : 'COMPARE_BASESEARCH_RESPONSES',
        'owner' : owner,
        'descr' : 'bs comparer',
        'ctx' : {'basesearch_responses1_resource_id' : oldRunId,
            'basesearch_responses2_resource_id' : newRunId,
            'omit_responses_check' : True,
            'max_diff_size' : max_diff_size
            }
        })
    )

def GetResourceId(task_id, resourceName):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    keys = retry_standard_times(lambda : con.getTask(task_id))
    resources = keys['ctx']
    resource = retry_standard_times(lambda : con.getResource(resources[resourceName]))
    return resource['id']

def TaskExists(task_id):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    return retry_standard_times(lambda : con.taskExists(task_id))

def TaskType(task_id):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    task = retry_standard_times(lambda : con.getTask(task_id))
    return task['type_name']

def TaskUrl(task_id):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    return retry_standard_times(lambda : con.getTaskUrl(task_id))

def CheckForRequired(param, message):
    if (param is None) or (len(param.strip()) == 0):
        print message + '\n\tCheck --help for the full list of params'
        return False
    return True

# for not it's pretty stupido way. TODO
def InitTaskId(descrPref):
    con = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc')
    tasks = retry_standard_times(lambda : con.listTasks({'task_type': 'GET_BASESEARCH_RESPONSES', 'limit': 500, 'show_childs': True, 'hidden' : True, 'status' : 'FINISHED'}))
    for i in tasks:
        if (i['owner'] == 'testengine') and (i['descr'].startswith(descrPref)):
            return i['id']
    raise Exception('500 weren\'t enough to find task for initialization.')

if __name__ == "__main__":
    ret = 0
    try:
        (options, args) = parser.parse_args()

        # Check required params
        if CheckForRequired(options.cgi, 'CGI params are required (-c option).') == False:
            sys.exit(ret)

        # Check if binary file exists
        if options.no_bs is None:
            if CheckForRequired(options.basesearch, 'basesearch is required (-b and --no-bs options).') == False:
                sys.exit(ret)
            tmp = options.basesearch.split('/')
            if len(tmp) != 1:
                print '-b is required just the name of binary file (in current folder).'
                sys.exit(ret)

            if os.path.exists(options.basesearch) == False:
                print "Basesearch binary   \'{0}\'   doesn't exist".format(options.basesearch)
                sys.exit(ret)

        # Init task_id (from user or from one of existing tasks)
        if options.task_id is None:
            if options.snip is None:
                options.task_id = InitTaskId('base-trunk: RESPONSES_BASESEARCH ')
            else:
                options.task_id = InitTaskId('base-trunk: RESPONSES_SNIPPETS ')

            print 'Using {0} task for initialization ({1}).'.format(options.task_id, TaskUrl(options.task_id))

        # Check if user task exists
        if TaskExists(options.task_id) == False:
            print "No such task ({0})\n".format(options.task_id)
            sys.exit(ret)

        # Check if type of this task is OK
        taskType = TaskType(options.task_id)
        if taskType != 'GET_BASESEARCH_RESPONSES':
            print "This task {0} has wrong type: {1}".format(options.task_id, taskType)
            sys.exit(ret)

        # Get all IDs
        config_id = GetResourceId(options.task_id, 'config_resource_id')
        db_id = GetResourceId(options.task_id, 'search_database_resource_id')
        queries_id = GetResourceId(options.task_id, 'queries_resource_id')
        model_id = GetResourceId(options.task_id, 'models_archive_resource_id')

        # Upload all resources
        newQueries_id = GetResourceId(LoadNewQueries(queries_id, options.owner, options.cgi), 'result_resource_id')
        if options.no_bs is True:
            sys.stderr.write('Using task\'s binary.\n')
            binary_id = GetResourceId(options.task_id, 'executable_resource_id')
        else:
            sys.stderr.write('Using local binary.\n')
            binary_id = GetResourceId(LoadBinaryFile(options.basesearch, options.owner), 'result_resource_id')

        # Run all main tasks. We don't wait for results - just get the ID and ULR
        oldRunId = StartBin(binary_id, config_id, db_id, queries_id, model_id, options.owner, 'origin run')
        newRunId = StartBin(binary_id, config_id, db_id, newQueries_id, model_id, options.owner, 'modified run')
        comparerId = StartComparer(GetResourceId(oldRunId, 'out_resource_id'),
            GetResourceId(newRunId, 'out_resource_id'), options.owner)

        print "Comparer task ID - {0}".format(comparerId)
        print "Url of this task:\n{0}".format(TaskUrl(comparerId))
    except Exception:
        traceback.print_exc(file=sys.stderr)
        ret = -1
    sys.exit(ret)
