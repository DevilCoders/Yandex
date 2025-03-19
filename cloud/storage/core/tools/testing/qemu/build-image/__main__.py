import argparse
import os
import subprocess
import yaml
from urllib.request import urlretrieve
import tempfile


def download_img(args):
    img_url = ("{cloud_images_mirror}/{release}/current/"
               "{release}-server-cloudimg-{arch}.img".format(**vars(args)))

    print("Downloading", img_url, end='', flush=True)

    def progress(blocknum, bs, size):
        if blocknum * bs % (10 << 20) < bs:
            print('.', end='', flush=True)

    urlretrieve(img_url, filename=args.out, reporthook=progress)

    print("done", flush=True)


def write_meta_data(filename):
    meta_data = {
        'instance-id': 'test',
        'local-hostname': 'test'
    }

    with open(filename, 'w') as f:
        f.write(yaml.dump(meta_data))


def ssh_pubkey_blob(ssh_privkey):
    # use pipe to make it work regardless of the permissions on the key file
    privkey_blob = open(ssh_privkey).read()
    proc = subprocess.run(["ssh-keygen", "-y", "-f", "/dev/fd/0"],
                          input=privkey_blob, stdout=subprocess.PIPE,
                          check=True, universal_newlines=True)
    return proc.stdout


def write_user_data(filename, args):
    pubkey_blob = ssh_pubkey_blob(args.ssh_key)

    user_data = {
        'users': [
            {
                'name': args.user,
                'plain_text_passwd': 'qemupass',
                'lock_passwd': False,
                'ssh_pwauth': True,
                'ssh_authorized_keys': [pubkey_blob],
                'sudo': ['ALL=(ALL) NOPASSWD:ALL'],
                'shell': '/bin/bash',
            },
        ],
        'runcmd': [
            "echo '### use login: qemu password: qemupass' >> /etc/issue",
        ],
        'apt': {
            'primary': [
                {
                    'arches': ['default'],
                    'uri': args.repo_mirror,
                },
            ],
        },
        'packages': [
            'nfs-common',
        ],
        'power_state': {
            'delay': 'now',
            'mode': 'poweroff',
        },
    }

    with open(filename, 'w') as f:
        f.write('#cloud-config\n' + yaml.dump(user_data))


def write_network_config(filename):
    # Minimal basic configuration.  Without it the guest would try to rename
    # the interface to the name from cloud-init time, which races with DHCP
    # client and may result in non-operational network.
    network_config = {
        'version': 2,
        'ethernets': {
            'eth0': {
                'dhcp4': True,
                'match': {
                    'name': '*'
                }
            }
        }
    }

    with open(filename, 'w') as f:
        f.write(yaml.dump(network_config))


def mk_cidata_iso(args, tmpdir):
    meta_data = os.path.join(tmpdir, 'meta-data')
    write_meta_data(meta_data)
    user_data = os.path.join(tmpdir, 'user-data')
    write_user_data(user_data, args)
    network_config = os.path.join(tmpdir, 'network-config')
    write_network_config(network_config)

    cidata_iso = os.path.join(tmpdir, 'cidata.iso')
    subprocess.check_call(['genisoimage', '-V', 'CIDATA', '-R',
                           '-o', cidata_iso,
                           meta_data, user_data, network_config])

    return cidata_iso


def qemu_bin(args):
    deb_arch_map = {
        'amd64': 'x86_64',
    }

    return "qemu-system-{}".format(deb_arch_map[args.arch])


def customize(args, cidata_iso):
    # arbitrary, should be enough
    nproc = 2
    mem = '2G'

    cmd = [
        qemu_bin(args),
        "-nodefaults",
        "-accel", "kvm",
        "-smp", str(nproc),
        "-m", mem,
        "-cpu", "host",
        "-netdev", "user,id=netdev0",
        "-device", "virtio-net-pci,netdev=netdev0,id=net0",
        "-smbios", "type=1,serial=ds=nocloud",
        "-nographic",
        "-drive", "format=qcow2,file={},id=hdd0,if=none,aio=native,cache=none".format(args.out),
        "-device", "virtio-blk-pci,id=vblk0,drive=hdd0,num-queues={},bootindex=1".format(nproc),
        "-drive", "format=raw,file={},id=cidata,if=none,readonly=on".format(cidata_iso),
        "-device", "virtio-blk-pci,id=vblk1,drive=cidata",
        "-serial", "stdio",
    ]

    subprocess.check_call(cmd, timeout=5 * 60)


def main(args):
    download_img(args)

    with tempfile.TemporaryDirectory() as tmpdir:
        cidata_iso = mk_cidata_iso(args, tmpdir)

        customize(args, cidata_iso)

    print("\n{} is ready to use. Upload to sandbox with\n".format(args.out))
    print("ya upload --ttl=inf -T NBS_QEMU_DISK_IMAGE \\")
    print("    -d 'Customized Ubuntu {} Cloud Image in QCOW2 format' \\".format(args.release.capitalize()))
    print("    {}".format(args.out))


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", help="output file path", default="rootfs.img")
    parser.add_argument("--release", help="Ubuntu release name",
                        default="focal")
    parser.add_argument("--arch", help="Ubuntu architecture", default="amd64")
    parser.add_argument("--cloud-images-mirror",
                        help="http://cloud-images.ubuntu.com mirror",
                        default="http://mirror.yandex.ru/ubuntu-cloud-images")
    parser.add_argument("--repo-mirror", help="Ubuntu repository mirror",
                        default="http://mirror.yandex.ru/ubuntu")
    parser.add_argument("--user", help="guest user", default="qemu")
    parser.add_argument("--ssh-key", help="private ssh key for guest user",
                        required=True)

    return parser.parse_args()

if __name__ == "__main__":
    main(parse_args())
