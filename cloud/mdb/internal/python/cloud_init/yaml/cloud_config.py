import yaml
from ..config import CloudConfig, WriteFiles, RunCMD, Hostname, UpdateEtcHosts, Users, LegacyKeyValue
from .modules import (
    encode_write_files,
    encode_runcmd,
    encode_hostname,
    encode_update_etc_hosts,
    encode_users,
    encode_legacy_key_value,
)


def encode_cloud_config(dumper: yaml.Dumper, subj: CloudConfig):
    result = {}
    for legacy in subj.legacy_values:
        result[legacy.key] = legacy.value
    for key, value in subj.list_modules.items():
        result[key] = value
    for value in subj.scalar_modules:
        result[value.module_name] = value
    return dumper.represent_dict(result)


class Dumper(yaml.SafeDumper):
    pass


Dumper.add_representer(CloudConfig, encode_cloud_config)
Dumper.add_representer(WriteFiles, lambda dumper, subj: dumper.represent_dict(encode_write_files(subj)))
Dumper.add_representer(Users, lambda dumper, subj: dumper.represent_dict(encode_users(subj)))
Dumper.add_representer(RunCMD, lambda dumper, subj: dumper.represent_str(encode_runcmd(subj)))
Dumper.add_representer(Hostname, lambda dumper, subj: dumper.represent_str(encode_hostname(subj)))
Dumper.add_representer(UpdateEtcHosts, lambda dumper, subj: dumper.represent_bool(encode_update_etc_hosts(subj)))
Dumper.add_representer(LegacyKeyValue, lambda dumper, subj: dumper.represent_dict(encode_legacy_key_value(subj)))
