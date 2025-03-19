#!/usr/bin/env python3
"""This module contains bot commands."""

import logging
import csv
import sys
import calendar
from datetime import datetime as dt, timedelta as td, time as tm
from services.abc2 import Shift,Abc2Client
from utils.config import Config
from core.constants import (DUTY_DAY1_STAFF, DUTY_DAY2_STAFF, DUTY_NIGHT1_STAFF, DUTY_NIGHT2_STAFF)

logger = logging.getLogger(__name__)

def schedule_generator(year,month,extend=False):

    in_month = int(month)
    in_year = int(year)
    in_day = int(dt.today().day if (dt.today().month == in_month and dt.today().year == in_year) else 0)
    cal = calendar.Calendar()
    DATES = [x for x in cal.itermonthdays(in_year, in_month) if x >= in_day]
    MONTH = [dt.strftime(dt(in_year, in_month, x), '%d/%m/%Y') for x in DATES if x != 0]
    if extend:
        DATES2 = [x for x in cal.itermonthdays(in_year, in_month + 1)]
        MONTH += [dt.strftime(dt(in_year, in_month + 1, x), '%d/%m/%Y') for x in DATES2 if x != 0]
    YEAR = []
    for i in range(1, 13):
        for x in cal.itermonthdates(in_year, i):
            if x not in YEAR:
                YEAR.append(x)    
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
    return duty_support_1, duty_support_2, MONTH

def print_schedule(schedule: tuple):
    """Generate CSV file and save it. Takees tuple schedule with two list of workdays"""
    support_day_primary_1 = DUTY_DAY1_STAFF[0] #'cameda'
    support_day_backup_1 = DUTY_DAY1_STAFF[1] #'krestunas'
    support_night_primary_1 = DUTY_NIGHT1_STAFF[0] #'teminalk0'
    support_night_backup_1 = DUTY_NIGHT1_STAFF[1] #'d-penzov'
    support_day_primary_2 = DUTY_DAY2_STAFF[0] #'v-pavlyushin'
    support_day_backup_2 = DUTY_DAY2_STAFF[1] #'hercules'
    support_night_primary_2 = DUTY_NIGHT2_STAFF[0] #'dymskovmihail'
    support_night_backup_2 = DUTY_NIGHT2_STAFF[1] #'snowsage'
    squad_1 = [dt.strftime(x, '%d/%m/%Y') for x in schedule[0]]
    squad_2 = [dt.strftime(x, '%d/%m/%Y') for x in schedule[1]]
    
    schedule_on_request = []
    for date in schedule[2]:
        if date in squad_1:
            elem = date, ', ', support_day_primary_1, ', ', support_day_backup_1, ', ', support_night_primary_1, ', ', support_night_backup_1
            string_elem = ''
            for i in elem:
                string_elem = string_elem + i
            schedule_on_request.append(string_elem)
        elif date in squad_2:
            elem = date, ', ', support_day_primary_2, ', ', support_day_backup_2, ', ', support_night_primary_2, ', ', support_night_backup_2
            string_elem = ''
            for i in elem:
                string_elem = string_elem + i
            schedule_on_request.append(string_elem)
    
    return(schedule_on_request)

def push_schedule(schedule: tuple):
    """Generate CSV file and save it. Takees tuple schedule with two list of workdays"""
    support_day_primary_1 = DUTY_DAY1_STAFF[0] #'cameda'
    support_day_backup_1 = DUTY_DAY1_STAFF[1] #'krestunas'
    support_night_primary_1 = DUTY_NIGHT1_STAFF[0] #'teminalk0'
    support_night_backup_1 = DUTY_NIGHT1_STAFF[1] #'d-penzov'
    support_day_primary_2 = DUTY_DAY2_STAFF[0] #'v-pavlyushin'
    support_day_backup_2 = DUTY_DAY2_STAFF[1] #'hercules'
    support_night_primary_2 = DUTY_NIGHT2_STAFF[0] #'dymskovmihail'
    support_night_backup_2 = DUTY_NIGHT2_STAFF[1] #'snowsage'
    squad_1 = [dt.strftime(x, '%d/%m/%Y') for x in schedule[0]]
    squad_2 = [dt.strftime(x, '%d/%m/%Y') for x in schedule[1]]
    slot_ids = []
    a = Abc2Client(timeout=60)
    slot_ids = a.get_slot_ids(Config.SCHEDULE_ID)
    a.tvmclient.stop()
    del a

    
    if slot_ids is None:
        slot_ids = [1728, 1729, 1730, 1731]
    schedule_on_request = []
    for date in schedule[2]:
        j = 0
        if date in squad_1:
            elem = [ support_day_primary_1, support_day_backup_1, support_night_primary_1, support_night_backup_1 ]
            times = [ dt.combine(dt.strptime(date, '%d/%m/%Y'), tm(7)),
                     dt.combine(dt.strptime(date, '%d/%m/%Y'), tm(19)), 
                     dt.combine(dt.strptime(date, '%d/%m/%Y') + td(days=1), tm(7)) 
                    ]
            for i in elem:
                schedule_on_request.append(Shift(slot_id=slot_ids[j],
                                start=times[0] if j < 2 else times[1],
                                end=times[1] if j < 2 else times[2],
                                staff_login=i,
                                is_primary=False if j % 2 else True).to_dict()
                         )
                j = j + 1
#            schedule_on_request.append(Shift(slot_id=slot_ids[1],
#                                start=dt.combine(date, tm(7)),
#                                end=dt.combine(date, tm(19)),
#                                staff_login=support_day_backup_1,
#                                is_primary=False).to_dict()
#                         )
#            schedule_on_request.append(Shift(slot_id=slot_ids[2],
#                                start=dt.combine(date, tm(19)),
#                                end=dt.combine(date + td(days=1), tm(7)),
#                                staff_login=support_night_primary_1,
#                                is_primary=True).to_dict()
#                         )
#            schedule_on_request.append(Shift(slot_id=slot_ids[3],
#                                start=dt.combine(date, tm(19)),
#                                end=dt.combine(date + td(days=1), tm(7)),
#                                staff_login=support_night_backup_1,
#                                is_primary=False).to_dict()
#                         )
            #schedule_on_request.append({support_day_primary_1, support_day_backup_1, support_night_primary_1, support_night_backup_1})
        elif date in squad_2:
            elem = [ support_day_primary_2, support_day_backup_2, support_night_primary_2, support_night_backup_2 ]
            times = [ dt.combine(dt.strptime(date, '%d/%m/%Y'), tm(7)),
                     dt.combine(dt.strptime(date, '%d/%m/%Y'), tm(19)), 
                     dt.combine(dt.strptime(date, '%d/%m/%Y') + td(days=1), tm(7)) 
                    ]
            for i in elem:
                schedule_on_request.append(Shift(slot_id=slot_ids[j],
                                start=times[0] if j < 2 else times[1],
                                end=times[1] if j < 2 else times[2],
                                staff_login=i,
                                is_primary=False if j % 2 else True).to_dict()
                         )
                j = j + 1
#            schedule_on_request.append(Shift(slot_id=slot_ids[1],
#                                start=dt.combine(date, tm(7)),
#                                end=dt.combine(date, tm(19)),
#                                staff_login=support_day_backup_2,
#                                is_primary=False).to_dict()
#                         )
#            schedule_on_request.append(Shift(slot_id=slot_ids[2],
#                                start=dt.combine(date, tm(19)),
#                                end=dt.combine(date + td(days=1), tm(7)),
#                                staff_login=support_night_primary_2,
#                                is_primary=True).to_dict()
#                         )
#            schedule_on_request.append(Shift(slot_id=slot_ids[3],
#                                start=dt.combine(date, tm(19)),
#                                end=dt.combine(date + td(days=1), tm(7)),
#                                staff_login=support_night_backup_2,
#                                is_primary=False).to_dict()
#                         )
            #schedule_on_request.append({support_day_primary_2, support_day_backup_2, support_night_primary_2, support_night_backup_2})
             
    
    a = Abc2Client(timeout=60)
    test = []
    
    logger.info(f'Start uploading shifts to abc2.0, count: {len(schedule_on_request)}')
    
    chunk_size = 100
    for i in range(0, len(schedule_on_request), chunk_size):
        result = False
        while result != None:
            logger.info(f'Trying to upload shifts, chunks: {i}-{i+chunk_size}')
            test.append(f'Trying to upload shifts, chunks: {i}-{i+chunk_size}')
            result = a.upload_shifts(schedule_id=Config.SCHEDULE_ID,shifts=schedule_on_request[i:i+chunk_size])
        test.append(result or "OK")
        
    logger.info(f'Done uploading all shifts to abc2.0')
    a.tvmclient.stop()
    del a
    return(test)
#    return(schedule_on_request)
