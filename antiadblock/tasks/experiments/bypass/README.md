# Скрипт для подготовки и сбора результатов bypass-эксперимента Антиадблока
```
./bypass.bin/bypass/bypass -h
usage: bypass [-h] [--test] [--daily_logs] [--service_id SERVICE_ID]
              [--start xxxx-x-xTx:x:x] [--percent INT] [--duration INT]

Tool for antiadblock bypass experiment: can collect experiment and control
uiniqids and calculate after experiment stats

optional arguments:
  -h, --help            show this help message and exit
  --test                Will send results to Stat testing
  --daily_logs          If endbled yql request will use logs/bs-dsp-log/1d
                        istead of logs/bs-dsp-log/stream/5min
  --service_id SERVICE_ID
                        Expects Antiadblock service_id
  --start xxxx-x-xTx:x:x
                        Date experiment was started
  --percent INT         Experimental uids percent
  --duration INT        Experiment duration
```
