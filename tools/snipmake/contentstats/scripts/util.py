def read_lines(f):
    if isinstance(f, basestring):
        inf = open(f)
    else:
        inf = f
    for line in inf:
        line = line.strip()
        if not line:
            continue
        yield line

def load_owners(owner_file = None):
    result = set()
    if not owner_file:
        owner_file = '2ld.list'
    for line in read_lines(owner_file):
        result.add(line)
    #some hardcoded ones
    result.add('google.com')
    result.add('google.ru')
    result.add('yandex.com')
    result.add('yandex.ru')
    result.add('mail.ru')
    result.add('meta.ua')
    return result

def load_banlist():
    return set([
        'vkontakte.ru'
    ])

def get_query_from_reask(reask):
    if not reask:
        return ''
    fields = reask.split(':', 3)
    q = fields[3]
    if q.endswith(':balancer'):
        return q[:-9]
    return q

def owner_name(url, owner_names = None):
    if url.startswith('http://'):
        url = url[7:]
    elif url.startswith('https://'):
        url = url[8:]
    host = url.split('/', 1)[0]
    owner = host
    n = len(host)
    lvl = 0
    target_lvl = 2
    while n > 0:
        n = host.rfind('.', 0, n)
        cand = host[n+1:]
        lvl += 1
        if lvl == target_lvl:
            owner = cand
        if owner_names and cand in owner_names:
            target_lvl = lvl+1
    return owner

# output: tuples of (reqid, raw key-values, list of queries, list of urls)
# urls are tuples of (url, owner, source, query_index into list of queries)
def read_sample_table(f, owner_db = None):
    for line in read_lines(f):
        hsh, reqid, params = line.split('\t', 2)
        params = params.split('\t')
        kv = {}
        raw_urls = []
        for p in params:
            key, val = p.split('=', 1)
            if key == 'url':
                raw_urls.append(val)
            else:
                kv[key] = val
        original_query = kv['q'].strip()
        fixed_query = get_query_from_reask(kv.get('reask', ''))
        fixed_msp_query = get_query_from_reask(kv.get('msp', ''))
        if not fixed_query:
            fixed_query = original_query
        if not fixed_msp_query:
            fixed_msp_query = fixed_query
        queries = (fixed_query, fixed_msp_query)
        urls = []
        for url in raw_urls:
            src = 'WEB'
            if url[0].isupper:
                src, url = url.split(':', 1)
            if url.startswith('http://'):
                url = url[7:]
            query_index = 0
            if src in ('WEB_MISSPELL', 'WEB_MISSPELL_TDI'):
                query_index = 1
            urls.append((url, owner_name(url, owner_db), src, query_index))
        yield (reqid, kv, queries, urls)


