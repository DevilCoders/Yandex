# -*- coding: utf-8 -*-

import smtplib

from django.conf import settings
from django.contrib.sites.models import Site
from django.template import Context, RequestContext, loader
from django.utils import translation
from django.utils.translation import ugettext as _
from email.header import Header
from email.mime.text import MIMEText

from core.hard.loghandlers import SteamLogger
from ui.settings import SERVER_EMAIL, ALLOW_EMAILS, SERVER_SMTP


def send_all_emails(request, aadmin_est, recipients, snip_tpls_ext):
    template = loader.get_template('core/email_template.html')
    mediaContent = aadmin_est.task.taskpool.kind_pool == aadmin_est.task.taskpool.TypePool.RCA
    email_html = template.render(RequestContext(request, {
        'message': request.POST['message'],
        'show_pools': False, 'task': aadmin_est.task,
        'snip_tpls_ext': snip_tpls_ext, 'host': request.get_host(),
        'no_icons': True, 'showMediaContent': mediaContent,
    }))
    from_addr = email_address(request.yauser.core_user.login)
    to_addrs = [email_address(recip) for recip in recipients]
    cc_addrs = [from_addr]
    return send_email(email_html, request.POST['subject'],
                      from_addr, to_addrs, cc_addrs)


def email_address(login):
    return '@'.join((login, 'yandex-team.ru'))


def notify_about_user(request):
    template = loader.get_template('core/user_notification_template.html')
    email_html = template.render(RequestContext(request, {
        'login': request.yauser.login,
        'host': request.get_host()
    }))
    to_addrs = [SERVER_EMAIL]
    return send_email(email_html, 'STEAM: new user',
                      email_address(request.yauser.login), to_addrs)


def email_to_new_user(request):
    template = loader.get_template('core/new_user_email_template.html')
    language = request.POST['language'].lower()
    if language not in dict(settings.LANGUAGES):
        language = 'en'
    translation.activate(language)
    yourequested = _('You requested permissions at')
    pleasewait = _('Please wait. You will be notified on getting access.')
    header = _('STEAM: permissions requested')
    translation.deactivate()
    email_html = template.render(RequestContext(request, {
        'yourequested': yourequested,
        'pleasewait': pleasewait,
        'host': request.get_host()
    }))
    to_addrs = [email_address(request.yauser.login)]
    return send_email(email_html, header,
                      SERVER_EMAIL, to_addrs)


def send_letter_on_grant(request, user):
    template = loader.get_template('core/grant_email_template.html')
    language = user.language.lower()
    if language not in dict(settings.LANGUAGES):
        language = 'en'
    translation.activate(language)
    user_literal = _('User')
    message = _('granted you permissions to access')
    header = _('STEAM: permissions granted')
    translation.deactivate()
    email_html = template.render(RequestContext(request, {
        'user_literal': user_literal,
        'granter': request.yauser.login,
        'message': message,
        'host': request.get_host()
    }))
    to_addrs = [email_address(user.login)]
    cc_addrs = [SERVER_EMAIL]
    return send_email(email_html, header,
                      SERVER_EMAIL, to_addrs, cc_addrs)


def notify_about_deadline(as_logins, language, taskpool_id):
    template = loader.get_template('core/deadline_email_template.html')
    language = language.lower()
    if language not in dict(settings.LANGUAGES):
        language = 'en'
    translation.activate(language)
    deadline_literal = _('The deadline for')
    a_text = _('this task pack')
    istoday_literal = _('is today')
    header = _('STEAM: deadline notification')
    translation.deactivate()
    host = 'steam.yandex-team.ru'
    sites = Site.objects.all()[:1]
    if sites:
        host = sites[0].domain
    email_html = template.render(Context({
        'deadline_literal': deadline_literal,
        'a_text': a_text,
        'istoday_literal': istoday_literal,
        'taskpool_id': taskpool_id,
        'host': host
    }))
    to_addrs = []
    bcc_addrs = [email_address(as_login) for as_login in as_logins]
    SteamLogger.info(
        'Notified %(count)d users about deadline for taskpool %(taskpool_id)d',
        count=len(as_logins), taskpool_id=taskpool_id, type='MAIL_SEND_EVENT'
    )
    return send_email(email_html, header,
                      SERVER_EMAIL, to_addrs, bcc_addr_list=bcc_addrs)


def send_stats_to_aadmins(aa_logins, info, language):
    template = loader.get_template('core/stats_email_template.html')
    language = language.lower()
    if language not in dict(settings.LANGUAGES):
        language = 'en'
    translation.activate(language)
    header_literal = _(
        'The assessors in your country have completed the following tasks amount for previous month'
    )
    login_literal = _('Login')
    ests_literal = _('Total')
    linked_literal = _('Linked')
    noests_literal = _('No estimations are available.')
    header = _('STEAM: monthly statistics')
    translation.deactivate()
    email_html = template.render(Context({
        'header_literal': header_literal,
        'login_literal': login_literal,
        'ests_literal': ests_literal,
        'linked_literal': linked_literal,
        'noests_literal': noests_literal,
        'infos': info.items(),
    }))
    to_addrs = [email_address(aa_login)
                for aa_login in aa_logins + ['steam-dev']]
    SteamLogger.info(
        'Senfing monthly statistics to %(count)d users',
        count=len(aa_logins), type='MAIL_SEND_EVENT'
    )
    return send_email(email_html, header,
                      SERVER_EMAIL, to_addrs)


def send_email(body, subject, from_addr, to_addr_list,
               cc_addr_list=None, bcc_addr_list=None):
    if not ALLOW_EMAILS:
        return True
    cc_addr_list = cc_addr_list or []
    bcc_addr_list = bcc_addr_list or []
    recipients = to_addr_list + cc_addr_list + bcc_addr_list
    if not recipients:
        return True
    sender = smtplib.SMTP(SERVER_SMTP)
    email = MIMEText(body, 'html', 'utf-8')
    email['From'] = from_addr
    email['Subject'] = Header(subject, 'utf-8')
    email['To'] = ', '.join(to_addr_list)
    if cc_addr_list:
        email['Cc'] = ', '.join(cc_addr_list)
    if bcc_addr_list:
        email['Bcc'] = ', '.join(bcc_addr_list)
    res = True
    try:
        sender.sendmail(from_addr, recipients, email.as_string())
    except (smtplib.SMTPDataError, smtplib.SMTPServerDisconnected,
            smtplib.SMTPRecipientsRefused) as err:
        SteamLogger.error('Mail sending error: %(err)s',
                          err=err, type='MAIL_SEND_ERROR')
        res = False
    try:
        sender.quit()
    except:
        pass
    return res
