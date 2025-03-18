#!/usr/bin/env python

import sys

import argparse
from crontab import CronTab
from datetime import datetime
import sched
import time
import subprocess
import traceback
import shlex
import platform
from collections import namedtuple
import mail_it
import getpass


def ParseArgs():
    parser = argparse.ArgumentParser(description="Simple cron-like scheduler")
    parser.add_argument('--shell', default='/bin/sh -c', help='shell binary to execute commands (default: /bin/sh -c)')
    parser.add_argument('--mail-from', help='sender')
    parser.add_argument('--mail-to', help='mail to')
    parser.add_argument('--sendmail', default='/usr/sbin/sendmail', help='a program to send email (default: /usr/sbin/sendmai)')
    parser.add_argument('--verbose', dest='verbose', action='store_true')
    parser.add_argument('crontab', help='crontab file', nargs=1)

    return parser.parse_args()


def MakeDefaultMailFrom():
    return 'Pycron <%s@%s>' % (getpass.getuser(), platform.node())


def Verbose(text):
    print >>sys.stderr, datetime.now().strftime('%Y-%m-%d %H:%M:%S'), text


class Job:
    def __init__(self, crontab_line):
        schedule_descr, cmd = self._split_line(crontab_line)
        self.cron_entry = CronTab(schedule_descr)
        self.cmd = cmd
        self.in_process = []

    @staticmethod
    def _split_line(line):
        fs = line.split()

        return ' '.join(fs[0:5]), ' '.join(fs[5:])


def ParseJobs(filename):
    result = []

    for line in open(filename):
        line = line.strip()
        if line.startswith('#'):
            continue

        result.append(Job(line))

    return result


ProcData = namedtuple('ProcData', "args, pobj")


class Scheduler:
    def __init__(self, opts):
        self.scheduler = sched.scheduler(time.time, time.sleep)
        self.opts = opts
        self.mail_from = self.opts.mail_from if self.opts.mail_from else MakeDefaultMailFrom()
        self.hostname = platform.node()

    def _execute_task(self, job):
        self._add_task(job)
        try:
            args = shlex.split(self.opts.shell)
            args.extend([job.cmd])
            if self.opts.verbose:
                Verbose("Starting: %s" % args)
            job.in_process.append(ProcData(
                args,
                subprocess.Popen(args, stderr=subprocess.PIPE, close_fds=True)
                ))
            time.sleep(0.1)

        except:
            Verbose("An exception has occured when executing [%s]" % " ".join(args))
            Verbose(traceback.format_exc())

        in_process = []

        for data in job.in_process:
            if data.pobj.poll() is None:
                in_process.append(data)
                continue

            data.pobj.wait()
            stderr = data.pobj.stderr.read()

            if self.opts.verbose or stderr:
                Verbose('stderr: %s' % stderr)

                if stderr and self.opts.mail_to:
                    mail_it.Send(
                        mail_it.MakeMessage(
                            self.mail_from,
                            self.opts.mail_to,
                            subject="Pycron %s - %s" % (self.hostname, ' '.join(data.args)),
                            text=stderr),
                        snd_program=self.opts.sendmail
                        )

        job.in_process = in_process

    def _add_task(self, job):
        self.scheduler.enter(
            job.cron_entry.next(now=datetime.now(), default_utc=False),
            0,
            self._execute_task,
            (job,))

    def run(self, jobs):
        for job in jobs:
            self._add_task(job)

        self.scheduler.run()


def main():
    opts = ParseArgs()

    jobs = ParseJobs(opts.crontab[0])

    scheduler = Scheduler(opts)
    scheduler.run(jobs)


if __name__ == '__main__':
    main()
