"""
class Auth:
authirization mixin
    def _get_iam_token -> IamToken
    def _get_iam_token_and_save_it_to_config(self) -> IamToken
    def _get_cached_token(self) -> str
    def clear_iam_token_cache(self)
"""

from helpers import *
from grpc_gw import grpc, iam_token_service, iam_token_service_pb2, get_grpc_channel, cloud_service,cloud_service_pb2
from assets import *
import jwt
import time

home_dir = expanduser('~')
config = configparser.RawConfigParser()
config.read('{}/.rei/rei.cfg'.format(home_dir))

class St_Auth:

    def _get_iam_token(self) -> IamToken:

        endpoint = self.endpoints.iam_token_url[self.profile.name.upper()]
        account_id = self.profile.profile_static_keys['acc_id']
        key_id = self.profile.profile_static_keys['id']
        private_key = self.profile.profile_static_keys['private_key']

        aud = 'https://iam.api.cloud.yandex.net/iam/v1/tokens'
        now = int(time.time())

        payload = {
            'aud': aud,
            'iss': account_id,
            'iat': now,
            'exp': now + 360}

        encoded_token = jwt.encode(
            payload,
            private_key,
            algorithm='PS256',
            headers={'kid': key_id})

        ssl = os.environ['REQUESTS_CA_BUNDLE']
        with open(ssl, 'rb') as cert:
            ssl_creds = grpc.ssl_channel_credentials(cert.read())
        call_creds = grpc.access_token_call_credentials('randomshit')
        chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

        channel = grpc.secure_channel(endpoint, chan_creds)
        iam_stub = iam_token_service.IamTokenServiceStub(channel)

        requesst = iam_token_service_pb2.CreateIamTokenRequest(
            jwt=encoded_token
        )
        resp = iam_stub.Create(requesst)

        return IamToken(resp.iam_token, timestamp_resolve(resp.expires_at.seconds))

    def _get_iam_token_and_save_it_to_config(self) -> IamToken:
        token = self._get_iam_token()
        section_name = self.profile.name.upper() + '.IAM'
        with open('{home}/.rei/rei.cfg'.format(home=home_dir), 'w') as cfgfile:
            try:
                config.add_section(section_name)
            except configparser.DuplicateSectionError as e:
                logging.debug(e)
            config.set(section_name, 'token', token.token)
            config.set(section_name, 'expires', token.expires_at)
            config.write(cfgfile)

        return token

    def _get_cached_token(self) -> str:
        if not os.path.exists('{home}/.rei'.format(home=home_dir)):
            os.system('mkdir -p {home}/.rei'.format(home=home_dir))

        section_name = self.profile.name.upper() + '.IAM'
        try:
            token = config.get(section_name, 'token')
            config.get(section_name, 'expires')
        except Exception:
            self._get_iam_token_and_save_it_to_config()

        try:
            channel = get_grpc_channel(cloud_service.CloudServiceStub,
                                       self.endpoints.cloud_url[self.profile.name.upper()],
                                       token)

            req = cloud_service_pb2.GetCloudRequest(
                cloud_id='b1gig0ogqtnk75jde2q8'
            )
            cloud = channel.Get(req)    # не удалять! Важный костыль!

        except Exception as e:
            logging.debug(e)
            self._get_iam_token_and_save_it_to_config()

        logging.debug('IAM token is expired. Update...')
        iam_token = self._get_iam_token_and_save_it_to_config()
        return iam_token.token

    def clear_iam_token_cache(self):
        section_name = self.profile.name.upper() + '.IAM'
        with open('{home}/.rei/rei.cfg'.format(home=home_dir), 'w') as cfgfile:
            try:
                config.remove_section(section_name)
            except configparser.DuplicateSectionError as e:
                logging.debug(e)
            except KeyError:
                print('Cache already clean')
            else:
                print('Cache cleared')
            config.write(cfgfile)

    @property
    def iam_token(self):
        return self._get_cached_token()
