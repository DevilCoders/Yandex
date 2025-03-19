from typing import NamedTuple

from dbaas_common import tracing

from ...common import BaseProvider
from ...pillar import DbaasPillar


PILLAR_PATH = ['data', 'byoa']


class Parameters(NamedTuple):
    iam_role_arn: str
    account_id: str


class BYOA(BaseProvider):
    _parameters: Parameters = None  # type: ignore

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)

    def iam_role(self) -> str:
        params = self._parameters or self._update_params()

        return params.iam_role_arn

    def account(self) -> str:
        params = self._parameters or self._update_params()

        return params.account_id

    def is_byoa(self):
        return self.iam_role() != self.config.aws.dataplane_role_arn

    @tracing.trace("Get BYOA params from pillar")
    def _update_params(self) -> Parameters:
        pillar = self.pillar.get('cid', self.task['cid'], PILLAR_PATH)
        self._parameters = Parameters(
            iam_role_arn=pillar.get('iam_role_arn', self.config.aws.dataplane_role_arn),
            account_id=pillar.get('account_id', self.config.aws.dataplane_account_id),
        )
        tracing.set_tag("iam_role_arn", self._parameters.iam_role_arn)
        tracing.set_tag("account_id", self._parameters.account_id)
        return self._parameters

    @tracing.trace("Set BYOA params")
    def set_params(self, iam_role: str, account: str) -> None:
        tracing.set_tag("iam_role_arn", iam_role)
        tracing.set_tag("account_id", account)
        self.pillar.exists(
            'cid',
            self.task['cid'],
            PILLAR_PATH,
            ['account_id', 'iam_role_arn'],
            [account, iam_role],
        )
