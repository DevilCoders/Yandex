# coding: utf-8
from .common import BaseProvider
from .iam_jwt import IamJwt
from .metadb_security_group import MetadbSecurityGroup
from ..exceptions import ExposedException

from cloud.mdb.internal.python.compute import vpc
from cloud.mdb.internal.python import grpcutil


class VPCApiError(ExposedException):
    """
    Base error
    """


class VPCProvider(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)

        self.vpc_iam_jwt = IamJwt(
            config,
            task,
            queue,
            service_account_id=self.config.vpc.service_account_id,
            key_id=self.config.vpc.key_id,
            private_key=self.config.vpc.private_key,
        )

        timeout = 10.0
        self._vpc = vpc.VPC(
            config=vpc.Config(
                transport=grpcutil.Config(
                    url=self.config.vpc.url,
                    cert_file=self.config.compute.ca_path,
                ),
                timeout=timeout,
            ),
            logger=self.logger,
            token_getter=self._get_token,
        )
        self.metadb_sg = MetadbSecurityGroup(config, task, queue)

    def delete_service_security_group(self, cid: str):
        sg_info = self.metadb_sg.get_sgroup_info(cid)
        if sg_info.service_sg:
            self.delete_security_group(sg_info.service_sg)
            self.metadb_sg.delete_sgroups('service')

    def _get_token(self):
        return self.vpc_iam_jwt.get_token()

    def get_subnet(self, subnet_id) -> vpc.Subnet:
        """
        Get subnet by id
        """
        return self._vpc.get_subnet(subnet_id)

    def get_subnets(self, network_id):
        """
        Get subnet by id
        """
        return self._vpc.get_subnets(network_id)

    def get_network(self, id: str) -> vpc.Network:
        """
        Get network by id
        """
        return self._vpc.get_network(id)

    def get_security_group(self, id):
        """
        Get security group by id
        """
        return self._vpc.get_security_group(id)

    def delete_security_group(self, id):
        """
        Delete security group by id
        """
        return self._vpc.delete_security_group(id)

    def get_geo_subnet(self, folder_id, network_id, zone_id):
        """
        Get first subnet for folder/network/zone combination
        """
        for subnet in self._vpc.get_subnets(network_id):
            if subnet.zone_id == zone_id and subnet.folder_id == folder_id:
                return subnet

        raise VPCApiError(
            'Unable to find subnet for network {network} in {zone}'.format(network=network_id, zone=zone_id)
        )

    def _sg_rules_to_api_rules(self, rules):
        protocol_map = {
            'ICMP': 1,
            'TCP': 6,
            'UDP': 17,
            'IPV6_ICMP': 58,
            'ANY': None,
        }
        api_rules = []
        for rule in rules:
            rule_direction = rule.get('direction', 'BOTH')
            if rule_direction == 'INGRESS':
                directions = [vpc.SecurityGroupRuleDirection.INGRESS]
            elif rule_direction == 'EGRESS':
                directions = [vpc.SecurityGroupRuleDirection.EGRESS]
            else:
                directions = [vpc.SecurityGroupRuleDirection.INGRESS, vpc.SecurityGroupRuleDirection.EGRESS]
            protocol = rule.get('protocol', 'TCP')
            for direction in directions:
                v4_cidr_blocks = rule.get('v4_cidr_blocks', [])
                v6_cidr_blocks = rule.get('v6_cidr_blocks', [])
                security_group_id = rule.get('security_group_id')
                predefined_target = rule.get('predefined_target')
                if not predefined_target and not v4_cidr_blocks and not v6_cidr_blocks and not security_group_id:
                    predefined_target = 'self_security_group'
                api_rule = vpc.SecurityGroupRule(
                    id=rule.get('id', ''),
                    description=rule.get('description', ''),
                    direction=direction,
                    protocol_name=protocol,
                    protocol_number=protocol_map[protocol],
                    ports_from=rule.get('ports_from', 0),
                    ports_to=rule.get('ports_to', 0),
                    v4_cidr_blocks=v4_cidr_blocks,
                    v6_cidr_blocks=v6_cidr_blocks,
                    predefined_target=predefined_target,
                    security_group_id=security_group_id,
                )
                api_rules.append(api_rule)
        return api_rules

    def create_service_security_group(self, user_network_id, rules):
        """
        VPC Create Security Group
        """
        cid = self.task['cid']
        name = f'service_cid_{cid}'
        api_rules = self._sg_rules_to_api_rules(rules)
        return self._vpc.create_security_group(name, self.config.compute.folder_id, user_network_id, api_rules)

    def set_security_group_rules(self, sg_id, rules):
        """
        VPC Update Security Group Rules
        """
        api_rules = self._sg_rules_to_api_rules(rules)
        return self._vpc.set_security_group_rules(sg_id, api_rules)

    def set_superflow_v22_flag_on_instance(self, instance_id):
        """
        VPC Set Superflow v 2.2 Flag On Instance
        """
        feature_flag_id = 'super-flow-v2.2'
        return self._vpc.set_superflow_v22_flag_on_instance(feature_flag_id=feature_flag_id, instance_id=instance_id)
