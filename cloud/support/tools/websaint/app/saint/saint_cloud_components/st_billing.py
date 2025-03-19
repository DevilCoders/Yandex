from app.saint.helpers import *

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
        person = res.get('person') or {}
        contract = res.get('contract') or {}

        r_result =  {
            'billing_id': res.get('id'),
            'display_status': res.get('displayStatus'),
            'usage_status': res.get('usageStatus'),
            'ba_name': res.get('name'),
            'type': res.get('personType'),
            'payment_method': res.get('paymentType'),
            'Balance': res.get('balance'),
            'balance_person_id': res.get('personId'),
            'balance_contract_id': res.get('balanceContractId'),
            'contract_is_active': contract.get('isActive') or None,
            'person_type': person.get('type'),
            'credit_limit': res.get('billingThreshold'),
            'country_code': res.get('countryCode'),
            'country': res.get('country'),
            'created_at': timestamp_resolve(res.get('createdAt')),
            'updated_at': timestamp_resolve(res.get('updatedAt')),
            'support_plan': self._get_support_level(billing_id=billing_id),
            'autopay_failures': res.get('metadata').get('autopayFailures'),
            'fraud_detected_by': res.get('metadata').get('fraudDetectedBy'),
            'floating_threshold': res.get('metadata').get('floatingThreshold'),
            'registration_user_iam': res.get('metadata').get('registrationUserIam'),
            'unblock_reason': res.get('metadata').get('unblockReason'),
            'verified': res.get('metadata').get('verified'),
            'contract_id': contract.get('externalId'),
            'contract_balance_id': contract.get('balanceClientId'),
            'contract_currency': contract.get('currency'),
            'contract_effective_date': contract.get('effectiveDate'),
            'contract_payment_type': contract.get('paymentType'),
            'personal_data': self._get_company_or_person(res),
            'login': res.get('passport').get('Login'),
            'antifraud': 'verified' if res.get('metadata').get('verified') else 'not verified',
            'last_block_reason': res.get('metadata').get('blockReason'),
            'last_block_comment': res.get('metadata').get('blockComment'),
            'personal_manager': self._get_personal_manager(res) or '-',
            'passport': self._get_ya_pass(res),
            'Active grants': grants,
            'var': res.get('isVar'),
            'partner_id': res.get('masterAccountId') if 'isSubaccount' in res else None,
        }
        return r_result

    def _get_ya_pass(self, res):
        yapass = res.get('passport') or None
        if yapass:
            return {
                'login': yapass.get('Login'),
                'name': yapass.get('Name'),
                'uid': yapass.get('Uid')
            }
        else:
            return {}
    def _get_personal_manager(self, res):
        salesmanager = res.get('salesManager') or None
        if salesmanager:
            return {
                'first_name': salesmanager.get('firstName'),
                'last_name': salesmanager.get('lastName'),
                'login': salesmanager.get('login'),
                'email' : get_nda_link(f"https://quotacalc.cloud-testing.yandex.net/saint/eml/{salesmanager.get('workEmail')}")
            }
        else:
            return {}

    def _get_company_or_person(self, res):
        person = res.get('person')

        try:
            if person.get('individual'):
                ba_user_name = (
                    person.get('individual').get('lastName'),
                    person.get('individual').get('firstName'),
                    person.get('individual').get('middleName')
                )
                full_name = ' '.join(ba_user_name)
                person_data, company_data = {
                    'full_name': full_name,
                    'email': get_nda_link(f"https://quotacalc.cloud-testing.yandex.net/saint/eml/{person.get('individual').get('email')}")
                }, {}

            else:   # company

                company = person.get('company')
                company_data, person_data = {
                    'long_name': company.get('longname'),
                    'email': get_nda_link(f"https://quotacalc.cloud-testing.yandex.net/saint/eml/{company.get('email')}"),
                    'post_code': company.get('postcode'),
                    'inn': company.get('inn'),
                    'bik': company.get('bik'),
                    'account': company.get('account'),
                    'kpp': company.get('kpp'),
                    'doc_checked': person.get('verifiedDocs')
                }, {}

        except AttributeError:
            person_data = None
            company_data = None

        return {'person': person_data, 'company': company_data}


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
                grant_reason_id = grant.get('sourceId')
                grant_reason = grant.get('source')
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
                                    'Reason Id': grant_reason_id,
                                    'Reason': grant_reason
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
                status = '{c1}{value}{c2}'.format(c1='', value=status, c2='')
            type = res.get('type', None)
            if type == 'payments':
                type = '{c1}{value}{c2}'.format(c1='', value=type, c2='')
            if type == 'costs':
                type = '{c1}{value}{c2}'.format(c1='', value=type, c2='')

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

    def _get_support_level(self, billing_id):
        url = self.endpoints.billing_url + 'billingAccounts/{b_id}/supportSubscriptions'.format(b_id=billing_id)
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        r = requests.get(url=url, headers=headers)
        res = r.json()
        logging.debug(res)

        if r.status_code != 200:
            logging.error(res)
            return None

        return res.get('current').get('name')
