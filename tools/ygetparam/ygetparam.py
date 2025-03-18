#!/usr/bin/env python
# $Id$
# $HeadURL$

'''
Full Description:
https://wiki.yandex-team.ru/DljaAdminov/SearchAdmin/BigRobot/Antispam/ygetparam
'''

import os
import sys
import argparse
import textwrap
import imp
import json
import socket
import re
import collections
import logging
import errno
import tempfile
import time
import datetime

import curly_braces_expander as cbe

MANIFESTS_LOCATION = "/Berkanavt/instances"
MANIFEST_ENCODING = "utf8"
MANIFEST_SUFFIX = ".inst"
MY_HOSTNAME = socket.gethostname()

LEX_VAR_OPEN = '@@('.encode(MANIFEST_ENCODING)
LEX_VAR_CLOSE = ')'.encode(MANIFEST_ENCODING)
LEX_G_SEARCH = '^'.encode(MANIFEST_ENCODING)
LEX_DELIMITER = '.'.encode(MANIFEST_ENCODING)
LEX_ENV_DELIMITER = '_'.encode(MANIFEST_ENCODING)

LEX_MODULE_OPEN = '~'.encode(MANIFEST_ENCODING)
LEX_MODULE_CLOSE = '://'.encode(MANIFEST_ENCODING)
MODULE_FOLDER = 'ygetparam_modules'
MODULE_INIT = "Module"

LEX_DELIMITER_IN_STRING = ' '

reAllowedInstName = re.compile("^[a-zA-Z0-9_]+$".encode(MANIFEST_ENCODING))
reAllowedParamName = re.compile("^[a-zA-Z0-9_.-]+$".encode(MANIFEST_ENCODING))

recursionVector = list()
instanceCache = dict()

expandCurlyBraces = True
expandExternalModules = True

specVarSelf = str()     # @@(^self) - name of service

externalModules = dict()

LOG_MSG_FORMAT = '%(levelname)s: %(message)s'

EXTERNAL_STORAGE_CONF = {'etcd': {
                         'port': 80,
                         'host': 'ygetparam.n.yandex-team.ru',
                         'handler': 'LoadJSONEtcd',
                         'manifest_fmt': '/{}/manifest'}}
EXTERNAL_STORAGE = 'etcd'
LOCAL_CACHE_STORAGE = os.path.join(os.path.expanduser('~'), '.ygetparam')
LOAD_CACHE = False
CACHE_EXPIRATION_INTERVAL = 60 * 60 * 4
CACHE_EXPIRATION_TIMESTAMP = int(time.time() - CACHE_EXPIRATION_INTERVAL)

logHandler = logging.StreamHandler()
logHandler.setFormatter(logging.Formatter(LOG_MSG_FORMAT))
log = logging.getLogger()
log.addHandler(logHandler)


# Exceptions
class YGetParamException(Exception):
    '''Base exception class.'''
    pass


class BadInstNameException(YGetParamException):
    pass


class BadParamNameException(YGetParamException):
    pass


class MissingClosingLexException(YGetParamException):
    pass


class MissingComponentNameException(YGetParamException):
    pass


class UndefinedVariableException(YGetParamException):
    pass


class UndfinedSectionException(YGetParamException):
    pass


class InfinityRecursionException(YGetParamException):
    pass


class MultidimensionalListException(YGetParamException):
    pass


class ExtModNotFoundException(YGetParamException):
    pass


class ExtModUndefinedClassException(YGetParamException):
    pass


class ExtModUndefinedMethodException(YGetParamException):
    pass


class ExtModResponseFormatException(YGetParamException):
    pass


class NoSuchCachedFileException(YGetParamException):
    pass


class OutdatedCacheException(YGetParamException):
    pass


class CacheFileCloseException(YGetParamException):
    pass


class DuplicatedKeyException(YGetParamException):
    pass


def YGetParam(instance, param):
    ''' str, str -> str
    API request handler
    '''
    return GetParam(LoadJSON(ManifestFileLocation(instance)), param)


def YGetResolvedManifest(instance):
    ''' str -> TManifest
    API get resolved manifest
    '''
    class TManifest(collections.Mapping):
        def __init__(self, in_dict=None):
            self._root = dict() if in_dict is None else in_dict

        def __iter__(self):
            class It(collections.Iterator):
                def __init__(self, gen):
                    self.gen = gen

                def __iter__(self):
                    return It(self.gen)

                def next(self):
                    return self.gen.next()

            return It(self.keys())

        def __len__(self):
            return len(self._root)

        def __getitem__(self, name):
            ret = self._root[name]
            return self._to_manifest_if_mapping(ret)

        def __getattr__(self, name):
            return self[name]

        def get(self, name, default=None):
            ret = self._root.get(name, default)
            return self._to_manifest_if_mapping(ret)

        def items(self):
            for key, value in self._root.iteritems():
                yield key, self._to_manifest_if_mapping(value)

        def values(self):
            for value in self._root.itervalues():
                yield self._to_manifest_if_mapping(value)

        def keys(self):
            return self._root.iterkeys()

        @staticmethod
        def _to_manifest_if_mapping(value):
            return TManifest(value) if isinstance(value, collections.Mapping) else value

    if instance not in instanceCache:
        manifestFile = ManifestFileLocation(instance)
        instanceCache[instance] = GetResolvedManifest(LoadJSON(manifestFile), dict(), "")

    return TManifest(instanceCache[instance])


def multikey_json_hook(json_pairs_list):
    resulting_dict = dict()
    for key, value in json_pairs_list:
        if key in resulting_dict:
            raise DuplicatedKeyException('Error parsing json. Duplicated key: {}'.format(key))
        else:
            resulting_dict[key] = value
    return resulting_dict


def LoadJSON(manifestFile):
    ''' str -> dict
    Read and parse given file.
    '''
    if LEX_MODULE_CLOSE not in manifestFile:
        with open(manifestFile, 'r') as fd:
            manifest = json.load(fd, object_pairs_hook=multikey_json_hook)
        return manifest
    else:
        proto, instance = manifestFile.split(LEX_MODULE_CLOSE)
        handler = globals()[EXTERNAL_STORAGE_CONF[proto]['handler']]
        return handler(instance)


def ManifestFileLocation(instance):
    ''' str -> str
    Return absolute path to manifest file or a path to external storage
    if local file was not found.
    '''
    if not reAllowedInstName.match(instance):
        msg = "Incorrect instance name: {}"
        raise BadInstNameException(msg.format(instance))
    local_file = os.path.join(MANIFESTS_LOCATION, instance + MANIFEST_SUFFIX)
    if os.path.exists(local_file):
        return local_file
    else:
        return "{}{}{}".format(EXTERNAL_STORAGE, LEX_MODULE_CLOSE, instance)


def GetParam(manifest, param):
    ''' dict, str -> str/tuple
    Return single parameter value from manifest.
    '''
    if not reAllowedParamName.match(param):
        msg = "Incorrect parameter name: {}"
        raise BadParamNameException(msg.format(param))
    manifest = GetResolvedManifest(manifest, dict(), "")
    varList = dict()
    varList = CompileParamsList(manifest)
    if param in varList:
        if isinstance(varList[param], list):
            return tuple(varList[param])
        return varList[param]
    else:
        msg = "Parameter: {!r} is not defined."
        raise UndefinedVariableException(msg.format(param))


def GetAllParamsForEnv(manifest, delimiter, prefix):
    ''' dict, str, str -> list
    Return list of all available values prepared to be exported as environment variables
      component.param=[value,value2]
    will be transformed into
      SECTION_PARAM="value<delimiter>value2"

    Prefix would be uppercased and set in front of parameter name followed by underscore
    character.
    '''
    if prefix:
        prefix = prefix.upper() + LEX_ENV_DELIMITER
    manifest = GetResolvedManifest(manifest, dict(), "")
    varList = dict()
    varList = CompileParamsList(manifest)
    outputLines = list()
    for param in varList:
        paramName = param.replace(LEX_DELIMITER, LEX_ENV_DELIMITER).upper()
        if isinstance(varList[param], list):
            paramValue = delimiter.join(varList[param])
        else:
            paramValue = varList[param]
        if prefix:
            outputLines.append("{}{}={!r}".format(prefix, paramName, paramValue))
        else:
            outputLines.append("{}={!r}".format(paramName, paramValue))
    return outputLines


def GetPartOfParamsForEnv(manifest, delimiter, prefix, component):
    ''' dict, str, str, str -> list
    Return filtered list of exportable variables.
    '''
    envVars = GetAllParamsForEnv(manifest, delimiter, prefix)
    filterString = component.replace(LEX_DELIMITER, LEX_ENV_DELIMITER).upper()
    envVars = [var for var in envVars if filterString in var]
    return envVars


def GetResolvedManifest(manifest, varList, upperLayer):
    ''' dict, dict, str -> dict
    Create a new manifest with all resolved variables.
    '''
    if not varList:
        varList = ResolveAllVariables(CompileParamsList(manifest))
    retDict = dict()
    for currentLayer in manifest:
        currentLayer = currentLayer.encode(MANIFEST_ENCODING)
        if upperLayer == '':
            fullLayerPath = currentLayer
        else:
            fullLayerPath = upperLayer + LEX_DELIMITER + currentLayer
        resolvedVar = GetResolvedVariable(currentLayer, varList, upperLayer)
        if isinstance(resolvedVar, str):
            resolvedVar = (resolvedVar,)
        for item in resolvedVar:
            if isinstance(manifest[currentLayer], dict):
                resolvedManifest = GetResolvedManifest(manifest[currentLayer],
                                                       varList, fullLayerPath)
                if item not in retDict:
                    retDict[item] = resolvedManifest
                else:
                    retDict[item].update(resolvedManifest)
            else:
                retDict[item] = GetResolvedVariable(manifest[currentLayer],
                                                    varList, upperLayer)
    return retDict


def GetPartOfManifest(manifest, component):
    ''' dict, str -> dict '''
    if not reAllowedParamName.match(component):
        msg = "Incorrect component name: {}"
        raise BadParamNameException(msg.format(component))

    if LEX_DELIMITER not in component:
        if component in manifest:
            return {component.decode(MANIFEST_ENCODING): manifest[component]}
        else:
            msg = "Undifined component: {}"
            raise UndfinedSectionException(msg.format(component))

    currentSection, nextComponent = component.split(LEX_DELIMITER, 1)
    if currentSection in manifest:
        return GetPartOfManifest(manifest[currentSection], nextComponent)
    else:
        msg = "Undifined component: {}"
        raise UndfinedSectionException(msg.format(component))


def CompileParamsList(manifest):
    ''' dict -> dict
    Return value is a dict with complete path to variable as a key and value
    as a param's value.
    '''
    varList = {}
    _CompileParamsList(varList, manifest, '')
    varList[u"self"] = specVarSelf
    return varList


def _CompileParamsList(varList, manifest, prefix):
    # Flatten manifest into varList
    for key, value in manifest.iteritems():
        path = prefix + key
        if isinstance(value, dict):
            _CompileParamsList(varList, value, path + LEX_DELIMITER)
        else:
            if isinstance(value, list):  # is dict inside list allowed?
                if any(isinstance(item, list) for item in value):
                    msg = "Multidimensional lists aren't permitted. Found in {!r}."
                    raise MultidimensionalListException(msg.format(path))
            varList[path] = value

    # Propagate values from MY_HOSTNAME value to current level
    if isinstance(manifest.get(MY_HOSTNAME), dict):
        _CompileParamsList(varList, manifest[MY_HOSTNAME], prefix)


def GetResolvedVariable(var, varList, currentLayer):
    ''' str, dict, str -> str
        str, dict, str -> list (while recursive)
    Find Variables in 'var' and return its value from 'varList'
    '''
    if isinstance(var, list):
        # Merge deep tree into flat list().
        retVar = list()
        for item in (GetResolvedVariable(item, varList, currentLayer) for item in var):
            if isinstance(item, list):
                retVar += GetResolvedVariable(item, varList, currentLayer)
            else:
                retVar.append(item)
        return retVar

    var = unicode(var).encode(MANIFEST_ENCODING)
    # Braces Expansion in values
    # These contain spaces to single str(), others to list().
    if expandCurlyBraces and cbe.IsExpandable(var):
        if var.find(LEX_DELIMITER_IN_STRING) > 1:
            var = cbe.ExpandToStr(var)
        else:
            retVar = cbe.Expand(var)
            return [GetResolvedVariable(item, varList, currentLayer) for item in retVar]

    # Parse string
    lexStart = var.find(LEX_VAR_OPEN)
    if lexStart == -1:
        return var

    exprStart = lexStart + len(LEX_VAR_OPEN)

    var = var[0:exprStart] + GetResolvedVariable(var[exprStart:],
                                                 varList, currentLayer)
    lexEnd = var.find(LEX_VAR_CLOSE, exprStart)

    partBeforeVar = var[:lexStart]
    partAfterVar = var[lexEnd + len(LEX_VAR_CLOSE):]

    if lexEnd == -1:
        msg = "Missing {!r} for {!r} variable from {!r}."
        raise MissingClosingLexException(msg.format(LEX_VAR_CLOSE, var[lexStart:], currentLayer))
    if var[exprStart] == LEX_G_SEARCH:
        # Global variable
        shift = exprStart + len(LEX_G_SEARCH)
        retVar = GetResolvedVariable(var[shift:lexEnd], varList, currentLayer)
    elif var[exprStart] == LEX_MODULE_OPEN:
        if not expandExternalModules:
            return var
        # External module
        module_end = var.find(LEX_MODULE_CLOSE)
        module = var[exprStart + len(LEX_MODULE_OPEN):module_end]
        string = var[module_end + len(LEX_MODULE_CLOSE):var.find(LEX_VAR_CLOSE)]
        retVar = ExternalResolve(module, string)
        lst = [partBeforeVar + x + partAfterVar for x in retVar]
        for item in lst:
            if var in varList:
                varList[item] = varList[var]
        # Single value of tuple
        if len(lst) == 1:
            return lst[0]
        return lst
    else:
        # Local variable
        shift = exprStart
        retVar = GetResolvedVariable(var[shift:lexEnd], varList, currentLayer)
        retVar = LEX_DELIMITER.join([currentLayer, retVar])

    # Prevent Recursion loop
    if retVar in recursionVector:
        msg = "Infinity resursion in variable {!r}."
        raise InfinityRecursionException(msg.format(retVar))
    recursionVector.append(retVar)

    if retVar not in varList:
        msg = "Variable {!r} from {!r} is not defined."
        raise UndefinedVariableException(msg.format(retVar, currentLayer))

    # Check variable even deeply
    retVal = GetResolvedVariable(varList[retVar], varList, currentLayer)
    recursionVector.pop()

    if isinstance(retVal, list):
        if var.find(LEX_DELIMITER_IN_STRING) > 1:
            # String contain variable pointed to a param that is a list() value.
            # TEST: GetSingleExpandableToStringParam
            retVal = ''.join([
                             partBeforeVar,
                             LEX_DELIMITER_IN_STRING.join(retVal),
                             partAfterVar,
                             ])
            return retVal
        retVal = [x + partAfterVar for x in retVal]
        return retVal

    # Restore full string
    retVal = ''.join([
                     partBeforeVar,
                     retVal,
                     partAfterVar,
                     ])
    return retVal


def ResolveAllVariables(varList):
    ''' dict -> dict
    Resolve all availabe variables in a dictionary of params.
    '''
    # Expand Curly Braces first
    if expandCurlyBraces:
        items = dict()
        itemsToRemove = list()
        for i in varList:
            if cbe.IsExpandable(i):
                itemsToRemove.append(i)
                items.update({x: varList[i] for x in cbe.Expand(i)})
        for i in itemsToRemove:
            del varList[i]
        varList.update(items)

    for i in varList.keys():
        # Calc currentLevel
        if i.find(LEX_VAR_OPEN) != -1:
            # str betwen i[0] and last '.' before '@@('
            firstLexVar = i.find(LEX_VAR_OPEN)
            lastDelimiter = i[:firstLexVar].rfind(LEX_DELIMITER)
        else:
            # str betwen i[0] and last '.' in str
            lastDelimiter = i.rfind(LEX_DELIMITER)
        if lastDelimiter == -1:
            # String begins with variable definition on the top level.
            lastDelimiter = 0
        currentLevel = i[:lastDelimiter]

        # If param name was also a variable.
        j = GetResolvedVariable(i, varList, currentLevel)
        var = GetResolvedVariable(varList[i], varList, currentLevel)
        if i != j:
            if isinstance(j, list):
                '''
                return varList.update({x: var for x in j})
                '''
                # Avoid LEX_DELIMITER in case of variable on top level
                if len(currentLevel) > 0:
                    varList.update({LEX_DELIMITER.join([currentLevel, x]): var for x in j})
                else:
                    varList.update({x: var for x in j})
                return varList
            else:
                varList[j] = var
            del varList[i]
            i = j
        varList[i] = var

    return varList


def ExternalResolve(module, string):
    ''' str, str -> tuple
    Retrieve values from external source via dynamically loaded module.
    '''
    # Load module if necessary
    if module not in externalModules:
        externalModules[module] = ExternalModuleLoader(module)
    retVal = externalModules[module](string)
    if not isinstance(retVal, tuple):
        msg = "Module: {} returns data in {!r}. Expected 'tuple'."
        raise ExtModResponseFormatException(msg.format(module, retVal.__class__.__name__))
    for item in retVal:
        msg = "Module: {} returns incorrect data in 'tuple':\nElement [{}] has type of {}."
        if not isinstance(item, (str, unicode)):
            raise ExtModResponseFormatException(msg.format(module, retVal.index(item),
                                                           item.__class__.__name__))
    return retVal


def ExternalModuleLoader(moduleName):
    ''' str -> function
    Load and execute MODULE_INIT for specified module.
    This call should return main handler function (or object's method) itself.
    '''
    moduleName = moduleName.lower()
    try:
        packageName = "{}.{}".format(MODULE_FOLDER, moduleName)
        package = __import__(packageName)
        module = getattr(package, moduleName)
        handler = getattr(module, MODULE_INIT)
    except ImportError as error:
        msg = "Unable to load module {!r}. ({})."
        raise ExtModNotFoundException(msg.format(moduleName, error))
    return handler()


def LoadJSONEtcd(instance):
    ''' str -> str
    Load from Etcd
    '''
    # Do not use system etcd if any
    storage = 'etcd'
    for path in sys.path + [os.path.dirname(os.path.realpath(__file__))]:
        mdl_path = os.path.join(path, MODULE_FOLDER)
        if os.path.exists(mdl_path):
            try:
                imp.find_module('etcd')
            except ImportError:
                sys.path.insert(0, mdl_path)
            break
    import etcd
    connect = etcd.Client(host=EXTERNAL_STORAGE_CONF[storage]['host'],
                          port=EXTERNAL_STORAGE_CONF[storage]['port'],
                          allow_redirect=False)
    obj_name = EXTERNAL_STORAGE_CONF[storage]['manifest_fmt'].format(instance)
    try:
        value = connect.read(obj_name).value
        try:
            CacheInstance(instance, value)
        except Exception:
            log.warning('Got exception while caching', exc_info=True)
    except etcd.EtcdKeyNotFound as error:
        msg = 'Unable to load manifest. {}'
        raise type(error)(msg.format(error.message))
    except etcd.EtcdException as error:
        if LOAD_CACHE:
            log.warning('Could not connect to etcd: {}. Loading cached version.'.format(error))
            value = LoadCachedJSONEtcd(instance)
        else:
            raise
    return json.loads(value, object_pairs_hook=multikey_json_hook)


def CacheInstance(instance, value):
    try:
        os.makedirs(LOCAL_CACHE_STORAGE)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise
    cache_path = os.path.join(LOCAL_CACHE_STORAGE, instance)

    fd, temp_name = tempfile.mkstemp(dir=LOCAL_CACHE_STORAGE)
    try:
        os.write(fd, value)
        os.close(fd)
        os.rename(temp_name, cache_path)
    except Exception as cache_exception:
        try:
            os.remove(temp_name)
        except Exception:
            log.warning('Could not remove temp file {} while caching, got exception: '.format(temp_name), exc_info=True)
        raise cache_exception


def LoadCachedJSONEtcd(instance):
    cache_path = os.path.join(LOCAL_CACHE_STORAGE, instance)
    try:
        cache_stat = os.stat(cache_path)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise
        raise NoSuchCachedFileException("No cache file for {}".format(instance))
    cache_modification_time = cache_stat.st_mtime
    if cache_modification_time < CACHE_EXPIRATION_TIMESTAMP:
        raise OutdatedCacheException(
            'Cache for {} is outdated. Last update was at {}. Required at least {}'.
            format(instance,
                   datetime.datetime.fromtimestamp(cache_modification_time),
                   datetime.datetime.fromtimestamp(CACHE_EXPIRATION_TIMESTAMP))
        )

    with open(cache_path) as fin:
        cache_data = fin.read()

    log.warning('Loaded cached {} instance. Last update was at {}'.
                format(instance, datetime.datetime.fromtimestamp(cache_modification_time)))
    return cache_data


def run():
    # Parse cmd arguments
    args_parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description='Retrieve specified parameter value for given instance.',
        epilog=textwrap.dedent('''
            Examples:
                ygetparam -i mascot clustermaster.master.http_port
                ygetparam -m ./mascot.inst clustermaster.worker.http_port
            '''))
    args_group1 = args_parser.add_mutually_exclusive_group(required=True)
    args_group1.add_argument('-i', '--instance',
                             nargs=1,
                             help='search named manifests in ' +
                             MANIFESTS_LOCATION + '.')
    args_group1.add_argument('-m', '--manifest',
                             nargs=1,
                             help='use specified manifest file instead.')
    args_group1.add_argument('-M', '--module',
                             nargs=1,
                             help='module name for direct request over it.')
    args_group2 = args_parser.add_mutually_exclusive_group(required=True)
    args_group2.add_argument('parameter',
                             nargs='?',
                             help='parameter name.')
    args_group2.add_argument('-c', '--compile',
                             action='store_true',
                             help='output resolved manifest(formated JSON).')
    args_group2.add_argument('-t', '--test',
                             action='store_true',
                             help='verify manifest syntax.')
    args_group2.add_argument('-e', '--export',
                             action='store_true',
                             help='export all available variables as for evironment.')
    args_modificators = args_parser.add_mutually_exclusive_group()
    args_modificators.add_argument('-d', '--delimiter',
                                   type=str,
                                   default='\n',
                                   help='use delimiter for the printing list '
                                        'item separator [default:%(default)s]')
    args_modificators2 = args_parser.add_mutually_exclusive_group()
    args_modificators2.add_argument('-E', '--no-expand',
                                    action='store_true',
                                    help='do not expand lists defined by curly braces.')
    args_modificators3 = args_parser.add_mutually_exclusive_group()
    args_modificators3.add_argument('--prefix',
                                    type=str,
                                    default='',
                                    help='prefix to be set in varible names '
                                         '(use with -e)')
    args_modificators4 = args_parser.add_mutually_exclusive_group()
    args_modificators4.add_argument('-p', '--partial',
                                    type=str,
                                    help='display/export a part of the configuration tree.')
    args_modificators5 = args_parser.add_mutually_exclusive_group()
    args_modificators5.add_argument('-X', '--no-modules',
                                    action='store_true',
                                    help='Do not resolve externale modules.')
    args_modificators6 = args_parser.add_mutually_exclusive_group()
    args_modificators6.add_argument('--debug',
                                    action='store_true',
                                    help='Print traceback on exception.')
    args_parser.add_argument('--no-cache',
                             action='store_true',
                             default=False,
                             help='Don\'t use file cache for inst files')
    args_parser.add_argument('--cache-path',
                             default=os.path.join(os.path.expanduser('~'), '.ygetparam'),
                             help='Path to local cache storage directory')
    args_parser.add_argument('--cache-expiration-time',
                             metavar='SECONDS',
                             type=int,
                             default=CACHE_EXPIRATION_INTERVAL,
                             help='Do not load cache that was updated  more than these seconds ago')
    args = args_parser.parse_args()

    try:
        global LOAD_CACHE
        LOAD_CACHE = not args.no_cache
        global CACHE_EXPIRATION_TIMESTAMP
        CACHE_EXPIRATION_TIMESTAMP = int(time.time() - args.cache_expiration_time)
        global LOCAL_CACHE_STORAGE
        LOCAL_CACHE_STORAGE = args.cache_path
        global specVarSelf
        if args.manifest:
            manifestFile = args.manifest[0]
            specVarSelf = os.path.basename(manifestFile)
        elif args.instance:
            manifestFile = ManifestFileLocation(args.instance[0])
            specVarSelf = args.instance[0]
        if args.no_expand:
            global expandCurlyBraces
            expandCurlyBraces = False
        if args.no_modules:
            global expandExternalModules
            expandExternalModules = False
        if args.compile:
            manifest = GetResolvedManifest(LoadJSON(manifestFile), dict(), "")
            if args.partial:
                manifest = GetPartOfManifest(manifest, args.partial)
            print json.dumps(manifest, sort_keys=True, indent=4)
        elif args.test:
            GetResolvedManifest(LoadJSON(manifestFile), dict(), "")
        elif args.export:
            manifest = GetResolvedManifest(LoadJSON(manifestFile), dict(), "")
            if args.partial:
                envVars = GetPartOfParamsForEnv(manifest, args.delimiter, args.prefix, args.partial)
            else:
                envVars = GetAllParamsForEnv(manifest, args.delimiter, args.prefix)
            print '\n'.join(envVars)
        else:
            if args.module:
                value = ExternalResolve(args.module[0], args.parameter)
            else:
                value = GetParam(LoadJSON(manifestFile), args.parameter)
            if isinstance(value, tuple):
                print args.delimiter.join(value)
            else:
                print value
    except Exception as err:
        if hasattr(err, "__module__") and err.__module__ != "__main__":
            log.error("(in %s) %s", err.__module__, err)
        else:
            log.error("Unhandled exception!", exc_info=True)
        if args.debug:
            raise
        sys.exit(1)


if __name__ == "__main__":
    run()
