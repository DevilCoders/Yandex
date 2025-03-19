import re

def get_mail_id(mail_desc):
    if mail_desc is None:
        return
    mail_desc = mail_desc.decode("utf-8") 
    mail_desc = mail_desc.lower()
    if '|' in mail_desc:
        if re.match('^\d+\.\d+\.\d+\|.*', mail_desc):
            st = mail_desc.split('|')[1].lower()
            st = ' '.join(st.split())
            return st.replace(' ', '-')
        else:
            st = mail_desc.split('|')[0].lower()
            st = ' '.join(st.split())
            return st.replace(' ', '-')
    else:
        if 'test' in mail_desc or 'тест' in mail_desc:
            return 'testing'

        if 'terms-update' in mail_desc or 'terms_update' in mail_desc:
            return 'terms-update'

        if 'activation' in mail_desc or 'act-' in mail_desc:
            return 'ba-activation'

        if 'scenario' in mail_desc:
            return '3-scenarios-to-paid'

        if 'go-to-paid' in mail_desc and 'promo' not in mail_desc:
            return 'go-to-paid'

        if 'beginners' in mail_desc and 'promo' in mail_desc:
            return 'promo-beginners'

        if 'active-user' in mail_desc and 'promo' in mail_desc:
            return 'promo-active-users'

        if 'we-are-public' in mail_desc:
            return 'we-are-public'

        if 'start-usage' in mail_desc and 'promo' in mail_desc:
            return 'promo-start-usage'

        if 'go-to-paid' in mail_desc and 'promo' in mail_desc:
            return 'promo-go-to-paid'

        if 'reminder' in mail_desc and 'promo' in mail_desc:
            return 'promo-reminder'

        if 'trial' in mail_desc and 'extended' in mail_desc:
            return 'trial-extended'

        if 'typical' in mail_desc:
            return 'typical-task'

        if 'grant' in mail_desc and 'use' in mail_desc:
            return 'use-grant'

        if 'open' in mail_desc:
            return 'we-are-open'

        if 'follow' in mail_desc:
            return 'webinar-follow-up'

        if 'error' in mail_desc:
            return 'error-payment-method'

        if 'cloud-functionality' in mail_desc:
            return 'cloud-functionality'

        if '[' in mail_desc:
            st = mail_desc.split('[')[0].lower()
            st = ' '.join(st.split())
            if st[-1] == '-':
                return st[:-1].replace(' ', '-')
            return st.replace(' ', '-')
        return mail_desc.replace(' ', '-')