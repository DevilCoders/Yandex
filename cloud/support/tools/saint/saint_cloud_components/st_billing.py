from helpers import *
import certifi

class St_Billing:


    # Resolve cloud_id to billing_id
    def resolve_billing(self, data, id_type=None):

        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        support_types = ('billing_account_id', 'cloud_id', 'passport_login')

        payload = {
            id_type: data
        }

        if id_type is None or id_type not in support_types:
            logging.error('Unknown type received')
            return

        r = requests.post(self.endpoints.billing_url + 'support/resolve', headers=headers, json=payload)
        res = r.json()
        logging.debug(res)

        if r.status_code != 200:
            logging.error(res)
            return None

        return res.get('id')

    def get_billing_full(self, billing_id):
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        r = requests.get(
            self.endpoints.billing_url + 'billingAccounts/{billing_id}/fullView'.format(billing_id=billing_id),
            headers=headers)
        res = r.json()
        logging.debug(res)

        if r.status_code != 200:
            logging.error(res)
            return None

        grants = self.get_cloud_grants_sum(billing_id)
        person = res.get('person')

        try:
            if person.get('individual'):
                ba_user_name = (
                    person.get('individual').get('lastName'),
                    person.get('individual').get('firstName'),
                    person.get('individual').get('middleName')
                )
                full_name = ' '.join(ba_user_name)
                company = None

            else:
                full_name = person.get('company').get('longname')
                company = person.get('company').get('name')

        except AttributeError:
            full_name = None
            company = None

        return {
            'billing_id': res.get('id'),
            'status': res.get('displayStatus'),
            'usage_status': res.get('usageStatus'),
            'ba_name': res.get('name'),
            'login': res.get('passport').get('Login'),
            'antifraud': 'verified' if res.get('metadata').get('verified') else 'not verified',
            'last_block_reason': res.get('metadata').get('blockReason'),
            'last_block_comment': res.get('metadata').get('blockComment'),
            'Balance': res.get('balance'),
            'Credit Limit': res.get('billingThreshold'),
            'Active grants': grants,
            'type': res.get('personType'),
            'full_name': full_name,
            'company_name': company,
            'var': res.get('isVar'),
            'partner_id': res.get('masterAccountId') if 'isSubaccount' in res else None,
        }

    # Return the grant amount
    def get_grants(self, billing_id, dead_grants=False):
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        r = requests.get(
            self.endpoints.billing_url + 'billingAccounts/{billing_id}/monetaryGrants'.format(billing_id=billing_id),
            headers=headers)
        res = r.json()
        logging.debug(res)

        if r.status_code != 200:
            logging.error(res)
            return None
        else:
            today = datetime.utcnow()
            grants = res.get('monetaryGrants')
            active_grants = []
            for grant in grants:
                grant_expired = timestamp_resolve(grant.get('endTime'))
                end_date = datetime.strptime(grant_expired, '%Y-%m-%dT%H:%M:%S')
                grant_id = grant.get('id')
                grant_reason = grant.get('sourceId')
                start_date = datetime.strptime(timestamp_resolve(grant.get('createdAt')), '%Y-%m-%dT%H:%M:%S')
                if (end_date > today and not dead_grants) or (end_date < today and dead_grants):
                    try:
                        consumed = grant.get('consumedAmount').split('.')[0]
                        initial = grant.get('initialAmount').split('.')[0]
                        balance = int(initial) - int(consumed)
                        if balance != 0:
                            active_grants.append(
                                {
                                    'Id': grant_id,
                                    'Start Date': start_date,
                                    'End Date': end_date,
                                    'Initial Amount': initial,
                                    'Consumed Amount': consumed,
                                    'Reason Id': grant_reason
                                }
                            )

                    except Exception as e:
                        logging.debug(e)

            return active_grants

    def check_promocode(self, promocode_id):
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        r = requests.get(self.endpoints.billing_url + '/monetaryGrantOffers/{promo}'.format(
            promo=promocode_id), headers=headers)
        res = r.json()
        logging.debug(res)

        if r.status_code != 200:
            logging.error(res)
            return None

        return {
            'promocode_id': promocode_id,
            'created_at': timestamp_resolve(res.get('createdAt')),
            'expiration_time': timestamp_resolve(res.get('expirationTime')),
            'duration': int(res.get('duration') / 86400),
            'amount': res.get('initialAmount'),
            'reason': res.get('proposedMeta').get('reason') if res.get('proposedMeta') else None,
            'created_by': res.get('proposedMeta').get('staffLogin') if res.get('proposedMeta') else None
        }

    # later...
    def get_payments_history(self, billing_id):
        url = self.endpoints.billing_url + 'billingAccounts/{}/transactions'.format(billing_id)
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        r = requests.get(url, headers=headers)
        payments = r.json()
        logging.debug(payments)

        result = []
        if r.status_code != 200:
            logging.error(payments)
            return None

        for res in payments['transactions']:
            status = res.get('status', None)
            if status != 'ok':
                status = '{c1}{value}{c2}'.format(c1=Color.red, value=status, c2=Color.END)
            type = res.get('type', None)
            if type == 'payments':
                type = '{c1}{value}{c2}'.format(c1=Color.yellow, value=type, c2=Color.END)
            if type == 'costs':
                type = '{c1}{value}{c2}'.format(c1=Color.green, value=type, c2=Color.END)

            result.append(
                {'id': res.get('id', None),
                 'amount': res.get('amount', None),
                 'created_at': datetime.strptime(timestamp_resolve(res.get('createdAt', None)), '%Y-%m-%dT%H:%M:%S'),
                 'modified_at': datetime.strptime(timestamp_resolve(res.get('modifiedAt', None)), '%Y-%m-%dT%H:%M:%S'),
                 'type': type,
                 'status':status,
                 'description': res.get('description', None)}
            )
        return result


# Функция, ранее используемого API для проверки маски(bin) платежной карты пользователя
    def bin_check(self, bin):
        url = f'https://lookup.binlist.net/{bin}'
        headers = {
            'Accept-Version': '3'
        }
        r = requests.get(url, headers=headers, verify=False)
        try:
            res = r.json()
        except json.decoder.JSONDecodeError:
            logging.warning('You have exceeded the request limit. Try again after 2 minutes.')
            return None

        try:
            data = {
                'bin': bin,
                'scheme': res['scheme'],
                'type': res['type'],
                'brand': res['brand'],
                'prepaid': f'{True if "prepaid" in res else False}',
                'country': res['country']['name'],
                'bank': res['bank']['name']
            }
            return data
        except KeyError:
            logging.warning('Incomplete response from API')
            return res

    def bin_check_dop(self, bin):
        url = 'http://mrbin.io/bins/bin/getFull'
        headers = {'Content-Type': 'application/json', 'Authorization': 'Basic bXJiaW5pbzp0ZXN0X21yYmluaW8='}
        data = {'fullBin': bin}
        try:
            r = requests.post(url=url, headers=headers, data=json.dumps(data), verify=certifi.where())
            if r.status_code == 200:
                r = r.json()
                data = {
                    'bin': bin,
                    'BankName': r['bankName'],
                    'System': r['paymentSystem'],
                    'Country1': r['countryAlpha2'],
                    'Country2': r['countryAlpha3'],
                    'Category': r['product']['category']
                }
        except:
            pass
        return data

    def get_clouds_by_billing_account_id(self, billing_account):
        headers = {
            'X-YaCloud-SubjectToken': self.iam_token,
            'content-type': 'application/json'
        }
        url = self.endpoints.billing_url + 'billingAccounts/{ba}/clouds'.format(ba=billing_account)
        r = requests.get(url, headers=headers)
        res = r.json()

        if r.status_code != 200:
            logging.error(res)
            return None

        return res.get('clouds')

    def get_cloud_grants_sum(self, billing_id):
        grants = self.get_grants(billing_id=billing_id)
        summ = 0

        for grant in grants:
            summ += int(grant.get('Initial Amount')) - int(grant.get('Consumed Amount'))

        return summ
