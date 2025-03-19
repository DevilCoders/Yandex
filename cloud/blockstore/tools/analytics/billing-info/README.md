# Get billing info by cloud ids

Get token https://yql.yandex-team.ru/oauth

```bash
export YC_BASTION_TOKEN=<token>
python3 billing-info.py
```

Script reads cloud ids from /tmp/clouds.csv, get billing info and save result to /tmp/clouds_res.csv
