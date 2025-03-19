#!/usr/bin/env python


def _mongod(port=27018):
    data = {}
    try:
        import pymongo
        options = dict((opt, 2 * 1000) for opt in ('connectTimeoutMS',
                                                   'socketTimeoutMS',
                                                   'serverSelectionTimeoutMS'))

        conn = pymongo.MongoClient(host='localhost', port=port, **options)
        doc = conn.admin.command('isMaster')
        replset_initialized = 'setName' in doc

        data['replset_initialized'] = replset_initialized
        data['is_standalone'] = not replset_initialized and not doc.get('isreplicaset', False)
        for item in ['ismaster', 'secondary']:
            data.update({item: doc.get(item)})

    except Exception:
        pass

    return data


def mongodb():
    services = {
        'mongod': 27018,
        'mongocfg': 27019,
    }
    report = {}
    for srv, port in services.items():
        report.update({srv: _mongod(port)})
    return {'mongodb': report}


if __name__ == '__main__':
    from pprint import pprint
    pprint(mongodb())
