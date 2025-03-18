startrek_client
================

**startrek_client** provides python interface to [Startrek v2 API](https://wiki.yandex-team.ru/tracker/api/).

Requirements (according to your distro) 
------------

- Alpine 3: `gcc musl-dev libffi-dev openssl-dev`

Installation
------------

**startrek_client** could be installed by pip:

from pypi

```bash
pip install -i https://pypi.yandex-team.ru/simple/ startrek_client
```

or from git

```bash
pip install git+git://github.yandex-team.ru/tools/startrek-python-client.git
```

Configuring
-----------

Startrek API requires OAuth token for access. It could be generated there:

https://oauth.yandex-team.ru/authorize?response_type=token&client_id=a7597fe0fbbf4cd896e64f795d532ad2

(click "Confirm" and you will see your OAuth token in the ```token``` parameter of
URL where you will be redirected)

NOTE: For some browsers (Safari confirmed), this link may redirect to an empty
page. In such a case, use another browser to obtain a token.

Startrek test api requires /etc/ssl/certs/ca-certificates.crt:
```bash
curl https://crls.yandex.net/YandexInternalRootCA.crt
```

Ubuntu:
```bash
apt-get install yandex-internal-root-ca
```

Usage
-----

To use client you need to import model classes:
```python
from startrek_client import Startrek

client = Startrek(useragent=<user-agent>, base_url=<base-url>, token=<token>)
```
There are two versions of Startrek API: 'v2' (default) and 'service'. 'service' version allows
to authorize by impersonate TVM-ticket. You can change version by passing 'api_version' param:
```python
from startrek_client import Startrek
from startrek_client.settings import VERSION_SERVICE

client = Startrek(api_version=VERSION_SERVICE, useragent=<user-agent>, base_url=<base-url>, token=<token>)
```

**Getting issue:**
```python
issue = client.issues['STARTREK-42']
print issue.deadline, issue.updatedAt
```

Handling 404 exceptions:
```python
try:
    issue = client.issues['STARTREK-43']
except NotFound:
    pass
```

**Creating issue:**
```python
client.issues.create(
    queue='STARTREK', 
    summary='API Test Issue', 
    type={'name': 'Bug'}, 
    description='**wiki markup here**'
)
```

**Updating issue:**
```python
issue = client.issues['STARTREK-42']
issue.update(summary='East or west, Startrek is the best', priority='minor')
```

**Obtaining list of transitions:**
```python
transitions = issue.transitions.get_all()
for transition in transitions:
  print transition
```

**Executing transition:**
```python
issue = client.issues['STARTREK-42']
issue.transitions['close'].execute()
```
Executing transition with comment and resolution:
```python
issue = client.issues['STARTREK-42']
transition = issue.transitions['close']
transition.execute(comment='Fixed', resolution='fixed')
```

**Searching for issues:**
```python
issues = client.issues.find('Queue: Стартрек Assignee: me()')
print [issue.key for issue in issues]
```

Using the 'filter' parameter possible to pass the parameters of the filtering as a dictionary:
```python
issues = client.issues.find(
    filter={'queue': 'STARTREK', 'assignee': 'me()', 'created': {'from': '2015-03-02'}},
    order=['update','-status', '+priority'],
    per_page=15
)
print [issue.key for issue in issues]
```


```
Deprecated, now you can use find method to get more than 10k issues see 
https://wiki.yandex-team.ru/tracker/api/issues/list/#polucheniebolshogokolichestvatiketov
```

Method ```find``` allows to get at most 10000 issues. It's possible to get more issues by using
```relative_scroll``` method, but it works only in 'service' version of api.
```python
issues_filter = {'queue': 'STARTREK', 'assignee': 'me()', 'created': {'from': '2015-03-02'}},
issues_first_page = client.issues.relative_scroll(filter=issues_filter)
issues_next_page = client.issues.relative_scroll(filter=issues_filter, from_id=issues_first_page[-1].id)
```

**Queue info:**
```python
queue = client.queues['STARTREK']
```
or:
```python
queue = client.issues['STARTREK-42'].queue
```

**Queue list:**
```python
queues = client.queues.get_all()[:3]
```

**List issue attachments:**
```python
attachments = client.issues['STARTREK-5'].attachments
```

**Downloading attachments to specified directory:**
```python
[attachment.download_to('some/path') for attachments in client.issues['STARTREK-5'].attachments.get_all()]
```

**Uploading an attachment**
```python
issue = client.issues['STARTREK-42']
issue.attachments.create('path/to/file')
```

**Deleting an attachment**
```python
issue = client.issues['STARTREK-42']
issue.attachments[42].delete()
```
or
```python
client.attachments[42].delete()
```

**List issue comments:**
```python
issue = client.issues['STARTREK-42']
comments = list(issue.comments.get_all())[:3]
```

**Add comment:**
```python
issue = client.issues['STARTREK-42']
comment = issue.comments.create(text='Test Comment')
```

**Add comment with attachments:**
```python
issue = client.issues['STARTREK-42']
comment = issue.comments.create(text='Test comment', attachments=['path/to/file1', 'path/to/file2'])
```

**Update comment:**
```python
issue = client.issues['STARTREK-42']
comment = issue.comments[42]
comment.update(text='New Text')
```

**Deleting a comment:**
```python
issue = client.issues['STARTREK-42']
comment = issue.comments[42]
comment.delete()
```

**List issue links:**
```python
issue = client.issues['STARTREK-42']
links = issue.links
```

**Add link:**
```python
issue = client.issues['STARTREK-42']
link = issue.links.create(issue='TEST-42', relationship='relates')
```

**Deleting a link:**
```python
issue = client.issues['STARTREK-42']
link = issue.links[42]
link.delete()
```

**Add remote link:**
```python
issue = client.issues['STARTREK-42']
link = issue.remotelinks.create(origin="ru.yandex.lunapark", key="STARTREK-42", relationship="relates")
```

### Advanced Usage

**Bulk update:**
```python
bulkchange = client.bulkchange.update(
    'STARTREK-42', 'STARTREK-43', 'STARTREK-44'],
    priority='minor',
    tags={'add': ['minored']})
print bulkchange.status
bulkchange = bulkchange.wait()
print bulkchange.status
```

**Bulk transition:**
```python
bulkchange = client.bulkchange.transition(
    ['STARTREK-42', 'STARTREK-43'], 'need_info', priority='minor')
bulkchange.wait()
```

**Bulk move:**
```python
bulkchange = client.bulkchange.move(['STARTREK-42', 'STARTREK-43'], 'TEST')
bulkchange.wait()
```

**Using in B2B:**
Pass organization ID in x_org_id param.
```python
issue = client.issues.get('STARTREK-42', x_org_id=287)
```

```python
client.issues.create(
    params={'x_org_id': 287},
    queue='STARTREK',
    summary='API Test Issue',
    type={'name': 'Bug'},
    description='**wiki markup here**'
)
```


## Upgrading To Newer Releases
### Upgrading to 1.0.x
```python
def get_issue(client, id):
    return client.issues[id]

def create_issue(client, **params):
    return client.issues.create(**params)

def update_issue(client, id, **params):
    return client.issues[id].update(**params)

def find_issues(client, query, limit=50):
    return client.issues.find(query, perPage=limit)

def get_transition(client, issue, id):
    return client.issues[issue].transitions[id]

def get_transitions(client, issue):
    return client.issues[issue].transitions.get_all()

def get_comment(client, issue, id):
    return client.issues[issue].comments[id]

def add_comment(client, issue, text, *args):
    return client.issues[issue].comments.create(text=text, attachments=args)

def update_comment(client, issue, id, text, *args):
    return client.issues[issue].comments[id].update(text=text, attachments=args)

def get_comments(client, issue):
    return client.issues[issue].comments.get_all()

def delete_comment(client, issue, id):
    return client.issues[issue].comments[id].delete()

def get_link(client, issue, id):
    return client.issues[issue].links[id]

def get_links(client, issue):
    return client.issues[issue].links.get_all()

def add_link(client, issue, relationship, to):
    return client.issues[issue].links.create(relationship, to)

def delete_link(client, issue, id):
    return client.issues[issue].links[id].delete()

def link(client, issue, resource, relationship):
    return client.issues[issue].link(resource, relationship)

def unlink(client, issue, resource, relationship):
    return client.issues[issue].unlink(resource, relationship)

def get_attachment(client, issue, id):
    return client.issues[issue].attachments[id]

def get_attachments(client, issue):
    return client.issues[issue].attachments.get_all()

def get_changelog(client, issue, sort='asc', fields=(), types=()):
    return client.issues[issue].changelog.get_all(sort=sort, field=fields, type=types)

def get_changelog_entry(client, issue, id):
    return client.issues[issue].changelog[id]

def upload_attachment(client, issue, file):
    return client.issues[issue].attachments.create(file)

def delete_attachment(client, issue, id):
    return client.issues[issue].attachments[id].delete()

def get_queue(client, id):
    return client.queues[id]

def get_queues(client):
    return client.queues.get_all()

def get_issue_types(client):
    return client.issue_types.get_all()

def get_priorities(client):
    return client.priorities.get_all()

def get_statuses(client):
    return client.statuses.get_all()

def get_resolutions(client):
    return client.resolutions.get_all()

def get_components(client):
    return client.components.get_all()

def get_queue_components(client, queue):
    return client.queues[queue].components

def get_versions(client):
    return client.versions.get_all()

def get_queue_versions(client, queue):
    return client.queues[queue].versions

def get_projects(client):
    return client.projects.get_all()

def get_queue_projects(client, queue):
    return client.queues[queue].projects

def get_queue_issue_types(client, queue):
    return client.queues[queue].issuetypes
```
