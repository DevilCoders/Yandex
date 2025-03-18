#!/usr/bin/perl

use warnings;
use strict;

use LWP::Simple;

# SETTINGS
########################################
our $TEMPDIR = './var';
our $CLUSTERMASTER_DIR = '..';

our $COMMON_TIMEOUT = 30;

# you probably don't need to change these
our $HOSTNAME = `hostname -s`; chomp($HOSTNAME);
our $CONFIG = "$TEMPDIR/config";

our $CONFDUMP = "$CLUSTERMASTER_DIR/confdump/confdump";
our $MASTER = "$CLUSTERMASTER_DIR/master/master";
our $WORKER = "$CLUSTERMASTER_DIR/worker/worker";
our $REMOTE = "$CLUSTERMASTER_DIR/remote/cmremote";

my $NWORKERS = 5;
my $NCLUSTERS = 20;
my $PARALLEL_CLUSTERS = 3;
my $PARALLEL_CLUSTERS2 = 2;
my $FIRSTWORKERPORT = 11000;
my $FIRSTWORKERHTTPPORT = 12000;

my $MASTERHTTPPORT = 9999;

my @common_config = (
    "GLOB = $HOSTNAME:$FIRSTWORKERPORT",
    "GLCL = $HOSTNAME:$FIRSTWORKERPORT 000..".sprintf("%03d", $NCLUSTERS-1),
    "SEGM = $HOSTNAME:".($FIRSTWORKERPORT+1)."..$HOSTNAME:".($FIRSTWORKERPORT+$NWORKERS-1)."",
    "CLST = $HOSTNAME:".($FIRSTWORKERPORT+1)."..$HOSTNAME:".($FIRSTWORKERPORT+$NWORKERS-1)." 000..".sprintf("%03d", $NCLUSTERS-1),

    "GLOB_LIM = $HOSTNAME:$FIRSTWORKERPORT cpu=1/$PARALLEL_CLUSTERS",
    "GLCL_LIM = $HOSTNAME:$FIRSTWORKERPORT 000..".sprintf("%03d", $NCLUSTERS-1)." cpu=1/$PARALLEL_CLUSTERS",
    "SEGM_LIM = $HOSTNAME:".($FIRSTWORKERPORT+1)."..$HOSTNAME:".($FIRSTWORKERPORT+$NWORKERS-1)." cpu=1/$PARALLEL_CLUSTERS",
    "CLST_LIM = $HOSTNAME:".($FIRSTWORKERPORT+1)."..$HOSTNAME:".($FIRSTWORKERPORT+$NWORKERS-1)." 000..".sprintf("%03d", $NCLUSTERS-1)." cpu=1/$PARALLEL_CLUSTERS",
);

my @functional_tests = (
);

# GLOBAL VARS
########################################
our $tests = 0;
our $expected_failure = 0;
our $unexpected_failure = 0;
our $expected_success = 0;
our $unexpected_success = 0;

our $workers_running = 0;
our $master_running = 0;

# FUNCTIONS
########################################

sub prepare_config {
    my %args = @_;

    open(CONFIG, '>', $CONFIG) || die "Cannot write config";
    print CONFIG "#!/bin/sh\n";
    print CONFIG "_scenario(){\n";

    if (defined $args{lines}) {
        foreach my $line (@{$args{lines}}) {
            print CONFIG "$line\n";
        }
    }

    print CONFIG "}\n";

    if (defined $args{'sleep'}) {
        print CONFIG "sleep $args{sleep}\n";
    }

    if (defined $args{fail}) {
        print CONFIG "if [ \"\$1\" = \"$args{fail}\" ]; then\n";
        print CONFIG "  exit 1\n";
        print CONFIG "else\n";
        print CONFIG "  exit 0\n";
        print CONFIG "fi\n";
    } else {
        print CONFIG "exit 0\n";
    }

    close(CONFIG);
}

sub run_master {
    # TODO: display stderr
    system_or_die("$MASTER -v $TEMPDIR/master.var -P $TEMPDIR/master.pid -h $MASTERHTTPPORT -s $CONFIG ".join(' ', @_), "failed to run master", "$TEMPDIR/master.stderr");

    $master_running = 1;
}

sub run_workers {
    foreach my $w (0..$NWORKERS-1) {
        my $port = $FIRSTWORKERPORT + $w;
        my $httpport = $FIRSTWORKERHTTPPORT + $w;
        system_or_die("$WORKER -v $TEMPDIR/worker$w.var -P $TEMPDIR/worker$w.pid -w $port -h $httpport ".join(' ', @_), "failed to run worker $w", "$TEMPDIR/worker$w.stderr");
    }

    $workers_running = 1;
}

sub kill_master {
    open(PIDFILE, "$TEMPDIR/master.pid") || return 0;
    my $pid = <PIDFILE>;
    close(PIDFILE);

    return 0 unless (kill(15, $pid) == 1);

    my $i = 0;
    waitfor($COMMON_TIMEOUT, sub { !-e "$TEMPDIR/master.pid" });

    $master_running = 0;
    return 1;
}

sub kill_workers {
    my $kills = 0;
    foreach my $w (0..$NWORKERS-1) {
        open(PIDFILE, "$TEMPDIR/worker$w.pid") || next;
        my $pid = <PIDFILE>;
        close(PIDFILE);

        $kills++ if (kill(15, $pid) == 1);
    }

    return 0 unless ($kills == $NWORKERS);

    foreach my $w (0..$NWORKERS-1) {
        waitfor($COMMON_TIMEOUT, sub { !-e "$TEMPDIR/worker$w.pid" });
    }

    $workers_running = 0;
    return 1;
}

sub run_functional_test {
    my ($name, $expected_result, $code) = @_;

    cleanup();

    eval {
        &$code();
    };
    my $error;
    if ($@) {
        $error = $@;
        chomp($error);

    }

    die "Cannot kill master, meaningless to continue; ps advised" if ($master_running && !kill_master());
    die "Cannot kill workers, meaningless to continue; ps adviced" if ($workers_running && !kill_workers());

    return register_test($name, !defined $error, $expected_result, $error);
}

sub register_test {
    my ($name, $result, $expected_result, $message) = @_;

    $result = !!$result;
    $expected_result = !!$expected_result;

    my $status;
    my $fail = 0;

    if ($result == $expected_result) {
        if ($result) {
            $expected_success++;
            $status = "expected success";
        } else {
            $expected_failure++;
            $status = "expected failure";
        }
    } else {
        $fail = 1;
        if ($result) {
            $unexpected_success++;
            $status = "unexpected success";
        } else {
            $unexpected_failure++;
            $status = "unexpected failure";
        }
    }

    printf("%sTest %3d: %s: %s%s\n", $fail ? '[FAILED] ' : '', ++$tests, $name, $status, (defined $message) ? " ($message)" : '');

    return !$fail;
}

sub cleanup {
    system("rm -rf $TEMPDIR");
    mkdir("$TEMPDIR");
}

sub system_or_die {
    my ($command, $message, $logfile) = @_;

    if (0 != system("$command >$logfile 2>&1")) {
        my $lastline = '<no output>';

        open(LOG, $logfile);
        while(<LOG>) {
            # get last line
            $lastline = $_;
        }
        close(LOG);

        chown $lastline;

        die "$message: $lastline";
    }
}

sub waitfor {
    my ($timeout, $code) = @_;

    my $i = 0;
    while (1) {
       return if &$code();

       die "timeout expired" if (++$i > $timeout);

       sleep(1);
    }
}

# PROCESS
########################################
print "===> Preparing\n";
cleanup();

print "===> Functional tests\n";
run_functional_test("Master starts and exists", 1, sub {
        prepare_config();
        run_master();
        sleep(1); # give master time to daemon()
        die "Cannot kill master" unless kill_master();
        die "Pidfile leftovers" if -e "$TEMPDIR/masted.pid";
    });

unless (run_functional_test("Pidfiles should not allow simultaneous runs", 0, sub {
        prepare_config();
        run_master();
        run_master();
        sleep(1);
    })) { die "pidfiles don't work - cannot reliably kill processes, cannot continue; please fix this ASAP, and to run this test again please kill runaway master processes"; }

run_functional_test("Basic functionality", 1, sub {
        prepare_config(lines => [ @common_config, 'GLOB lonely' ]);
        run_master();
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require lonely unknown", "no unknown status on run", "$TEMPDIR/remote.stderr");
        run_workers();
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require lonely idle", "workers not connectin/no idle status", "$TEMPDIR/remote.stderr");

        die "workers page doesn't work as expected" unless (get("http://$HOSTNAME:$MASTERHTTPPORT/workers") =~ /$HOSTNAME.*Active.*idle/);
        die "targets page doesn't work as expected" unless (get("http://$HOSTNAME:$MASTERHTTPPORT/targets") =~ /GLOB.*lonely.*idle/);
        # Replace following uncommented strig with this <- add worker status to target page
        #die "target page doesn't work as expected" unless (get("http://$HOSTNAME:$MASTERHTTPPORT/target/lonely") =~ /$HOSTNAME.*Active.*idle/);
        die "target page doesn't work as expected" unless (get("http://$HOSTNAME:$MASTERHTTPPORT/target/lonely") =~ /$HOSTNAME.*idle/);
        die "worker page doesn't work as expected" unless (get("http://$HOSTNAME:$MASTERHTTPPORT/worker/$HOSTNAME:$FIRSTWORKERPORT") =~ /GLOB.*lonely.*idle/);
    });

run_functional_test("Running targets", 1, sub {
        prepare_config(lines => [ @common_config, 'GLOB one', 'SEGM two', 'GLOB three' ]);
        run_workers();
        run_master();
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require one idle", "no idle status after start", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT run three require three success", "running path failed", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT invalidate one require three idle", "cannot invalidate chain", "$TEMPDIR/remote.stderr");
    });

run_functional_test("Failing targets", 1, sub {
        prepare_config(lines => [ @common_config, 'GLOB one', 'SEGM two', 'GLOB three' ], fail => 'two');
        run_workers();
        run_master();
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require one idle", "no idle status after start", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT run three require two failed", "expected target haven't failed", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require three depfailed", "expected target haven't depfailed", "$TEMPDIR/remote.stderr");
    });

run_functional_test("State persistence", 1, sub {
        prepare_config(lines => [ @common_config, 'GLOB one', 'SEGM two', 'GLOB three' ]);
        run_workers();
        run_master();

        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require one idle", "no idle status after start", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT run three require three success", "running path failed", "$TEMPDIR/remote.stderr");

        kill_master();
        sleep(1);
        run_master();
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require one success require two success require three success", "status not restored after master restart", "$TEMPDIR/remote.stderr");

        kill_workers();
        sleep(1);
        run_workers();
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require one success require two success require three success", "status not restored after workers restart", "$TEMPDIR/remote.stderr");

        kill_master();
        kill_workers();
        sleep(1);
        run_master();
        run_workers();
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require one success require two success require three success", "status not restored after whole system restart", "$TEMPDIR/remote.stderr");
    });

run_functional_test("Type-defined limits", 1, sub {
        prepare_config(lines => [ @common_config, 'GLCL_LIM lonely' ], sleep => 1);
        run_workers();
        run_master();

        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require lonely idle", "no idle status after start", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT run lonely", "running target failed", "$TEMPDIR/remote.stderr");

        waitfor($COMMON_TIMEOUT, sub { get("http://$HOSTNAME:$MASTERHTTPPORT/targets_xml") =~ /<pending>/ });

        my $max_tasks = 0;
        waitfor($COMMON_TIMEOUT+$NCLUSTERS/$PARALLEL_CLUSTERS, sub {
                my $targets_xml = get("http://$HOSTNAME:$MASTERHTTPPORT/targets_xml");
                if ($targets_xml =~ /<running>(\d+)<\/running>/) {
                    $max_tasks = $1 if ($1 > $max_tasks);
                    return 0;
                } else {
                    return 1;
                }
            });

        die "parallel execution ($max_tasks) has not hit the limit ($PARALLEL_CLUSTERS)" if ($max_tasks < $PARALLEL_CLUSTERS);
        die "parallel execution ($max_tasks) has exceeded the limit ($PARALLEL_CLUSTERS)" if ($max_tasks > $PARALLEL_CLUSTERS);

        system_or_die("$REMOTE -t0 -r1 $HOSTNAME:$MASTERHTTPPORT require lonely success", "running target failed", "$TEMPDIR/remote.stderr");
    });

run_functional_test("Target-defined limits", 1, sub {
        prepare_config(lines => [ @common_config, "GLCL lonely cpu=1/$PARALLEL_CLUSTERS" ], sleep => 1);
        run_workers();
        run_master();

        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require lonely idle", "no idle status after start", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT run lonely", "running target failed", "$TEMPDIR/remote.stderr");

        waitfor($COMMON_TIMEOUT, sub { get("http://$HOSTNAME:$MASTERHTTPPORT/targets_xml") =~ /<pending>/ });

        my $max_tasks = 0;
        waitfor($COMMON_TIMEOUT+$NCLUSTERS/$PARALLEL_CLUSTERS, sub {
                my $targets_xml = get("http://$HOSTNAME:$MASTERHTTPPORT/targets_xml");
                if ($targets_xml =~ /<running>(\d+)<\/running>/) {
                    $max_tasks = $1 if ($1 > $max_tasks);
                    return 0;
                } else {
                    return 1;
                }
            });

        die "parallel execution ($max_tasks) has not hit the limit ($PARALLEL_CLUSTERS)" if ($max_tasks < $PARALLEL_CLUSTERS);
        die "parallel execution ($max_tasks) has exceeded the limit ($PARALLEL_CLUSTERS)" if ($max_tasks > $PARALLEL_CLUSTERS);

        system_or_die("$REMOTE -t0 -r1 $HOSTNAME:$MASTERHTTPPORT require lonely success", "running target failed", "$TEMPDIR/remote.stderr");
    });

run_functional_test("Target-defined limits overriding target-defined", 1, sub {
        prepare_config(lines => [ @common_config, "GLCL_LIM lonely cpu=1/$PARALLEL_CLUSTERS2" ], sleep => 1);
        run_workers();
        run_master();

        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT require lonely idle", "no idle status after start", "$TEMPDIR/remote.stderr");
        system_or_die("$REMOTE -t$COMMON_TIMEOUT -r1 $HOSTNAME:$MASTERHTTPPORT run lonely", "running target failed", "$TEMPDIR/remote.stderr");

        waitfor($COMMON_TIMEOUT, sub { get("http://$HOSTNAME:$MASTERHTTPPORT/targets_xml") =~ /<pending>/ });

        my $max_tasks = 0;
        waitfor($COMMON_TIMEOUT+$NCLUSTERS/$PARALLEL_CLUSTERS2, sub {
                my $targets_xml = get("http://$HOSTNAME:$MASTERHTTPPORT/targets_xml");
                if ($targets_xml =~ /<running>(\d+)<\/running>/) {
                    $max_tasks = $1 if ($1 > $max_tasks);
                    return 0;
                } else {
                    return 1;
                }
            });

        die "parallel execution ($max_tasks) has not hit the limit ($PARALLEL_CLUSTERS2)" if ($max_tasks < $PARALLEL_CLUSTERS2);
        die "parallel execution ($max_tasks) has exceeded the limit ($PARALLEL_CLUSTERS2)" if ($max_tasks > $PARALLEL_CLUSTERS2);

        system_or_die("$REMOTE -t0 -r1 $HOSTNAME:$MASTERHTTPPORT require lonely success", "running target failed", "$TEMPDIR/remote.stderr");
    });

print "===> Results\n";
print "         Total tests: $tests\n";
print "   Expected failures: $expected_failure\n";
print "  Expected successes: $expected_success\n";
print " Unexpected failures: $unexpected_failure\n";
print "Unexpected successes: $unexpected_success\n";
print "             Overall: ".($unexpected_failure + $unexpected_success ? "failure" : "success")."\n";

exit($unexpected_failure + $unexpected_success);

#    rm -rf $VAR_DIR/cm-0.var
#    rm -rf $VAR_DIR/cm-1.var
#    rm -rf $VAR_DIR/cm-2.var
#    rm -rf $VAR_DIR/cm-3.var
#    rm -rf $VAR_DIR/cm-t0.var
#    rm -rf $VAR_DIR/cm-t1.var
#    rm -rf $VAR_DIR/cm-t2.var
#    rm -rf $VAR_DIR/cm-t3.var
##    rm -rf $VAR_DIR/cm-g1.var
#    rm -rf $VAR_DIR/cm-g2.var

#    # global workers
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-g1.var -P $VAR_DIR/cm-g1.pid -l $VAR_DIR/cm-g1.log -w 10001 -h 20001
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-g2.var -P $VAR_DIR/cm-g2.pid -l $VAR_DIR/cm-g2.log -w 10002 -h 20002

#    # normal workers
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-0.var -P $VAR_DIR/cm-0.pid -l $VAR_DIR/cm-0.log -w 11000 -h 21000
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-1.var -P $VAR_DIR/cm-1.pid -l $VAR_DIR/cm-1.log -w 11001 -h 21001
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-2.var -P $VAR_DIR/cm-2.pid -l $VAR_DIR/cm-2.log -w 11002 -h 21002
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-3.var -P $VAR_DIR/cm-3.pid -l $VAR_DIR/cm-3.log -w 11003 -h 21003

    # test workers
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-t0.var -P $VAR_DIR/cm-t0.pid -l $VAR_DIR/cm-t0.log -w 11010 -h 21010
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-t1.var -P $VAR_DIR/cm-t1.pid -l $VAR_DIR/cm-t1.log -w 11011 -h 21011
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-t2.var -P $VAR_DIR/cm-t2.pid -l $VAR_DIR/cm-t2.log -w 11012 -h 21012
#    $CM_DIR/worker/worker -v $VAR_DIR/cm-t3.var -P $VAR_DIR/cm-t3.pid -l $VAR_DIR/cm-t3.log -w 11013 -h 21013

#    # master
#    $CM_DIR/master/master -v $VAR_DIR/cm-m.var -P $VAR_DIR/cm-m.pid -l $VAR_DIR/cm-m.log -s "$2" -c clm-host.cfg
#    ;;
#kill)
#    killall master
#    killall worker
#    ;;
#*)
#    echo "Usage: $0 deploy config"
#    echo "       $0 kill"
#    exit 1
#esac
