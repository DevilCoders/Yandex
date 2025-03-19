#!/usr/bin/env python3

import csv
import sys
import calendar
from datetime import datetime as dt

help_msg = 'CSV schedule generator for YC Support\n\nusage: python3 duty_generate.py [year] [month] ' + \
           '(args must be int)\nexample: ./duty_generate.py 2020 8 ' + \
           '\n\nmonth format: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12\n' + \
           'year format: 2020, 2021, ...\n'

try:
    in_year = int(sys.argv[1])
    in_month = int(sys.argv[2])
    if in_year < 2020 or in_month > 12:
        print(help_msg)
        quit()
except (IndexError, ValueError):
    print(help_msg)
    quit()

cal = calendar.Calendar()
DATES = [x for x in cal.itermonthdays(in_year, in_month)]
MONTH = [dt.strftime(dt(in_year, in_month, x), '%d/%m/%Y') for x in DATES if x != 0]
YEAR = []

for i in range(1, 13):
    for x in cal.itermonthdates(in_year, i):
        if x not in YEAR:
            YEAR.append(x)


def support_schedule_generator():
    duty_support_1 = []
    duty_support_2 = []
    count = 1

    days = [x for x in YEAR]

    while days:
        for _ in days:
            if count != 1 and count % 2 == 0:
                [duty_support_2.append(days.pop(0)) for _ in range(2)]
                [duty_support_1.append(days.pop(0)) for _ in range(2)]
                [duty_support_2.append(days.pop(0)) for _ in range(3)]
                count += 1
            else:
                [duty_support_1.append(days.pop(0)) for _ in range(2)]
                [duty_support_2.append(days.pop(0)) for _ in range(2)]
                [duty_support_1.append(days.pop(0)) for _ in range(3)]
                count += 1

    return duty_support_1, duty_support_2


def create_csv_schedule(schedule: tuple):
    """Generate CSV file and save it. Takees tuple schedule with two list of workdays"""
    support_day_1 = 'staff:apereshein'
    support_night_1 = 'staff:chronocross'
    support_day_2 = 'staff:zpaul'
    support_night_2 = 'staff:vr-owl'

    squad_1 = [dt.strftime(x, '%d/%m/%Y') for x in schedule[0]]
    squad_2 = [dt.strftime(x, '%d/%m/%Y') for x in schedule[1]]

    with open('yc_support.csv', 'w', encoding='utf-8') as outfile:
        fieldnames = ['Date', 'Primary', 'Backup']
        wr = csv.DictWriter(outfile, fieldnames=fieldnames)
        wr.writeheader()

        for date in MONTH:
            if date in squad_1:
                wr.writerow({'Date': date, 'Primary': support_day_1, 'Backup': support_night_1})
            elif date in squad_2:
                wr.writerow({'Date': date, 'Primary': support_day_2, 'Backup': support_night_2})

        outfile.close()

    print('Done')


if __name__ == '__main__':
    create_csv_schedule(support_schedule_generator())
