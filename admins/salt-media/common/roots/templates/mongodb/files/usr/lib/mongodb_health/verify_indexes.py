#!/usr/bin/python
import json
import os
import sys
import logging
import basecheck
import pymongo
from bson.son import SON
from collections import OrderedDict, defaultdict
from pymongo import errors


def get_indexes(db, collection, server_major_ver, pymongo_major_ver):
    """
    Workaround for incompatible versions of MongoDB and PyMongo
    """
    if server_major_ver >= 3 and pymongo_major_ver < 3:
        logging.debug('Incompatible pymongo and mongodb versions')
        cursor = db.command('listIndexes', collection.name,
                            read_preference=pymongo.read_preferences.ReadPreference.SECONDARY)
        indexes = cursor['cursor']['firstBatch']
        logging.debug(repr(indexes))
        info = {}

        for index in indexes:
            index['key'] = index['key'].items()
            index = dict(index)
            info[index.pop('name')] = index
        return info

    else:
        logging.debug('Server and pymongo versions are compatible')

        return collection.index_information()


def find_missing_indexes(required_database_indexes, actual_database_indexes):
    """
    Comparing results from json config file and index information from database
    """

    required = defaultdict(dict)
    actual = defaultdict(dict)
    missing_indexes = []

    # processing required indexes with options
    for index in required_database_indexes:
        # sorting required index options by option name
        index_fields = tuple(sorted(index['index'].items(), key=lambda x: x[0]))
        index_options = tuple(sorted(index['opt'].items(), key=lambda x: x[0]))
        required[index_fields] = index_options

    # processing actual indexes with options
    for index_name in actual_database_indexes:
        # index_fields = tuple
        index = [(key, int(order)) for (key, order) in actual_database_indexes[index_name].pop('key')]
        # excluding ns, key and v elements from index dict returned from mongodb
        for k in ('ns', 'v'):
            actual_database_indexes[index_name].pop(k, None)
        # sorting actual index options by option name
        index_fields = tuple(sorted(index))
        index_options = tuple(sorted(actual_database_indexes[index_name].items(), key=lambda x: x[0]))
        actual[index_fields] = index_options

    for required_index in required:
        if required_index in actual:
            required_opts = required[required_index]
            actual_opts = actual[required_index]
            is_error = False

            for req_opt in required_opts:
                req_opt_k, req_opt_v = req_opt
                for (act_opt_k, act_opt_v) in actual_opts:
                    if act_opt_k == req_opt_k:
                        break
                    else:
                        act_opt_k = False

                if act_opt_k:
                    if req_opt_v != act_opt_v and req_opt_v != 'Any':
                        logging.debug('MISSING: wrong option "{}" ({} instead of {}) for index {}'.
                                      format(req_opt_k, act_opt_v, req_opt_v, repr(required_index)))
                        missing_indexes.append(required_index)
                        is_error = True
                else:
                    logging.debug('MISSING: option {} for index {}'.format(req_opt, repr(required_index)))
                    missing_indexes.append(required_index)
                    is_error = True

            if not is_error:
                logging.debug('OK: {}'.format(repr(required_index)))

        else:
            logging.debug('MISSING: {}'.format(repr(required_index)))
            missing_indexes.append(required_index)

    return missing_indexes


def verify_indexes(port=27018, user=None, password=None, jsondir='/etc/monitoring/mongodb_indexes/conf.d/', code=None):
    """
    Reading JSON config files with required indexes for databases.
    Getting current index information from MongoDB and comparing results
    """

    required_indexes = {}
    failed_namespaces = []
    actual_indexes = defaultdict(dict)
    missing_indexes = []
    pymongo_major_ver = int(pymongo.version_tuple[0])

    if code == '2':
        error_code = 2
    else:
        error_code = 0

    # reading config files and getting required indexes for databases
    for config_filename in os.listdir(jsondir):
        # parsing JSON files and saving information as OrderedDict object
        if config_filename.endswith('.json'):
            config_filename = os.path.join(jsondir, config_filename)
            try:
                with open(config_filename, 'r') as config_file:
                    required_index_desc = json.load(config_file, object_pairs_hook=OrderedDict)
                    required_indexes.update(required_index_desc)
            except (ValueError, IOError) as e:
                return '{};Failed;index config file {} ' \
                       'processing failed with: {}'.format(error_code, config_filename, e)

    # Returning error if no index checker config file found
    if not required_indexes:
        return '{};Warning;No index config file found in {}'.format(0, jsondir)

    # connecting to database
    opts = {}
    if pymongo_major_ver >= 3:
        pymongo_base_opts = {'serverSelectionTimeoutMS': 5000}
    else:
        pymongo_base_opts = {}
    opts.update(pymongo_base_opts)

    try:
        client = None
        pymongo_slave_opts = {'read_preference': pymongo.read_preferences.ReadPreference.SECONDARY}
        opts.update(pymongo_slave_opts)
        pymongo_docclass_opts = {'document_class': SON}
        opts.update(pymongo_docclass_opts)
        client = pymongo.MongoClient(["localhost:{}".format(port)], **opts)
        # getting info from server to check if it is alive
        server_version = client.server_info()['version']
        server_major_ver = int(server_version[0])

    except:
        exception = sys.exc_info()
        logging.debug('Exception: {}'.format(repr(exception)))
        return '1;Database connection failed with {}'.format(repr(exception))

    if user and password:
        db = client['admin']
        try:
            db.authenticate(user, password)
        except errors.PyMongoError as e:
            return '1;Auth failed'

    # Checking Replica Set status
    (status, master, self) = basecheck.getRSStatus(host='localhost', port=port)
    if status == 'error':
        return '1;Warning;Replica status: error'
    if status != 'no_rs' and self['state'] in (0, 3, 5, 9):
        return "1;Warning;Mongo state is %s, check, whether it is expected behaviour" % self['stateStr']

    # getting index information for databases described in JSON config file
    for database_name in required_indexes:
        logging.debug('Database: {}'.format(database_name))
        db = client[database_name]
        # collection = db[database_name]
        for collection_name in required_indexes[database_name]:
            collection = db[collection_name]
            try:
                indexes = get_indexes(db, collection, server_major_ver, pymongo_major_ver)
                logging.debug("indexes returned from mongodb:")
                for index in indexes:
                    logging.debug(repr(indexes[index]))
                actual_indexes[database_name][collection_name] = indexes

            # handling database selection failure or index query failure
            except errors.OperationFailure as e:
                logging.debug('Getting database {} indexes failed with {}'.format(database_name, e[0]))
                failed_namespaces.append("{}.{}".format(database_name, collection_name))

            # handling database connection failure
            except errors.ServerSelectionTimeoutError as e:
                logging.debug('Database connection failed  with {}'.format(e[0]))
                return '{};Failed;Database connection failed: {}'.format(error_code, e[0])

            # handling all other exceptions during database query
            except:
                exception = sys.exc_info()
                logging.debug('Exception: {}'.format(repr(exception)))
                return '{};Failed;Exception occurred {}'.format(error_code, exception[1])

    # comparing results from DB and configs
    logging.debug('***=== Check results ===***\n')
    for database in actual_indexes:
        for collection in actual_indexes[database]:
            logging.debug('Namespace: {}.{}'.format(database, collection))
            namespace_missing_indexes = find_missing_indexes(required_indexes[database][collection],
                                                             actual_indexes[database][collection])
            if namespace_missing_indexes:
                namespace = "{}.{}".format(database, collection)
                if namespace not in missing_indexes:
                    missing_indexes.append(namespace)
                logging.debug('Index comparison result. Missing or wrong indexes : {} '.
                              format(namespace_missing_indexes))

    logging.debug('Missing or failed namespaces: {}'.format(', '.join(failed_namespaces)))
    logging.debug('Missing indexes: {}'.format(', '.join(missing_indexes)))

    if missing_indexes and failed_namespaces:
        return '{};Failed;Missing or wrong indexes: {}.' \
               ' Check failed on namespaces: {}.'.format(error_code,
                                                         ', '.join(missing_indexes),
                                                         ', '.join(failed_namespaces))
    elif missing_indexes:
        return '{};Failed;Missing or wrong indexes in namespaces: {}.'.format(error_code, ', '.join(missing_indexes))
    elif failed_namespaces:
        return '{};Failed;Check failed on namespaces: {}.'.format(error_code, ', '.join(failed_namespaces))
    else:
        return '0;OK;All indexes present'


if __name__ == '__main__':
    INDEXCFG_DIR = '/etc/monitoring/mongodb_indexes/conf.d/'
    logging.basicConfig(level=logging.DEBUG, format='%(levelname)s %(message)s')
    result = verify_indexes(port=27018, code='2', jsondir=INDEXCFG_DIR)
    print 'Check returned: ' + str(result)
