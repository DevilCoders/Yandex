import subprocess
from email.mime.text import MIMEText


SENDMAIL = "/usr/sbin/sendmail" # sendmail location


def MakeMessage(from_, to, subject='', text=''):
    msg = MIMEText(text)

    msg['From'] = from_
    msg['To'] = to
    msg['Subject'] = subject

    return msg.as_string()


def Send(full_mime_msg, snd_program=SENDMAIL):
    p = subprocess.Popen([snd_program, '-t'], stdin=subprocess.PIPE, close_fds=True)
    if not p:
        print >>sys.stderr, "Could not execute %s" % snd_program

    stdout, stderr = p.communicate(full_mime_msg)

    if stderr:
        print >>sys.stderr, "%s has failed: %s", stderr
