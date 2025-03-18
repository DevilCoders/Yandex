#!/usr/bin/env python3
import argparse
import os
import smtplib
import sys


SMTP_HOST_PORT="smtp.yandex-team.ru:587"
LOGIN="robot-ne-robot"


def parse_args():
    parser = argparse.ArgumentParser(description='Send mail from YP')
    parser.add_argument('-S', '--subject', help='subject', required=True)
    parser.add_argument('to', type=str, nargs='+', help='To')
    opts = parser.parse_args()
    return opts


def main():
    args = parse_args()
    host, port = SMTP_HOST_PORT.split(':')
    s=smtplib.SMTP(host, port)
    s.starttls()
    s.login(LOGIN, os.environ.get("EMAIL_HOST_PASSWORD"))
    msg = f'''\
From: {LOGIN}@yandex-team.ru
Subject: {args.subject}

'''
    msg += sys.stdin.read()
    s.sendmail(f"{LOGIN}@yandex-team.ru", [f"{user}@yandex-team.ru" for user in args.to], msg)
    s.quit()


if __name__ == '__main__':
    main()
