"""
Steps related to faked dataproc cluster without control-plane
"""

from behave import when, then
from helpers import tunnel


@then('tunnel to {port} is open')
@when('tunnel to {port} is open')
def step_tunnel_open(ctx, port):
    """
    Open ssh tunnel to specified service
    """
    cluster = ctx.state['clusters'][ctx.state['cluster']]
    fqdn = cluster['masternodes'][0]

    tunnel.tunnel_open(ctx, fqdn, port)
