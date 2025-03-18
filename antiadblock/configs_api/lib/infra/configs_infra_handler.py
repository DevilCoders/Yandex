import time

from antiadblock.libs.infra.lib.infra_client import InfraException
from antiadblock.configs_api.lib.infra.infra_cache_manager import InfraCacheManager


class ConfigsInfraHandler(object):
    PROD_DATACENTERS = {  # not used
        "man": True,
        "myt": True,
        "iva": True,
        "sas": True,
        "vla": True
    }
    TEST_DATACENTERS = {  # not used
        "man": True,
        "sas": True,
        "vla": True
    }

    SERVICE_WHITELIST = (  # service_ids
        "kinopoisk.ru",
        "zen.yandex.ru",
        "yandex_morda",
        "yandex_news"
    )

    def __init__(self, infra_client, environment_type, host_domain, namespace_id):
        """
        :param infra_client: InfraClient instance
        """
        self.infra_manager = InfraCacheManager(infra_client, namespace_id)
        self.infra_client = infra_client
        self.environment_type = environment_type
        self.host_domain = host_domain

    def send_event_config_updated(self,
                                  service_id, label_id,
                                  old_config_id, new_config_id,
                                  config_comment,
                                  is_test_config,
                                  device_type=None,
                                  retrieval_timeout=None):
        if self.environment_type.upper() not in ("PRODUCTION", "TESTING"):
            return
        # TODO: Right now Infra isn't ready to maintain environments for every partner.
        #       Remove this when it'll be fixed and switch to monitoring_enabled flag checking
        if service_id not in self.SERVICE_WHITELIST:
            return

        env_label = "testing" if is_test_config else "production"
        service_name = "Antiadblock partner {}".format(service_id)

        def send_event():
            # also creates a service if needed
            environment = self.infra_manager.get_or_create_infra_environment(service_name=service_name,
                                                                             environment_name=env_label,
                                                                             retrieval_timeout=retrieval_timeout)

            self.infra_client.create_event(
                service_id=environment['service_id'],
                environment_id=environment['id'],
                title="Antiadblock partner {SERVICE} config update".format(SERVICE=service_id),
                desc="service: {SERVICE}\n"
                     "label: {LABEL}\n"
                     "{DEVICE_LINE}"
                     "old_config_id: {OLD_CONFIG_ID}\n"
                     "new_config_id: {NEW_CONFIG_ID}\n"
                     "diff: https://{HOST_DOMAIN}/service/{SERVICE}/configs/diff/{OLD_CONFIG_ID}/{NEW_CONFIG_ID}\n"
                     "comment: {COMMENT}\n"
                     .format(
                        HOST_DOMAIN=self.host_domain,
                        SERVICE=service_id,
                        ENV_LABEL=env_label,
                        LABEL=label_id,
                        COMMENT=config_comment.encode('utf-8'),
                        OLD_CONFIG_ID=old_config_id,
                        NEW_CONFIG_ID=new_config_id,
                        DEVICE_LINE=self.__make_device_line(device_type)
                     ),
                start_time=time.time(),
                finish_time=time.time() + 3 * 60,
                # datacenters=self.TEST_DATACENTERS if is_test_config else self.PROD_DATACENTERS,
            )

        try:
            send_event()
        except InfraException as e:
            if e.response.status_code in (403, 404):  # maybe something was deleted by hand
                self.infra_manager.invalidate_cache()
                send_event()
            else:
                raise e

    @staticmethod
    def __make_device_line(device_type):
        device_line = ""
        if device_type is not None:
            if device_type == 0:
                device_tag = "desktop"
            elif device_type == 1:
                device_tag = "mobile"
            else:
                device_tag = str(device_type)
            device_line = "device: {}\n".format(device_tag)
        return device_line
