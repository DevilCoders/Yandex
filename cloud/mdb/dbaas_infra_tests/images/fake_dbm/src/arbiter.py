"""
Monitor for changes in state-file.
There is also a file where tracking occurs.
File is used as a communication bus, and contains a json string, one per line.
Each line must have uuid and command. Command may have arguments.
"""
import json
import logging
import os
import threading
import time
import traceback
from uuid import uuid4

CMD_FIELDS = {
    'request': ('uuid', 'command', 'args'),
    'status': ('uuid', 'command', 'result', 'code'),
}


def _serialize_msg(*, entry_type, uuid=None, **kwargs):
    """
    Form a valid message
    """
    assert entry_type in CMD_FIELDS, \
        'unknown entry type: {0}'.format(entry_type)

    result = {}
    if uuid is None:
        uuid = str(uuid4())
    args = kwargs.copy()
    args['uuid'] = uuid
    try:
        for key in CMD_FIELDS[entry_type]:
            result[key] = args[key]
    except KeyError as key:
        raise ValueError('missing required argument: {0}'.format(key))
    return json.dumps(result)


def _parse_msg(raw_data, entry_type='request'):
    """
    Parse individual entries of command message or progress log.
    """
    assert entry_type in CMD_FIELDS, \
        'unknown entry type: {0}'.format(entry_type)

    parsed = {}
    # Parse and validate JSON string
    try:
        data = json.loads(raw_data)
        for key in CMD_FIELDS[entry_type]:
            parsed[key] = data[key]

    except (json.JSONDecodeError, ValueError):
        raise ValueError('unable to decode message "{0}"'.format(raw_data))
    except KeyError as key:
        raise ValueError('required key missing: {0}'.format(key))
    return parsed


def _read_already_processed(status_file):
    """
    Read already processed commands so that the daemon would not
    repeat itself.
    """
    entries = {}
    if not os.path.exists(status_file):
        return entries
    with open(status_file) as processed_stream:
        for entry in processed_stream:
            try:
                entry = _parse_msg(entry, entry_type='status')
                entries.update({entry['uuid']: entry})
            except Exception:
                # Called on start, so ignore any errors
                pass
    return entries


# pylint: disable=too-few-public-methods
class ArbiterClient:
    """
    Implements client part of file-based two-way communication
    """

    def __init__(self, status_file, command_file):
        self._status_file = status_file
        self._command_file = command_file

    def run(self, command, timeout=300, **kwargs):
        """
        Entry point.
        Listen command and status files, process commands.
        """
        uuid = str(uuid4())
        # Prepare cmd
        cmd = _serialize_msg(
            entry_type='request',
            command=command,
            uuid=uuid,
            args=kwargs,
        )
        # Write to command file
        code, result = self._exec_listen(
            entry=cmd,
            uuid=uuid,
            timeout=timeout,
        )
        if code == 0:
            return result
        raise RuntimeError(result)

    def _exec_listen(self, entry, uuid, timeout):
        """
        Put command into queue and listen for a result
        in status file
        """
        deadline = time.time() + timeout
        with open(self._command_file, 'a') as cmd_stream:
            cmd_stream.write(entry + '\n')
        with open(self._status_file) as status_stream:
            status_stream.seek(0, 2)  # set to end of file
            while time.time() <= deadline:
                try:
                    cmd_result = _parse_msg(
                        status_stream.readline(),
                        entry_type='status',
                    )
                    if cmd_result['uuid'] == uuid:
                        return cmd_result['code'], cmd_result['result']
                except ValueError:
                    pass
                time.sleep(0.1)
        raise RuntimeError('command timed out: {0}'.format(entry))


class ArbiterServer(threading.Thread):
    """
    Implements server part of file-based two-way communication
    """

    def __init__(self, status_file, command_file, dispatcher):
        super().__init__(name='arbiter')

        self.daemon = True
        self.should_run = True

        self._dispatcher = dispatcher
        self._status_file = status_file
        self._command_file = command_file
        self._processed = _read_already_processed(self._status_file)

    def stop(self):
        """
        Graceful exit
        """
        self.should_run = False

    def _handle(self, cmd):
        """
        Prepares command name and arguments and hands them over to
        dispatcher.
        """
        # Check if we have already processed that command
        if self._processed.get(cmd['uuid']) is not None:
            return None

        return self._dispatcher.run_command(command=cmd['command'], args=cmd.get('args', {}))

    def run(self):
        """
        Opens command_file for reading, status for append.
        Polls command file each second and processes tasks.
        """
        # Create command file. Progress log will be created below.
        if not os.path.exists(self._command_file):
            open(self._command_file, 'w').close()
        # Open both files
        with open(self._command_file) as cmd_stream, \
                open(self._status_file, 'a') as status_stream:
            while self.should_run:
                cmd = {'command': None}
                result = None
                # Non-zero return code will cause RuntimeException
                # on the receiving end.
                code = 0
                try:
                    # file.readline() also moves file offset forward.
                    raw = cmd_stream.readline()
                    # Empty string
                    if not raw.strip():
                        time.sleep(0.1)
                        continue
                    # Try parsing command. Also validates its structure.
                    cmd = _parse_msg(
                        raw,
                        entry_type='request',
                    )
                    result = self._handle(cmd)
                except ValueError as err:
                    logging.error(
                        'error while processing, cmd=%s, offset=%s, data=%s',
                        err,
                        cmd_stream.tell(),
                        raw,
                    )
                    time.sleep(0.1)
                    continue
                except Exception:
                    # Pass the error thru
                    result = traceback.format_exc()
                    code = 1
                # Save results
                status = dict(result=result, code=code, **cmd)
                # Remember processed command
                self._processed[cmd.get('uuid')] = status
                # Write status file
                status_stream.write(_serialize_msg(
                    entry_type='status',
                    **status,
                ) + '\n')
                status_stream.flush()
