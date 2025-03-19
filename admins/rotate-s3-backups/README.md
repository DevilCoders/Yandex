# rotate-backups
rotate s3 backups

# Если вдруг захотелось проверить работу алгоритма
Воспользуйся тестами
```
skacheev-nix:rotate-s3-backups (master) λ nosetests
............
----------------------------------------------------------------------
Ran 12 tests in 10.439s

OK
```
Или поиграйся с визуализатором
```
./visualize_steps.py --help
usage: visualize_steps.py [-h] [--start YYYMMDD] --che YYYYMMDD [YYYYMMDD ...]
                          [-p PERIOD] [-d DAILY] [-w WEEKLY] [-m MONTHLY]

Visualize "rotate-s3-backups"

optional arguments:
  -h, --help            show this help message and exit
  --start YYYMMDD       Start from time (default 20170903)
  --che YYYYMMDD [YYYYMMDD ...]
                        Time check points
  -p PERIOD, --period PERIOD
                        Backup period in days (def=365 days)
  -d DAILY, --daily DAILY
                        Policy daily (def=5)
  -w WEEKLY, --weekly WEEKLY
                        Policy weekly (def=1)
  -m MONTHLY, --monthly MONTHLY
                        Policy monthly (def=1)


skacheev-nix:rotate-s3-backups (master) λ ./visualize_steps.py --che 20170906 20171217 20181006 -d 1 -w 5 -m 5 -p 720
Start time 2017-09-03
Time check points: ['2017-09-06', '2017-12-17', '2018-10-06']
Backups for 2017-09-06
['2017-09-03', '2017-09-04', '2017-09-06']
Backups for 2017-12-17
['2017-09-03',
 '2017-10-01',
 '2017-11-01',
 '2017-11-06',
 '2017-11-13',
 '2017-11-20',
 '2017-11-27',
 '2017-12-01',
 '2017-12-04',
 '2017-12-11',
 '2017-12-17']
Backups for 2018-10-06
['2018-04-01',
 '2018-05-01',
 '2018-06-01',
 '2018-07-01',
 '2018-08-01',
 '2018-08-27',
 '2018-09-01',
 '2018-09-03',
 '2018-09-10',
 '2018-09-17',
 '2018-09-24',
 '2018-10-01',
 '2018-10-06']
skacheev-nix:rotate-s3-backups (master) λ cal 2018 -m 9
   September 2018
Mo Tu We Th Fr Sa Su
                1  2
 3  4  5  6  7  8  9
10 11 12 13 14 15 16
17 18 19 20 21 22 23
24 25 26 27 28 29 30
