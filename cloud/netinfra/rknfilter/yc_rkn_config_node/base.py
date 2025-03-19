import dns.resolver
from netaddr import IPNetwork
from collections import defaultdict


class Base:
    pass


class ContentEntry(Base):
    def __init__(
        self,
        content_id,
        rkn_rule_hash,
        ip_addresses,
        ip_prefixes,
        domains,
        urls,
        blocking_type,
    ):
        self.ip_addresses = ip_addresses
        self.ip_prefixes = ip_prefixes
        self.content_id = content_id
        self.rkn_rule_hash = rkn_rule_hash
        self.domains = domains
        self.urls = urls
        self.blocking_type = blocking_type


class NetaddrContainingClass(Base):
    def convert_ip_addresses_to_objects(self, ip_addresses):
        ip_address_objects = set()
        for ip_address in ip_addresses:
            ip_address_objects.add(IPNetwork("".join([ip_address, "/32"])))
        return ip_address_objects

    def convert_ip_prefixes_to_objects(self, ip_prefixes):
        ip_prefix_objects = set()
        for ip_prefix in ip_prefixes:
            ip_prefix_objects.add(IPNetwork(ip_prefix))
        return ip_prefix_objects


class RknBlockingRule(NetaddrContainingClass):
    def __init__(
        self,
        content_id,
        rkn_rule_hash,
        blocking_type,
        ip_addresses=set(),
        ip_prefixes=set(),
        domain=None,
        url=None,
        path=None,
        scheme="both",
        pcre=None,
        dns_cache=None,
    ):
        self.content_id = content_id
        self.blocking_type = blocking_type
        self.ip_addresses = ip_addresses
        self.ip_prefixes = ip_prefixes
        self.resolved_ip_addresses = set()
        self.ip_address_objects = set()
        self.resolved_ip_addresses_objects = set()
        self.ip_prefix_objects = set()
        self.rkn_rule_hash = rkn_rule_hash
        self.domain = domain
        self.scheme = scheme
        self.url = url
        self.dns_cache = dns_cache
        if self.dns_cache and domain:
            self.resolved_addresses = self.dns_cache.resolve(domain)
            self.ip_addresses.update(self.resolved_addresses)
        if self.ip_addresses:
            self.ip_address_objects = self.convert_ip_addresses_to_objects(
                self.ip_addresses
            )
        if self.resolved_ip_addresses:
            self.resolved_ip_addresses_objects = self.convert_ip_addresses_to_objects(
                self.resolved_ip_addresses
            )
        if self.ip_prefixes:
            self.ip_prefix_objects = self.convert_ip_prefixes_to_objects(
                self.ip_prefixes
            )
        self.path = path
        self.pcre = pcre


class RknAddressingDb(NetaddrContainingClass):
    def __init__(self):
        self.greylist_ip_address_set = set()
        self.greylist_ip_address_object_set = set()
        self.greylist_ip_prefix_set = set()
        self.greylist_ip_prefix_object_set = set()
        self.blacklist_ip_address_set = set()
        self.blacklist_ip_address_object_set = set()
        self.blacklist_ip_prefix_set = set()
        self.blacklist_ip_prefix_object_set = set()

    def include(self, rbr, use_resolved_addresses_only=False):
        if rbr.blocking_type == "ip":
            self.blacklist_ip_address_set.update(rbr.ip_addresses)
            self.blacklist_ip_address_object_set.update(rbr.ip_address_objects)
            self.blacklist_ip_prefix_set.update(rbr.ip_prefixes)
            self.blacklist_ip_prefix_object_set.update(rbr.ip_prefix_objects)
        else:
            self.greylist_ip_prefix_set.update(rbr.ip_prefixes)
            self.greylist_ip_prefix_object_set.update(rbr.ip_prefix_objects)
            if use_resolved_addresses_only:
                if rbr.resolved_ip_addresses:
                    print(
                        "greylist db size: {}".format(len(self.greylist_ip_address_set))
                    )
                    print(
                        "resolved addresses {}".format(len(rbr.resolved_ip_addresses))
                    )
                    print("blocking_type {}".format(rbr.blocking_type))
                self.greylist_ip_address_set.update(rbr.resolved_ip_addresses)
                self.greylist_ip_address_object_set.update(
                    rbr.resolved_ip_addresses_objects
                )
            else:
                self.greylist_ip_address_set.update(rbr.ip_addresses)
                self.greylist_ip_address_object_set.update(rbr.ip_address_objects)

    def count_all_routes(self):
        return (
            len(self.blacklist_ip_address_set)
            + len(self.blacklist_ip_prefix_set)
            + len(self.greylist_ip_address_set)
            + len(self.greylist_ip_prefix_set)
        )

    def count_host_routes(self):
        return len(self.blacklist_ip_address_set) + len(self.greylist_ip_address_set)

    def count_subnet_routes(self):
        return len(self.blacklist_ip_prefix_set) + len(self.greylist_ip_prefix_set)

    def count_blacklist_routes(self):
        return len(self.blacklist_ip_address_set) + len(self.blacklist_ip_prefix_set)


class DescriptivePrefixListElement(NetaddrContainingClass):
    def __init__(
        self,
        addressing_block_charachteristics,
        addressing_block_name,
        ip_prefixes=set(),
    ):
        self.addressing_block_charachteristics = addressing_block_charachteristics
        self.addressing_block_name = addressing_block_name
        self.ip_prefixes = ip_prefixes
        if self.ip_prefixes:
            self.ip_prefix_objects = self.convert_ip_prefixes_to_objects(
                self.ip_prefixes
            )
        else:
            self.ip_prefix_objects = set()

    def __lt__(self, other):
        return self.addressing_block_name < self.addressing_block_name

    def __le__(self, other):
        return self.addressing_block_name <= self.addressing_block_name

    def __eq__(self, other):
        return (
            self.addressing_block_charachteristics
            == other.addressing_block_charachteristics
            and self.addressing_block_name == other.addressing_block_name
            and self.ip_prefixes == other.ip_prefixes
            and self.ip_prefix_objects == other.ip_prefix_objects
        )

    def __hash__(self):
        return hash(
            (
                self.addressing_block_name,
                self.addressing_block_charachteristics,
                str(self.ip_prefixes),
            )
        )


class DnsCache(Base):
    def __init__(
        self,
        upstreams=[
            "8.8.8.8",
            "8.8.4.4",
            "77.88.8.8",
            "77.88.8.1",
            "2a02:6b8::1:1",
            "2a02:6b8:0:3400::1",
        ],
        requests_amount=0,
        extensive_debug=False,
    ):
        self.cache = {}
        self.resolver = dns.resolver.Resolver()
        self.resolver.nameservers = upstreams
        self.nameservers = upstreams
        self.resolver.timeout = 10
        self.resolver.lifetime = 10
        self.requests_amount = requests_amount
        self.extensive_debug = extensive_debug
        if self.extensive_debug:
            self.resolution_error_counter = 0
            self.resolution_errors = defaultdict(dict)
            self.dns_cache_hits = defaultdict(int)

    def resolve(self, domain):
        if domain not in self.cache:
            self.send_request_to_upstrem_server(domain)
        else:
            if self.extensive_debug:
                self.dns_cache_hits[domain] += 1
        return self.cache[domain]

    def send_request_to_upstrem_server(self, domain):
        self.cache[domain] = set()
        for resolution_attempt in range(self.requests_amount):
            for nameserver in self.nameservers:
                try:
                    self.resolver.nameservers = [nameserver]
                    self.cache[domain].update(
                        str(ip) for ip in self.resolver.query(domain, "A")
                    )
                except dns.exception.DNSException as dns_exception_message:
                    #                    print(dns_exception_message)
                    if self.extensive_debug:
                        self.resolution_error_counter += 1
                #        self.resolution_errors[domain][nameserver][resolution_attempt] = dns_exception_message
        return self.cache[domain]

    # def dump_dns_cache_data(
    #     self,
    #     extensive_debug_directory="/tmp", dns_cache_file_name="cache.yaml",
    #     dns_cache_hits_file_name="cache_hits.yaml", resolution_errors_file_name="resolution_errors.yaml"
    # ):
    #     if not os.path.exists(extensive_debug_directory):
    #         os.makedirs(extensive_debug_directory)
    #     dns_cache_file_path = os.path.join(extensive_debug_directory, dns_cache_file_name)
    #     with open(dns_cache_file_path, "w") as dns_cache_file:
    #         yaml.dump(self.cache, dns_cache_file)
    #     if self.extensive_debug:
    #         dns_cache_hits_file_path = os.path.join(extensive_debug_directory, dns_cache_hits_file_name)
    #         resolution_errors_file_path = os.path.join(extensive_debug_directory, resolution_errors_file_name)
    #         with open(dns_cache_hits_file_path, "w") as dns_cache_hits_file:
    #             yaml.dump(self.dns_cache_hits, dns_cache_hits_file)
    #         with open(resolution_errors_file_path, "w") as resolution_errors_file:
    #             yaml.dump(self.resolution_errors, resolution_errors_file)
    #         print("Appeared " + str(self.resolution_error_counter) + " resolution errors")
