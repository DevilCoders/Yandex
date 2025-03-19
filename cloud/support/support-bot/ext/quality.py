#!/usr/bin/env python3
"""This module contains bot commands."""

import logging

import csv
import sys
import calendar
import random
from datetime import datetime as dt

from services.startrek import StartrekClient

logger = logging.getLogger(__name__)

def quality_generator(login,month,days,year):
    """Generate 30 random tickets for login"""
    year = year
    month = month
    day_first = days[0]
    day_last = days[1]
    login = login
    tickets = []
    issues = StartrekClient().search_issues(f'Queue: CLOUDSUPPORT and Status: Closed and "Comment Author": {login} and components: !"квоты" and \
     "Created": >{year}-{month}-{day_first} and Resolved: <{year}-{month}-{day_last}') or []
    for ticket in issues:
        tickets.extend(StartrekClient().get_comments_for_author(ticket.key,login))
    if len(tickets) >= 50:
        tickets = random.sample(tickets, 50)
    create_csv_quality(tickets,login)
    message = '\n'.join(tickets)
    return message
    
def create_csv_quality(tickets: list,login: str):
    """"Generate CSV file and save it. Takees tickets and send for telegram"""
    with open(f'csv/{login}.csv', 'w', encoding='utf-8') as outfile:
        wr = csv.writer(outfile)
        for ticket in tickets:
            wr.writerow([ticket])
        outfile.close()
    logger.info(f'Создан файл {login}.csv')
