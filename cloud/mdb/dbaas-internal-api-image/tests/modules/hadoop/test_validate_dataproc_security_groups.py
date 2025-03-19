"""
Tests for validate_dataproc_security_groups, do_validate_dataproc_security_groups
"""
import pytest
from flask import Flask
from flask_appconfig import AppConfig
from unittest.mock import Mock
from typing import List, Optional

from dbaas_internal_api.modules.hadoop.validation import (
    validate_dataproc_security_groups,
    do_validate_dataproc_security_groups,
    S3_HOSTNAME,
    MANAGER_HOSTNAME,
    MONITORING_HOSTNAME,
    UI_PROXY_HOSTNAME,
    GATEWAY_URL,
    LOGGING_INGESTER_URL,
)
from dbaas_internal_api.core.exceptions import DbaasClientError
from dbaas_internal_api.utils.network import SecurityGroup, SecurityGroupRule, SecurityGroupRuleDirection

INGRESS = SecurityGroupRuleDirection.INGRESS
EGRESS = SecurityGroupRuleDirection.EGRESS

S3_IP = '213.180.193.243'
MANAGER_IP = '213.180.193.244'
MONITORING_IP = '213.180.193.245'
UI_PROXY_IP = '213.180.193.246'
GATEWAY_IP = '213.180.193.247'
LOGGING_INGESTER_IP = '213.180.193.248'
HOSTNAMES = {
    S3_HOSTNAME: S3_IP,
    MANAGER_HOSTNAME: MANAGER_IP,
    MONITORING_HOSTNAME: MONITORING_IP,
    UI_PROXY_HOSTNAME: UI_PROXY_IP,
    GATEWAY_URL: GATEWAY_IP,
    LOGGING_INGESTER_URL: LOGGING_INGESTER_IP,
}


@pytest.fixture
def app_config():
    """
    Allows testing code to user current_app.config
    """
    app = Flask("dbaas_internal_api")
    AppConfig(app, None)
    app.config['DNS_CACHE'] = HOSTNAMES

    with app.app_context():
        yield app.config


# tests for public ip in validate_dataproc_security_groups
def test_fails_with_public_ip_and_0_security_groups(app_config):
    groups = make_groups(0)
    with pytest.raises(DbaasClientError):
        validate_dataproc_security_groups(make_pillar(), True, True, groups)


def test_works_with_no_public_ip_and_0_security_groups(app_config):
    groups = make_groups(0)
    validate_dataproc_security_groups(make_pillar(), True, False, groups)


# tests for do_validate_dataproc_security_groups
def test_works_with_self_security_group(app_config):
    groups = make_groups(1)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group')
    add_rule(groups[0], direction=EGRESS, predefined_target='self_security_group')
    do_validate_dataproc_security_groups(make_pillar(), False, groups)


def test_works_with_all_ips_mask(app_config):
    groups = make_groups(1)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group')
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=['0.0.0.0/0'])
    do_validate_dataproc_security_groups(make_pillar(), False, groups)


def test_works_with_all_ports_range(app_config):
    groups = make_groups(1)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group')
    add_rule(groups[0], direction=EGRESS, predefined_target='self_security_group', ports_from=0, ports_to=65535)
    do_validate_dataproc_security_groups(make_pillar(), False, groups)


def test_works_with_multiple_groups(app_config):
    groups = make_groups(2)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group')
    add_rule(groups[1], direction=EGRESS, predefined_target='self_security_group')
    do_validate_dataproc_security_groups(make_pillar(), False, groups)


def test_fails_when_tcp_is_not_allowed(app_config):
    groups = make_groups(1)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group', protocol_name='ICMP')
    add_rule(groups[0], direction=EGRESS, predefined_target='self_security_group')
    with pytest.raises(DbaasClientError):
        do_validate_dataproc_security_groups(make_pillar(), False, groups)


def test_fails_when_no_ports_are_allowed(app_config):
    groups = make_groups(1)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group')
    add_rule(groups[0], direction=EGRESS, predefined_target='self_security_group', ports_from=0, ports_to=1024)
    with pytest.raises(DbaasClientError):
        do_validate_dataproc_security_groups(make_pillar(), False, groups)


def test_fails_when_not_all_ips_are_allowed(app_config):
    groups = make_groups(1)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group')
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=['84.0.0.0/8'])
    with pytest.raises(DbaasClientError):
        do_validate_dataproc_security_groups(make_pillar(), False, groups)


def test_service_endpoints_should_be_allowed(app_config):
    groups = make_groups(1)
    add_rule(groups[0], direction=INGRESS, predefined_target='self_security_group')
    add_rule(groups[0], direction=EGRESS, predefined_target='self_security_group')
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(S3_IP), port=443)
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(MANAGER_IP), port=443)
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(MONITORING_IP), port=443)
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(UI_PROXY_IP), port=443)
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(GATEWAY_IP), port=443)
    add_rule(groups[0], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(LOGGING_INGESTER_IP), port=443)
    do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_fails_if_service_endpoint_egress_is_missing(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    add_all_rules_except_s3(groups[1])
    add_rule(groups[1], direction=INGRESS, v4_cidr_blocks=as_cidr_blocks(S3_IP), port=443)
    with pytest.raises(DbaasClientError):
        do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_accepts_all_ports_for_service_endpoint(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    add_all_rules_except_s3(groups[1])
    add_rule(groups[1], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(S3_IP))
    do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_accepts_wider_range_for_service_endpoint(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    add_all_rules_except_s3(groups[1])
    add_rule(groups[1], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(S3_IP), ports_from=0, ports_to=1024)
    do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_fails_if_service_endpoint_port_is_invalid(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    add_all_rules_except_s3(groups[1])
    add_rule(groups[1], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(S3_IP), port=80)
    with pytest.raises(DbaasClientError):
        do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_accepts_service_endpoint_with_all_protocols_allowed(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    add_all_rules_except_s3(groups[1])
    add_rule(groups[1], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(S3_IP), port=443, protocol_name='ANY')
    do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_fails_if_service_endpoint_protocol_is_not_valid(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    add_all_rules_except_s3(groups[1])
    add_rule(groups[1], direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(S3_IP), port=443, protocol_name='ICMP')
    with pytest.raises(DbaasClientError):
        do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_allows_to_specify_service_endpoints_with_wide_cidr(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    add_rule(groups[1], direction=EGRESS, v4_cidr_blocks=['213.180.193.0/24'], port=443)
    do_validate_dataproc_security_groups(make_pillar(), True, groups)


def test_service_endpoints_error_text(app_config):
    groups = make_groups(2)
    allow_internal_traffic(groups[0])
    text = (
        'It is required that security groups allow egress TCP traffic to the following list '
        'of Yandex Cloud service endpoints for the Data Proc cluster to function properly: '
        '213.180.193.243:443 (required to access Object Storage), '
        '213.180.193.244:443 (required for reporting of service health and running Data Proc jobs), '
        '213.180.193.245:443 (required for Data Proc service monitoring and autoscaling), '
        '213.180.193.246:443 (used by Data Proc UI Proxy)'
    )
    with pytest.raises(DbaasClientError) as exc:
        do_validate_dataproc_security_groups(make_pillar(), True, groups)
    assert text in exc.value.message


def make_groups(num=1):
    groups = []
    for _ in range(num):
        group = SecurityGroup(id='', name='', folder_id='', network_id='', rules=[])
        groups.append(group)
    return groups


def make_pillar():
    pillar = Mock()
    pillar.s3 = {}
    pillar.agent = {}
    pillar.monitoring = {}
    pillar.logging = {}
    return pillar


def add_rule(
    group: SecurityGroup,
    direction=None,
    port: Optional[int] = None,
    ports_from: Optional[int] = None,
    ports_to: Optional[int] = None,
    protocol_name: str = 'TCP',
    v4_cidr_blocks: List[str] = None,
    predefined_target: Optional[str] = None,
):
    enum_direction = None
    if direction == INGRESS:
        enum_direction = SecurityGroupRuleDirection.INGRESS.value
    elif direction == EGRESS:
        enum_direction = SecurityGroupRuleDirection.EGRESS.value
    if port:
        ports_from = ports_to = port
    rule = SecurityGroupRule(
        id='',
        description='',
        direction=enum_direction,
        ports_from=ports_from,
        ports_to=ports_to,
        protocol_name=protocol_name,
        protocol_number=0,
        v4_cidr_blocks=v4_cidr_blocks or [],
        v6_cidr_blocks=[],
        predefined_target=predefined_target,
        security_group_id=None,
    )
    group.rules.append(rule)


def allow_internal_traffic(group: SecurityGroup):
    add_rule(group, direction=INGRESS, predefined_target='self_security_group')
    add_rule(group, direction=EGRESS, predefined_target='self_security_group')


def add_all_rules_except_s3(group: SecurityGroup):
    add_rule(group, direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(MANAGER_IP), port=443)
    add_rule(group, direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(MONITORING_IP), port=443)
    add_rule(group, direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(UI_PROXY_IP), port=443)
    add_rule(group, direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(GATEWAY_IP), port=443)
    add_rule(group, direction=EGRESS, v4_cidr_blocks=as_cidr_blocks(LOGGING_INGESTER_IP), port=443)


def as_cidr_blocks(ip):
    return [f'{ip}/32']
