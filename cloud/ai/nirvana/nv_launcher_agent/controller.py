import os
import time
from threading import Thread, Lock

from cloud.ai.nirvana.nv_launcher_agent.deployer import Deployer, DeployStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import print_process_exit_status, ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.cli_process import CliProcess
from cloud.ai.nirvana.nv_launcher_agent.service import AgentServiceCreator
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class AgentController:
    def __init__(self, agent_port, log_dir, debug):
        self.agent_port = agent_port
        self.log_dir = log_dir
        self.debug = debug
        self._prepare_env()

        self.agent_process: CliProcess = self._run_agent()

        self.deployer = Deployer()
        self.deploy_status = DeployStatus.VERSIONS_ARE_EQUAL
        self.deploy_status_lock = Lock()

        self.agent_lock = Lock()

        self.controller_log_file = os.path.join(Config.get_log_dir(), 'controller.stdout.log')

        self.main_thread = Thread(target=self.__main_cycle)
        self.main_thread.start()

    def _prepare_env(self):
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)

    def update(self):
        with self.deploy_status_lock:
            self.deploy_status = DeployStatus.UPDATING

    def check_for_update(self) -> DeployStatus:
        with self.deploy_status_lock:
            return self.deploy_status

    def _check_for_update(self) -> DeployStatus:
        ThreadLogger.info(f'Checking for updates')
        if self.deploy_status == DeployStatus.NEED_UPDATE and self.deployer.has_aws_keys():
            return DeployStatus.NEED_UPDATE
        return self.deployer.check_for_update()

    def _run_agent(self):
        ThreadLogger.info("Running agent")
        agent_service_creator = AgentServiceCreator(port=self.agent_port, debug=self.debug)
        launch_args = agent_service_creator.get_launch_agent_args()
        ThreadLogger.info(f"Launch agent args:\n{launch_args}")

        run_agent_process = CliProcess(
            "run_agent",
            args=launch_args,
            log_dir=self.log_dir
        )
        run_agent_process.start()
        return run_agent_process

    def _restart_agent(self):
        with self.agent_lock:
            self._kill_agent()
            self.agent_process = self._run_agent()
            if self.agent_process.exit_code() is not None:
                print_process_exit_status(self.agent_process)

    def _kill_agent(self):
        if self.agent_process is not None:
            ThreadLogger.info(f"Killing process with pid={self.agent_process.pid()}")
            self.agent_process.kill()

    def _kill_all(self):
        with self.agent_lock:
            self._kill_agent()

        ThreadLogger.info(f"Restarting proxy")
        os._exit(1)

    def agent_alive(self):
        with self.agent_lock:
            return self.agent_process.status == ProcessStatus.RUNNING

    def __main_cycle(self):
        ThreadLogger.register_thread(self.controller_log_file)
        seconds_counter = 0

        while True:
            time.sleep(1)
            seconds_counter = (seconds_counter + 1) % 60

            if not self.agent_alive():
                ThreadLogger.info("Agent is dead, trying to up")
                self._restart_agent()

            if seconds_counter == 59:
                with self.deploy_status_lock:
                    self.deploy_status = self._check_for_update()

            with self.deploy_status_lock:
                if self.deploy_status == DeployStatus.UPDATING:
                    time.sleep(1)  # time to answer to NIRVANA from PROXY
                    self._kill_all()
