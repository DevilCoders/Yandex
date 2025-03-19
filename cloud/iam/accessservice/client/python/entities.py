import datetime

from yandex.cloud.priv.servicecontrol.v1 import access_service_pb2, resource_pb2


class Subject(object):
    def __init__(self):
        return NotImplemented

    def _to_grpc_message(self):
        return NotImplemented

    @classmethod
    def _from_grpc_message(cls, grpc_subject):
        grpc_subject_type = grpc_subject.WhichOneof("type")
        if grpc_subject_type == "anonymous_account":
            return AnonymousAccountSubject._from_grpc_message(grpc_subject.anonymous_account)
        if grpc_subject_type == "user_account":
            return UserAccountSubject._from_grpc_message(grpc_subject.user_account)
        if grpc_subject_type == "service_account":
            return ServiceAccountSubject._from_grpc_message(grpc_subject.service_account)
        raise ValueError("Unknown GRPC Subject type: {}.".format(grpc_subject_type))


class AnonymousAccountSubject(Subject):
    def __init__(self):
        pass

    def __repr__(self):
        return "<AnonymousAccount>"

    def __eq__(self, other):
        return isinstance(other, AnonymousAccountSubject)

    def _to_grpc_message(self):
        anonymous_account = access_service_pb2.Subject.AnonymousAccount()
        subject = access_service_pb2.Subject(
            anonymous_account=anonymous_account,
        )
        return subject

    @classmethod
    def _from_grpc_message(cls, grpc_subject):
        return cls()


class UserAccountSubject(Subject):
    def __init__(self, id):
        self.id = id

    def __repr__(self):
        return "<UserAccount: id {}>".format(self.id)

    def __eq__(self, other):
        return isinstance(other, UserAccountSubject) and other.id == self.id

    def _to_grpc_message(self):
        user_account = access_service_pb2.Subject.UserAccount(
            id=self.id,
        )
        subject = access_service_pb2.Subject(
            user_account=user_account,
        )
        return subject

    @classmethod
    def _from_grpc_message(cls, grpc_subject):
        return cls(
            id=grpc_subject.id,
        )


class ServiceAccountSubject(Subject):
    def __init__(self, id, folder_id=None):
        self.id = id
        self.folder_id = folder_id

    def __repr__(self):
        return "<ServiceAccount: id {}, folder_id {}>".format(
            self.id,
            self.folder_id if self.folder_id is not None else "unknown",
        )

    def __eq__(self, other):
        return (
            isinstance(other, UserAccountSubject)
            and other.id == self.id
            and other.folder_id == self.folder_id
        )

    def _to_grpc_message(self):
        service_account = access_service_pb2.Subject.ServiceAccount(
            id=self.id,
            folder_id=self.folder_id,
        )
        subject = access_service_pb2.Subject(
            service_account=service_account,
        )
        return subject

    @classmethod
    def _from_grpc_message(cls, grpc_subject):
        return cls(
            id=grpc_subject.id,
            folder_id=grpc_subject.folder_id,
        )


class AccessKeySignature(object):
    def __init__(self, *args, **kwargs):
        return NotImplemented


class AccessKeySignatureV2(object):
    class SignatureMethod(object):
        UNSPECIFIED = 0
        HMAC_SHA1 = 1
        HMAC_SHA256 = 2

        @classmethod
        def name(cls, signature_method):
            if signature_method == cls.UNSPECIFIED:
                return "UNSPECIFIED"
            elif signature_method == cls.HMAC_SHA1:
                return "HMAC_SHA1"
            elif signature_method == cls.HMAC_SHA256:
                return "HMAC_SHA256"
            else:
                return "unknown"

    def __init__(
        self,
        access_key_id, string_to_sign, signature,
        signature_method,
    ):
        self.access_key_id = access_key_id
        self.string_to_sign = string_to_sign
        self.signature = signature

        self.signature_method = signature_method

    def __repr__(self):
        return (
            "<AccessKeySignature V2: access_key_id {}, string_to_sign {}, "
            "signature {}, signature_method {}>".format(
                self.access_key_id,
                _ellipsis(self.string_to_sign),
                _ellipsis(self.signature),
                self.SignatureMethod.name(self.signature_method),
            )
        )

    def _to_grpc_message(self):
        v2_parameters = access_service_pb2.AccessKeySignature.Version2Parameters(
            signature_method=self.signature_method,
        )
        message = access_service_pb2.AccessKeySignature(
            access_key_id=self.access_key_id,
            string_to_sign=self.string_to_sign,
            signature=self.signature,
            v2_parameters=v2_parameters,
        )
        return message


class AccessKeySignatureV4(object):
    def __init__(
        self,
        access_key_id, string_to_sign, signature,
        signed_at, service, region,
    ):
        self.access_key_id = access_key_id
        self.string_to_sign = string_to_sign
        self.signature = signature

        if isinstance(signed_at, datetime.datetime):
            self.signed_at = signed_at
        else:
            self.signed_at = datetime.datetime.utcfromtimestamp(signed_at)
        self.service = service
        self.region = region

    def __repr__(self):
        return (
            "<AccessKeySignature V4: access_key_id {}, string_to_sign {}, "
            "signature {}, signed_at {}, service {}, region {}>".format(
                self.access_key_id,
                _ellipsis(self.string_to_sign),
                _ellipsis(self.signature),
                self.signed_at,
                self.service,
                self.region,
            )
        )

    def _to_grpc_message(self):
        v4_parameters = access_service_pb2.AccessKeySignature.Version4Parameters(
            service=self.service,
            region=self.region,
        )
        v4_parameters.signed_at.FromDatetime(self.signed_at)
        message = access_service_pb2.AccessKeySignature(
            access_key_id=self.access_key_id,
            string_to_sign=self.string_to_sign,
            signature=self.signature,
            v4_parameters=v4_parameters,
        )
        return message


class Resource(object):
    TYPE_CLOUD = "resource-manager.cloud"
    TYPE_FOLDER = "resource-manager.folder"
    TYPE_SERVICE_ACCOUNT = "iam.service-account"
    TYPE_BILLING_ACCOUNT = "billing.account"

    def __init__(self, id, type):
        self.id = id
        self.type = type

    def __repr__(self):
        return "<Resource: id {}, type {}>".format(self.id, self.type)

    def __eq__(self, other):
        return (
            isinstance(other, Resource)
            and other.id == self.id
            and other.type == self.type
        )

    def _to_grpc_message(self):
        return resource_pb2.Resource(
            id=self.id,
            type=self.type,
        )


class Cloud(Resource):
    def __init__(self, id):
        super(Cloud, self).__init__(id=id, type=Resource.TYPE_CLOUD)


class Folder(Resource):
    def __init__(self, id):
        super(Folder, self).__init__(id=id, type=Resource.TYPE_FOLDER)


class ServiceAccount(Resource):
    def __init__(self, id):
        super(ServiceAccount, self).__init__(id=id, type=Resource.TYPE_SERVICE_ACCOUNT)


class BillingAccount(Resource):
    def __init__(self, id):
        super(BillingAccount, self).__init__(id=id, type=Resource.TYPE_BILLING_ACCOUNT)


def _ellipsis(string, ends_length=4):
    """Helper function for __repr__."""
    if len(string) <= ends_length * 2 + 3:
        return string
    else:
        return "{}...{}".format(string[:ends_length], string[-ends_length:])
