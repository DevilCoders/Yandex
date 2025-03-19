A client library for bot.yandex-team.ru API.

Details: https://wiki.yandex-team.ru/bot/api/


How to setup:

```bash
#client
python setup.py bdist_wheel && pip install dist/byc-1.0-py3-none-any.whl
#cli
cd ./bot_cli
python setup.py bdist_wheel && pip install dist/bh-1.0-py3-none-any.whl
```

Tested with Python 3.7.2

How to use:

```bash
❯ bh
Usage: bh [OPTIONS] COMMAND [ARGS]...

Options:
  --help  Show this message and exit.

Commands:
  servers
  write-config
```

```bash
❯ bh servers --help
Usage: bh servers [OPTIONS] [IDS]...

Options:
  -t, --total   Calculate total amount of resources
  -m, --memory  List of memory (RAM)
  -d, --disks   List of disks
  --of TEXT     Output formats: ['plain', 'tabulate', 'ttabulate', 'json', 'csv']

  -f TEXT       Comma (,) separated list of fields
  --help        Show this message and exit.
```

Examples:
```bash
❯ bh servers  -t sas2-9179.search.yandex.net s49sas.storage.yandex.net
         INV                         FQND   DC UNITSIZE               CPUMODEL NUMCPU PHISICALCORES RAMSLOTS     RAMTYPE DISKSLOTS DISKINTERFACE DISKFORM_INCH  SSD_COUNT  SSD_TOTAL  HDD_TOTAL  HDD_COUNT SSD_LIST                  HDD_LIST  RAM_TOTAL                                          RAM_LIST
0  900918364  sas2-9179.search.yandex.net  SAS        1  INTEL XEON E5-2650 V2      2            16       16  DDR3ECCREG         4          SATA           3.5          0          0       8000          4       []  [2000, 2000, 2000, 2000]        128  [8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8]
1  900918002    s49sas.storage.yandex.net  SAS        1  INTEL XEON E5-2650 V2      2            16       16  DDR3ECCREG         4          SATA           3.5          1        240       6000          3    [240]        [2000, 2000, 2000]        128                  [16, 16, 16, 16, 16, 16, 16, 16]

```

```bash
❯ bh servers --of tabulate 900918363 900918364
+----+-----------+-----------------------------+------+------------+-----------------------+----------+-----------------+------------+------------+-------------+-----------------+-----------------+-------------+-------------+-------------+-------------+----------------------+--------------------------+-------------+--------------------------------------------------+
|    |       INV | FQND                        | DC   |   UNITSIZE | CPUMODEL              |   NUMCPU |   PHISICALCORES |   RAMSLOTS | RAMTYPE    |   DISKSLOTS | DISKINTERFACE   |   DISKFORM_INCH |   SSD_COUNT |   SSD_TOTAL |   HDD_TOTAL |   HDD_COUNT | SSD_LIST             | HDD_LIST                 |   RAM_TOTAL | RAM_LIST                                         |
|----+-----------+-----------------------------+------+------------+-----------------------+----------+-----------------+------------+------------+-------------+-----------------+-----------------+-------------+-------------+-------------+-------------+----------------------+--------------------------+-------------+--------------------------------------------------|
|  0 | 900918363 | bstat70i.yandex.ru          | SAS  |          1 | INTEL XEON E5-2650 V2 |        2 |              16 |         16 | DDR3ECCREG |           4 | SATA            |             3.5 |           4 |        2880 |           0 |           0 | [480, 800, 800, 800] | []                       |         128 | [16, 16, 16, 16, 16, 16, 16, 16]                 |
|  1 | 900918364 | sas2-9179.search.yandex.net | SAS  |          1 | INTEL XEON E5-2650 V2 |        2 |              16 |         16 | DDR3ECCREG |           4 | SATA            |             3.5 |           0 |           0 |        8000 |           4 | []                   | [2000, 2000, 2000, 2000] |         128 | [8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8] |
+----+-----------+-----------------------------+------+------------+-----------------------+----------+-----------------+------------+------------+-------------+-----------------+-----------------+-------------+-------------+-------------+-------------+----------------------+--------------------------+-------------+--------------------------------------------------+
```

```bash
❯ bh servers --of ttabulate 900918363 900918364
+---------------+----------------------------------+--------------------------------------------------+
|               | 0                                | 1                                                |
|---------------+----------------------------------+--------------------------------------------------|
| INV           | 900918363                        | 900918364                                        |
| FQND          | bstat70i.yandex.ru               | sas2-9179.search.yandex.net                      |
| DC            | SAS                              | SAS                                              |
| UNITSIZE      | 1                                | 1                                                |
| CPUMODEL      | INTEL XEON E5-2650 V2            | INTEL XEON E5-2650 V2                            |
| NUMCPU        | 2                                | 2                                                |
| PHISICALCORES | 16                               | 16                                               |
| RAMSLOTS      | 16                               | 16                                               |
| RAMTYPE       | DDR3ECCREG                       | DDR3ECCREG                                       |
| DISKSLOTS     | 4                                | 4                                                |
| DISKINTERFACE | SATA                             | SATA                                             |
| DISKFORM_INCH | 3.5                              | 3.5                                              |
| SSD_COUNT     | 4                                | 0                                                |
| SSD_TOTAL     | 2880                             | 0                                                |
| HDD_TOTAL     | 0                                | 8000                                             |
| HDD_COUNT     | 0                                | 4                                                |
| SSD_LIST      | [480, 800, 800, 800]             | []                                               |
| HDD_LIST      | []                               | [2000, 2000, 2000, 2000]                         |
| RAM_TOTAL     | 128                              | 128                                              |
| RAM_LIST      | [16, 16, 16, 16, 16, 16, 16, 16] | [8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8] |
+---------------+----------------------------------+--------------------------------------------------+
```

```bash
❯ bh servers -t 900918365 900918329 900918363 900918365 100377911 100371893 100377277
+----+-----------+--------------------------+------+------------+-----------------------+-----------------+------------+------------+-------------+-----------------+-------------+-------------+-------------+-------------+--------------------------+------------+-------------+--------------------------------------------------+
|    |       INV | FQND                     | DC   |   UNITSIZE | CPUMODEL              |   PHISICALCORES |   RAMSLOTS | RAMTYPE    |   DISKSLOTS | DISKINTERFACE   |   SSD_COUNT |   SSD_TOTAL |   HDD_TOTAL |   HDD_COUNT | SSD_LIST                 | HDD_LIST   |   RAM_TOTAL | RAM_LIST                                         |
|----+-----------+--------------------------+------+------------+-----------------------+-----------------+------------+------------+-------------+-----------------+-------------+-------------+-------------+-------------+--------------------------+------------+-------------+--------------------------------------------------|
|  0 | 900918363 | bstat70i.yandex.ru       | SAS  |          1 | INTEL XEON E5-2650 V2 |              16 |         16 | DDR3ECCREG |           4 | SATA            |           4 |        2880 |           0 |           0 | [480, 800, 800, 800]     | []         |         128 | [16, 16, 16, 16, 16, 16, 16, 16]                 |
|  1 | 900918365 | bstat71i.yandex.ru       | SAS  |          1 | INTEL XEON E5-2650 V2 |              16 |         16 | DDR3ECCREG |           4 | SATA            |           4 |        2880 |           0 |           0 | [480, 800, 800, 800]     | []         |         128 | [16, 16, 16, 16, 16, 16, 16, 16]                 |
|  2 | 100371893 | bstat117e.yabs.yandex.ru | IVA  |          1 | INTEL XEON E5-2660    |              16 |         16 | DDR3ECCREG |           4 | SATA            |           4 |        3200 |           0 |           0 | [800, 800, 800, 800]     | []         |         128 | [8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8] |
|  3 | 900918329 | bstat58i.yandex.ru       | SAS  |          1 | INTEL XEON E5-2650 V2 |              16 |         16 | DDR3ECCREG |           4 | SATA            |           4 |        2880 |           0 |           0 | [800, 800, 480, 800]     | []         |         128 | [16, 16, 16, 16, 16, 16, 16, 16]                 |
|  4 | 100377277 | bstat34e.yabs.yandex.ru  | IVA  |          1 | INTEL XEON E5-2660    |              16 |         16 | DDR3ECCREG |           4 | SATA            |           4 |        3200 |           0 |           0 | [800, 800, 800, 800]     | []         |         128 | [8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8] |
|  5 | 100377911 | bstat40e.yabs.yandex.ru  | IVA  |          1 | INTEL XEON E5-2660    |              16 |         16 | DDR3ECCREG |           4 | SATA            |           4 |        7680 |           0 |           0 | [1920, 1920, 1920, 1920] | []         |         128 | [8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8] |
+----+-----------+--------------------------+------+------------+-----------------------+-----------------+------------+------------+-------------+-----------------+-------------+-------------+-------------+-------------+--------------------------+------------+-------------+--------------------------------------------------+
TOTAL:
+----+---------+-----------------+-------------+-------------+-------------+-------------+-------------+
|    |   COUNT |   PHISICALCORES |   RAM_TOTAL |   SSD_COUNT |   SSD_TOTAL |   HDD_COUNT |   HDD_TOTAL |
|----+---------+-----------------+-------------+-------------+-------------+-------------+-------------|
|  0 |       6 |              96 |         768 |          24 |       22720 |           0 |           0 |
+----+---------+-----------------+-------------+-------------+-------------+-------------+-------------+
```

```bash
❯ bh servers --of json -t sas2-9179.search.yandex.net s49sas.storage.yandex.net | jq '.total'
{
  "COUNT": 2,
  "HDD_COUNT": 7,
  "HDD_TOTAL": 14000,
  "PHISICALCORES": 32,
  "RAM_TOTAL": 256,
  "SSD_COUNT": 1,
  "SSD_TOTAL": 240
}
```
```bash
❯ bh servers --of json -d 900918365 900918329 | jq '[ .servers[]|.DC + "/"+.FQDN +"/"+.CPUMODEL+"/"+.PHISICALCORES+"/"+ (.RAM_TOTAL|tostring) + "/"+ (.SSD_TOTAL|tostring) +"/" +.SSD_LIST + "/" +(.HDD_TOTAL|tostring)]'
[
  "SAS/bstat71i.yandex.ru/INTEL XEON E5-2650 V2/16/128/2880/[480, 800, 800, 800]/0",
  "SAS/bstat58i.yandex.ru/INTEL XEON E5-2650 V2/16/128/2880/[800, 800, 480, 800]/0"
]
```