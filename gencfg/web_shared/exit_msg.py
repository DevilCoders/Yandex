import errno
import os


class ExitCodes(object):
    NORMAL = 0
    SPECIAL_CODES_START = 240
    DIE = SPECIAL_CODES_START + 1
    UPDATE_ALL = SPECIAL_CODES_START + 2
    UNKNOWN = -1


_EXIT_MSG_FILE_DIR = "web_shared"
_EXIT_MSG_FILE = os.path.join(_EXIT_MSG_FILE_DIR, ".exit_status")


class ExitMessageFile(object):
    @staticmethod
    def reset():
        ExitMessageFile.set_code(ExitCodes.UNKNOWN)

    @staticmethod
    def set_code(exit_code):
        try:
            os.mkdir(_EXIT_MSG_FILE_DIR)
        except EnvironmentError as e:
            if e.errno != errno.EEXIST:
                raise

        # Web-main application is actually running covered with Flask Engine process
        # Therefore caller application cannot receive exit code
        # So we use file for this purpose
        with open(_EXIT_MSG_FILE, 'w') as f:
            print >> f, exit_code
            f.close()

    @staticmethod
    def get_code():
        try:
            return int(open(_EXIT_MSG_FILE).read().split('\n')[0])
        except:
            return ExitCodes.UNKNOWN
