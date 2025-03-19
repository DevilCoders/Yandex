## katan - auto-deploy for mdb dataplane

### imp
It imports clusters with theirs metadata from different sources (`metadb`, `....`) to `katandb`

### katan
It run rollouts

### scheduler
Create rollouts based on `katan.schedules`

### cluster rollout transitions

![dot source] [states]

[states]: https://jing.yandex-team.ru/files/wizard/rollout_states.svg

