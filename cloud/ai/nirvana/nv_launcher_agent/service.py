import os
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config


class AgentServiceCreator:
    def __init__(self, service_name='Nirvana job launcher agent', mode='agent', debug=True, port=7842):
        self.service_name = service_name
        self.service_mode = mode
        self.debug = debug
        self.port = port

    def get_service_file(self):
        return '\n\n'.join([
            self.get_unit_description(),
            self.get_service_description(),
            self.get_install_description()
        ])

    def get_unit_description(self):
        return '\n'.join([
            "[Unit]",
            f"Description='{self.service_name}'",
            "After=network-online.target",
            "Wants=network-online.target"
        ])

    def get_launch_agent_args(self):
        launch_str = [os.path.join(Config.get_project_dir(), 'nv_launcher_agent')]
        if self.debug:
            launch_str = ['/opt/conda/bin/python', os.path.join(Config.get_project_dir(), "app.py")]
        return launch_str + [
            "agent",
            "--working-dir", f"{Config.get_working_dir()}/nv/jobs",
            "--layers-dir", f"{Config.get_working_dir()}/nv/layers",
            "--port", str(self.port)
        ]

    def get_exec_start(self):
        run_agent_args = self.get_launch_agent_args()
        run_agent_args[0] = 'ExecStart=' + run_agent_args[0]
        return '\\\n'.join(run_agent_args)

    def get_service_description(self):
        return '\n'.join([
            "[Service]",
            self.get_exec_start(),
            "ExecStop=/bin/kill -INT $MAINPID",
            "ExecReload=/bin/kill -TERM $MAINPID",
            "",
            "Restart=always",
            "Type=simple"
        ])

    def get_install_description(self):
        return '\n'.join([
            "[Install]",
            "WantedBy=multi-user.target:"
        ])


def test():
    print(AgentServiceCreator().get_service_file())


if __name__ == '__main__':
    test()
