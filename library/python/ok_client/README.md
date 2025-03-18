# OK Service Client

- [OK Wiki](https://wiki.yandex-team.ru/Intranet/OK/)
- [OK Private API Description](https://wiki.yandex-team.ru/Intranet/OK/private-api/)

## Usage

### Creating an Approvement

```python
from library.python.ok_client import OkClient, CreateApprovementRequest, StageComplex, StageSimple

ok = OkClient(token="YOUR TOKEN")

# create an approvement
approvement_uuid = ok.create_approvement(payload=CreateApprovementRequest(
    type="tracker",  # Startrek
    object_id="TICKET-11",  # Startrek ticket key
    text="Approve this",  # Approvement description
    author="username",  # Staff login, approvement author
    groups=["group_1", "group_2"],  # Staff groups
    is_parallel=True,  # Is this a parallel approvement?
    stages=[
        StageSimple(approver="approver_1"),
        StageComplex(
            need_all=True,
            stages=[
                StageSimple(approver="approver_2"),
                StageSimple(approver="approver_3"),
            ],
        )
    ],
))

# get OK approvement URL for Startrek comment
# see https://wiki.yandex-team.ru/Intranet/OK/private-api/#vizualizacijasoglasovanija
iframe_url = ok.get_embed_url(approvement_uuid)

# You can now post something like
# f'{{{{iframe src="{iframe_url}" frameborder=0 width=100% height=400px scrolling=no}}}}'

# Some Startrek comment posting logic should follow
# See https://wiki.yandex-team.ru/Intranet/OK/private-api/#vizualizacijasoglasovanija
```


### Managing an Approvement

```python

# Get approvement info
info = ok.get_approvement(approvement_uuid)

# Suspend approvement
ok.suspend_approvement(approvement_uuid)

# Resume approvemtn
ok.resume_approvement(approvement_uuid)

# Close approvement
ok.close_approvement(approvement_uuid)

```
