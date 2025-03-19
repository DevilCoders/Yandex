from ..config import Users, UpdateEtcHosts, RunCMD, Hostname, WriteFiles, LegacyKeyValue


def encode_write_files(subj: WriteFiles) -> object:
    return dict(
        path=subj.path,
        content=subj.content,
        permissions=subj.permissions,
    )


def encode_users(subj: Users) -> object:
    return dict(
        name=subj.name,
        sudo=subj.sudo,
        shell=subj.shell,
        ssh_authorized_keys=subj.keys,
    )


def encode_update_etc_hosts(subj: UpdateEtcHosts) -> object:
    return subj.update


def encode_hostname(subj: Hostname) -> object:
    return subj.fqdn


def encode_runcmd(subj: RunCMD) -> object:
    return subj.cmd


def encode_legacy_key_value(subj: LegacyKeyValue) -> object:
    return {
        subj.key: subj.value,
    }
