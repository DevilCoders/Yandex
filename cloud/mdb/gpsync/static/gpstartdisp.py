#!/usr/bin/env python

import imp
import os
import sys

try:
    from gppylib.mainUtils import simple_main
    from gppylib.gparray import Segment
    from gppylib.commands import gp
    from gppylib.commands import pg

    sys.dont_write_bytecode = True
    gpstart_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gpstart')
    with open(gpstart_path, 'r') as gpstart_file:
        imp.load_module('gpstart', gpstart_file, gpstart_path, ('', 'U', 1))
    from gpstart import GpStart, logger
except ImportError as e:
    sys.exit('Cannot import modules.  Please check that you have sourced greenplum_path.sh.  Detail: ' + str(e))


GP_RECOVERY_CONF_FILE = 'recovery.conf'


class GpStartDisp(GpStart):
    def run(self):
        self._prepare()
        return self._start_local_standby()

    @staticmethod
    def is_configured_standby(options):
        master_datadir = options.masterDataDirectory
        if not master_datadir:
            master_datadir = gp.get_masterdatadir()
        return os.path.exists(os.path.join(master_datadir, GP_RECOVERY_CONF_FILE))

    def _start_local_standby(self):
        """Simplified GpStart._start_final_master()

        Only starts local standby with same arguments that we would start real master
        """
        restrict_txt = ""
        if self.restricted:
            restrict_txt = "in RESTRICTED mode"

        logger.info("Starting Standby instance directory %s %s" % (self.master_datadir, restrict_txt))

        # attempt to start master
        gp.MasterStart.local(
            "Starting Standby instance",
            self.master_datadir,
            self.port,
            self.era,
            wrapper=self.wrapper,
            wrapper_args=self.wrapper_args,
            specialMode=self.specialMode,
            restrictedMode=self.restricted,
            timeout=self.timeout,
            max_connections=self.max_connections,
        )

        master_segment = Segment(
            content=None,
            preferred_role=None,
            dbid=None,
            role=None,
            mode=None,
            status=None,
            hostname=None,
            address=None,
            port=self.port,
            datadir=self.master_datadir,
        )

        # check that master is running now
        if not pg.DbStatus.local('master instance', master_segment):
            logger.warning(
                "Command pg_ctl reports Master %s on port %d not running"
                % (master_segment.datadir, master_segment.port)
            )
            logger.warning("Master could not be started")
            return 1

        logger.info("Command pg_ctl reports Master %s instance active" % master_segment.hostname)

        return 0

    @staticmethod
    def createProgram(options, args):
        logfileDirectory = options.ensure_value("logfileDirectory", False)

        if GpStartDisp.is_configured_standby(options):
            return GpStartDisp(
                options.specialMode,
                options.restricted,
                options.start_standby,
                master_datadir=options.masterDataDirectory,
                parallel=options.parallel,
                quiet=options.quiet,
                masteronly=options.master_only,
                interactive=options.interactive,
                timeout=options.timeout,
                wrapper=options.wrapper,
                wrapper_args=options.wrapper_args,
                skip_standby_check=options.skip_standby_check,
                logfileDirectory=logfileDirectory,
                skip_heap_checksum_validation=options.skip_heap_checksum_validation,
            )
        return GpStart.createProgram(options, args)


if __name__ == '__main__':
    simple_main(GpStartDisp.createParser, GpStartDisp.createProgram)
