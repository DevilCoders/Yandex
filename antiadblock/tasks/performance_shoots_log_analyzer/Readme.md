# Performance shoots log analyzer

Показывает статистику по duration'ам по каждому action'у.

1. Скачать логи `CRYPROX logs for shoot 0/1 session_...` из ресурсов sandbox таски
2. Запустить таску:
  ```
    $ ya make && ./performance_shoots_log_analyzer --shoot-0 <path to the first log> --shoot-1 <path to the second log>
  ```

Можно запускать, указывая только первый лог, если разница не интересует.

## Пример

```
vsalavatov@i107248757:~/arcadia/antiadblock/tasks/performance_shoots_log_analyzer$ ya make
Ok
vsalavatov@i107248757:~/arcadia/antiadblock/tasks/performance_shoots_log_analyzer$ cd ppp1
vsalavatov@i107248757:~/arcadia/antiadblock/tasks/performance_shoots_log_analyzer/ppp1$ ../performance_shoots_log_analyzer --shoot-0 s00,s01 --shoot-1 s10,s11
INFO [2021-07-07 11:08:52,925]  Reading logs...
INFO [2021-07-07 11:09:01,266]  s00: 152881 good, 126 bad lines
INFO [2021-07-07 11:09:09,589]  s01: 152874 good, 152 bad lines
INFO [2021-07-07 11:09:17,997]  s10: 152879 good, 140 bad lines
INFO [2021-07-07 11:09:26,175]  s11: 152866 good, 180 bad lines

s00,s01:
                        action  count       p50        p75       p90        p95         p99        p99.9
0                   decrypturl  85660     263.0     309.00     350.0     378.00      433.00      668.000
1                    chkaccess  85763      64.0     156.00     377.0     522.00      763.00     1442.952
2                fetch_content  42138    3723.0    9484.00   21893.0   34530.70    98815.43  1201102.016
3                  handler_cry  59638    5465.0   11106.00   24831.9   37159.45   195102.46  1260591.501
4                    cache_get  17504    2921.0    5462.25   11156.4   17706.85   770083.38  1250858.442
5                    bodycrypt   2219    1914.0    2546.00   61677.6   66346.90    71714.86    78217.852
6                  bodyreplace   2220    8393.0   10442.25   25956.7   29155.75    35250.80    39603.543
7   inject_autoredirect_script   1564       4.0       4.00       5.0       5.00       20.00       46.925
8                   js_replace   1355    1955.0    2221.00    2723.6    3134.90     4322.12     7042.908
9           get_crypted_cookie   6642     202.0     229.00     255.0     272.00      306.00      657.795
10      nanpu_response_process    340     143.5     255.25     328.4     466.55     6849.17     8355.574
11       handler_detect_script     98   14291.0   20776.75   31437.5   65483.80  1369073.13  1443340.113
12             loaded_js_crypt      6   16572.5   16837.00   18234.0   18919.00    19467.00    19590.300
13        adb_function_replace    312    5622.0    6111.00    6908.8    7344.10     8325.42     9900.336
14         adb_cookies_replace     12     579.0     687.00     739.8     746.05      750.01      750.901
15       handler_crypt_content    284  101706.0  107366.00  111350.9  114808.35   124112.59   126356.397

s10,s11:
                        action  count       p50        p75       p90        p95        p99        p99.9
0                   decrypturl  85664     258.0     304.00     346.0     372.00     428.00      560.674
1                    chkaccess  85757      63.0     153.00     370.0     510.00     754.00     1395.000
2                fetch_content  42134    3859.0    9705.25   21990.0   34358.70   96125.10  1126489.280
3                  handler_cry  59640    5701.0   11555.75   24739.1   37129.10  152103.62  1185975.256
4                    cache_get  17500    3035.5    5682.00   11569.6   17597.70  609655.42  1197358.416
5                    bodycrypt   2220    1854.0    2508.25   61504.9   67025.20   72729.13    77310.377
6                  bodyreplace   2220    8209.0   10110.50   26387.2   29443.05   33503.66    39400.409
7   inject_autoredirect_script   1564       4.0       4.00       5.0       5.00      21.00       32.437
8                   js_replace   1354    1932.5    2182.75    2721.8    3135.35    4021.64     5854.277
9           get_crypted_cookie   6642     202.0     228.00     255.0     272.95     307.59      571.206
10      nanpu_response_process    340     136.5     256.25     321.0     413.25    6804.83     7458.185
11       handler_detect_script     98   14217.5   22215.75   26247.5   29733.00   50791.39    51675.739
12             loaded_js_crypt      6   16188.0   16753.75   19033.0   20156.00   21054.40    21256.540
13        adb_function_replace    310    5651.0    6232.25    7238.1    7812.50    9394.48    10403.196
14         adb_cookies_replace     12     557.5     607.25     732.8     765.35     784.27      788.527
15       handler_crypt_content    284  102361.0  107964.25  112453.6  116011.05  121464.25   125253.631

diff:
                        action count_0 count_1       p99_0      p99_1   diff (%)
0                   decrypturl   85660   85664      433.00     428.00  -1.154734
1                    chkaccess   85763   85757      763.00     754.00  -1.179554
2                fetch_content   42138   42134    98815.43   96125.10  -2.722581
3                  handler_cry   59638   59640   195102.46  152103.62 -22.039107
4                    cache_get   17504   17500   770083.38  609655.42 -20.832544
5                    bodycrypt    2219    2220    71714.86   72729.13   1.414309
6                  bodyreplace    2220    2220    35250.80   33503.66  -4.956313
7   inject_autoredirect_script    1564    1564       20.00      21.00   5.000000
8                   js_replace    1355    1354     4322.12    4021.64  -6.952144
9           get_crypted_cookie    6642    6642      306.00     307.59   0.519608
10      nanpu_response_process     340     340     6849.17    6804.83  -0.647378
11       handler_detect_script      98      98  1369073.13   50791.39 -96.290089
12             loaded_js_crypt       6       6    19467.00   21054.40   8.154312
13        adb_function_replace     312     310     8325.42    9394.48  12.840914
14         adb_cookies_replace      12      12      750.01     784.27   4.567939
15       handler_crypt_content     284     284   124112.59  121464.25  -2.133821
```

