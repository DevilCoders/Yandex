import logging
import os
import shutil
import signal
import subprocess
import threading
import time
import traceback
from typing import Optional

from agent.config.models import ProberConfig, Prober, ClusterMetadata
from agent.process_buffer import ProcessOutputBuffer
from common.monitoring.solomon import SolomonClient
from agent.status import ProberExecutionStatus
from agent.telemetry import ProberTelemetrySender
from agent.timer import Timer


class ProberProcess:
    def __init__(
        self, prober: Prober, config: ProberConfig, cluster: ClusterMetadata,
        solomon_client: SolomonClient, delay_seconds: int = 0
    ):
        self._prober = prober
        # `updated_prober` is a pending prober update. It self._prober != self._updated_prober,
        # then self._prober will be updated before next execution.
        self._updated_prober = prober
        self._config = config
        self._cluster = cluster

        # First, we wait for `delay_seconds` for the first start.
        # Before that moment `ProberProcess.need_start()` returns False.
        self._waiting_for_first_start = True
        self._delay_seconds = delay_seconds

        self._start_timestamp: Optional[int] = None
        self._timer = Timer()
        self._process: Optional[subprocess.Popen] = None

        self.stdout: Optional[ProcessOutputBuffer] = None
        self.stderr: Optional[ProcessOutputBuffer] = None

        self._telemetry_sender = ProberTelemetrySender(solomon_client, prober, config, cluster)

        # This flag is set by `stop_forever()` before deleting the prober. It prevents process to be started
        # again in another thread.
        self._stopped_forever = False
        self._lock = threading.RLock()

    def start(self):
        """
        Start new prober process
        """
        with self._lock:
            if self._stopped_forever:
                return

            if self._process is not None:
                raise RuntimeError("Can't run prober until previous instance is running")

            # Apply pending prober update. We update prober strictly between executions
            if self._updated_prober != self._prober:
                # Remove old prober files
                if self._updated_prober.files_location != self._prober.files_location:
                    shutil.rmtree(self._prober.files_location)
                self._prober = self._updated_prober
                self._telemetry_sender.update_prober_and_config(self._prober, self._config)

            self._waiting_for_first_start = False
            self._timer.restart()
            # Send metrics and logs as happened at execution start time
            self._start_timestamp = int(time.time())

            logging.info(
                f"Running prober {self._prober.name!r} ({str(self._prober.runner)!r}) "
                f"with variables {self._config.variables!r} "
                f"on {self._prober.files_location.as_posix()} "
                f"with timeout {self._config.timeout_seconds} seconds",
            )
            self._telemetry_sender.send_telemetry_about_prober_start()

            try:
                self._process = self._prober.runner.create_process(self._prober, self._config)
                self.stdout = ProcessOutputBuffer(self._process.stdout)
                self.stderr = ProcessOutputBuffer(self._process.stderr)
            except Exception as e:
                logging.warning(f"Can't start process for prober: {e}", exc_info=e)
                self._telemetry_sender.send_telemetry_about_prober_execution(
                    self._start_timestamp, self._timer.get_total_seconds(),
                    ProberExecutionStatus.FAILED_TO_START, stderr=traceback.format_exc().encode()
                )

    def update_prober_if_changed(self, prober: Prober):
        if self._updated_prober != prober:
            logging.info(
                f"Prober {self._prober.name!r} has been changed, update it correspondingly. "
                f"Previous value: [{self._prober}], new value: [{prober}]"
            )
            self._updated_prober = prober

    def update_config_if_changed(self, config: ProberConfig):
        if self._config != config:
            logging.info(
                f"Config of the prober {self._prober.name!r} has been changed, "
                f"update it correspondingly. Previous value: [{self._config}], new value: [{config}]",
            )
            self._config = config

    def _read_buffers(self):
        self.stdout.read_once()
        self.stderr.read_once()

    def _read_buffers_and_close(self):
        self._read_buffers()
        self._process.stdout.close()
        self._process.stderr.close()

    def check_status(self):
        """
        This function is called ~100 times in a second. It checks the process' status (is it exited?) and kill it
        if time is out.
        """
        with self._lock:
            if self._process is None:
                # Process already finished, wait for the next run
                return

            self._read_buffers()
            exit_code = self._process.poll()
            if exit_code is not None:
                # Process has been exited before the time is over
                self._read_buffers_and_close()
                self._process = None
                execution_status = ProberExecutionStatus.SUCCESS if exit_code == 0 else ProberExecutionStatus.FAIL

                self._telemetry_sender.send_telemetry_about_prober_execution(
                    self._start_timestamp, self._timer.get_total_seconds(),
                    execution_status, exit_code, self.stdout.get_data(), self.stderr.get_data()
                )
                return

            if self._timer.get_total_seconds() > self._config.timeout_seconds:
                # Stop the process, because time is over
                self.stop()

                self._telemetry_sender.send_telemetry_about_prober_execution(
                    self._start_timestamp, self._timer.get_total_seconds(),
                    ProberExecutionStatus.TIMEOUT, None, self.stdout.get_data(), self.stderr.get_data()
                )
                return

            # In other cases just wait next iteration

    def need_start(self) -> bool:
        if self._process is not None:
            # Don't restart prober until previous instance is not finished
            return False

        if self._waiting_for_first_start:
            return self._timer.get_total_seconds() >= self._delay_seconds

        return self._timer.get_total_seconds() > self._config.interval_seconds

    def _kill(self):
        """
        Send the signal to all the process groups
        """
        return os.killpg(os.getpgid(self._process.pid), signal.SIGKILL)

    def stop(self):
        with self._lock:
            if self._process is None:
                # Process has been already stopped
                return

            logging.warning(
                f"Kill the process {self._process.pid} for prober {self._prober.name!r}, because time is out"
            )
            try:
                self._kill()
                self._process.wait()
            except Exception as e:
                logging.error(
                    f"Can't kill the process {self._process.pid} for prober {self._prober.name!r}: {e}", exc_info=e
                )
            self._read_buffers_and_close()
            self._process = None

    def stop_forever(self):
        with self._lock:
            self.stop()
            self._stopped_forever = True
