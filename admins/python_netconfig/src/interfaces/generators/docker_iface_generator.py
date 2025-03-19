from ..docker_bridge_interface import DockerBridgeInterface


class DockerIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces):
        if 'docker-bridge' in parameters.host_tags:
            iface = DockerBridgeInterface(network_info)
            iface.name = 'docker0'
            interfaces.add_interface(iface)
