from dataclasses import dataclass
import itertools

from .errors import Error


@dataclass
class TestCase:
    disk_type: str
    disk_size: int  # bytes
    disk_bs: int  # bytes
    rw: str
    bs: int
    iodepth: int
    device_name: str  # /dev/vdb by default
    rw_mix_read: int  # rw_mix_write = 100 - rw_mix_read

    def __eq__(self, other):
        if isinstance(other, TestCase):
            return self.name == other.name
        raise NotImplementedError

    def __hash__(self):
        return hash(self.name)

    @property
    def rw_mix_write(self) -> int:
        return 100 - self.rw_mix_read

    @property
    def bs_formatted(self) -> str:
        if self.bs % (1024 ** 2) == 0:
            return f'{self.bs // 1024 ** 2}M'
        elif self.bs % 1024 == 0:
            return f'{self.bs // 1024}K'
        else:
            return f'{self.bs}bytes'

    @property
    def disk_size_formatted(self) -> str:
        if self.disk_size % 1024 == 0:
            return f'{self.disk_size // 1024}TB'
        else:
            return f'{self.disk_size}GB'

    @property
    def fio_cmd(self) -> str:
        return (f'fio --name=test --filename={self.device_name} --rw={self.rw}'
                f' --bs={self.bs} --iodepth={self.iodepth} --direct=1 --sync=1'
                f' --ioengine=libaio --rwmixwrite={self.rw_mix_write}'
                f' --rwmixread={self.rw_mix_read} --runtime=120 --time_based=1'
                f' --size={160 * 1024 ** 3} --output-format=json')

    @property
    def name(self) -> str:
        return (f'{self.disk_type}-{self.disk_size_formatted}-{self.rw}'
                f'-{self.bs_formatted}-{self.iodepth}')


def generate_test_cases(test_suite: str, cluster_name: str) -> list[TestCase]:
    test_suite_to_func = {
        'default': _generate_test_cases_for_default_test_suite,
        'max_iops': _generate_test_cases_for_max_iops_test_suite,
        'max_bandwidth': _generate_test_cases_for_max_bandwidth_test_suite,
        'default_nbd': _generate_test_cases_for_default_test_suite,
        'max_iops_nbd': _generate_test_cases_for_max_iops_test_suite,
        'max_bandwidth_nbd': _generate_test_cases_for_max_bandwidth_test_suite,
        'nrd': _generate_test_cases_for_nrd_test_suite,
        'nrd_vhost': _generate_test_cases_for_nrd_test_suite,
        'nrd_nbd': _generate_test_cases_for_nrd_test_suite,
        'large_block_size': _generate_test_cases_for_large_bs_test_suite,
        'overlay_disk_max_count': _generate_test_cases_for_overlay_disk_max_count_suite,
    }
    try:
        func = test_suite_to_func[test_suite]
    except KeyError:
        raise Error(f'no such test suite <{test_suite}>')
    return func(cluster_name)


def _generate_test_cases_for_default_test_suite(
        cluster_name: str) -> list[TestCase]:
    return [
        TestCase(disk_type,
                 disk_size,
                 disk_bs,
                 rw,
                 bs,
                 iodepth,
                 '/dev/vdb',
                 rw_mix_read)
        for disk_type,
            disk_size,
            disk_bs,
            rw,
            bs,
            iodepth,
            rw_mix_read in itertools.product(
            ['network-ssd', 'network-ssd-v2'],
            [320],  # GB
            [4 * 1024],  # bytes
            ['randrw'],
            [4 * 1024, 128 * 1024],  # bytes
            [1, 32],
            [50]
        )
    ]


def _generate_test_cases_for_max_iops_test_suite(
        cluster_name: str) -> list[TestCase]:
    return [
        TestCase(disk_type,
                 disk_size,
                 disk_bs,
                 rw,
                 bs,
                 iodepth,
                 '/dev/vdb',
                 rw_mix_read)
        for disk_type,
            disk_size,
            disk_bs,
            rw,
            bs,
            iodepth,
            rw_mix_read in itertools.product(
            ['network-ssd', 'network-ssd-v2'],
            [960],  # GB
            [4 * 1024],  # bytes
            ['randread', 'randwrite', 'randrw'],
            [4 * 1024],  # bytes
            [256],
            [50]
        )
    ]


def _generate_test_cases_for_max_bandwidth_test_suite(
        cluster_name: str) -> list[TestCase]:
    return [
        TestCase(disk_type,
                 disk_size,
                 disk_bs,
                 rw,
                 bs,
                 iodepth,
                 '/dev/vdb',
                 rw_mix_read)
        for disk_type,
            disk_size,
            disk_bs,
            rw,
            bs,
            iodepth,
            rw_mix_read in itertools.product(
            ['network-ssd', 'network-ssd-v2'],
            [960],  # GB
            [4 * 1024],  # bytes
            ['randread', 'randwrite', 'randrw'],
            [4 * 1024 ** 2],  # bytes
            [256],
            [50]
        )
    ]


def _generate_test_cases_for_nrd_test_suite(
        cluster_name: str) -> list[TestCase]:
    disk_size = 93 * 20  # GB

    if cluster_name == 'preprod' or cluster_name == 'hw-nbs-stable-lab':
        disk_size = 93 * 8  # GB

    return [
        TestCase(disk_type,
                 disk_size,
                 disk_bs,
                 rw,
                 bs,
                 iodepth,
                 '/dev/vdb',
                 rw_mix_read)
        for disk_type,
            disk_size,
            disk_bs,
            rw,
            bs,
            iodepth,
            rw_mix_read in itertools.product(
            ['network-ssd-nonreplicated'],
            [disk_size],
            [4 * 1024],  # bytes
            ['randread', 'randwrite', 'randrw'],
            [4 * 1024, 128 * 1024],  # bytes
            [1, 32, 256],
            [50]
        )
    ] + [
        TestCase(disk_type,
                 disk_size,
                 disk_bs,
                 rw,
                 bs,
                 iodepth,
                 '/dev/vdb',
                 rw_mix_read)
        for disk_type,
            disk_size,
            disk_bs,
            rw,
            bs,
            iodepth,
            rw_mix_read in itertools.product(
            ['network-ssd-nonreplicated'],
            [disk_size],  # GB
            [4 * 1024],  # bytes
            ['randread', 'randwrite', 'randrw'],
            [4 * 1024 ** 2],  # bytes
            [1, 32],
            [50]
        )
    ]


def _generate_test_cases_for_large_bs_test_suite(
        cluster_name: str) -> list[TestCase]:
    return [
        TestCase(disk_type,
                 disk_size,
                 disk_bs,
                 rw,
                 bs,
                 iodepth,
                 '/dev/vdb',
                 rw_mix_read)
        for disk_type,
            disk_size,
            disk_bs,
            rw,
            bs,
            iodepth,
            rw_mix_read in itertools.product(
            ['network-ssd', 'network-ssd-v2'],
            [320],  # GB
            [64 * 1024],  # bytes
            ['randrw'],
            [4 * 1024, 128 * 1024],  # bytes
            [32],
            [50]
        )
    ]


def _generate_test_cases_for_overlay_disk_max_count_suite(
        cluster_name: str) -> list[TestCase]:
    return [
        TestCase(disk_type,
                 disk_size,
                 disk_bs,
                 rw,
                 bs,
                 iodepth,
                 '/dev/vdb',
                 rw_mix_read)
        for disk_type,
            disk_size,
            disk_bs,
            rw,
            bs,
            iodepth,
            rw_mix_read in itertools.product(
            ['network-ssd'],
            [320, 325],  # GB
            [4 * 1024],  # bytes
            ['randrw'],
            [4 * 1024 ** 2],  # bytes
            [32],
            [75]
        )
    ]
