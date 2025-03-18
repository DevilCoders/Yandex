from ci.tasklet.registry.demo.picklock.proto import schema_tasklet
from tasklet.services.yav.proto import yav_pb2


class PicklockImpl(schema_tasklet.PicklockBase):
    def run(self):
        secret_spec = yav_pb2.YavSecretSpec()
        secret_spec.uuid = self.input.context.secret_uid

        for key in self.input.yav_filter.keys:
            secret_spec.key = key
            yav_response = self.ctx.yav.get_secret(secret_spec)
            value_container = self.output.values.add()
            value_container.key = key
            value_container.value = yav_response.secret
