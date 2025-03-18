import smtplib
import os
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email.mime.multipart import MIMEMultipart
from email import encoders
import zipfile
import zlib


def zip_file(f):
    zipped = f + ".zip"
    zipFile = zipfile.ZipFile(zipped, 'w', compression=zlib.DEFLATED)
    zipFile.write(f)
    zipFile.close()


def send_mail(sender, to, subj, body, files):
    server = smtplib.SMTP('localhost')

    for f in files:
        zip_file(f)

    #files = [f + '.zip' for f in files]
    receivers = [s if s.find('@') != -1  else s + '@yandex-team.ru' for s in to]

    msg = MIMEMultipart()
    msgtext = MIMEText(body, 'plain', "utf-8")
    msg.attach(msgtext)
    msg['Subject'] = subj
    msg['From'] = sender
    msg['To'] = ', '.join(receivers)

    for zf in files:
        fileName, fileExtension = os.path.splitext(zf)
        attachment = MIMEBase('application', fileExtension[1:])
        attachment.set_payload(open(zf, "rb").read())
        encoders.encode_base64(attachment)
        attachment.add_header('Content-Disposition', 'attachment', filename=zf)
        msg.attach(attachment)

    #print msg.as_string()
    server.sendmail('', receivers, msg.as_string())
