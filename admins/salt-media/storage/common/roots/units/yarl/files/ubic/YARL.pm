package Ubic::Service::YARL;

use strict;
use warnings;

use Ubic::Daemon qw(:all);
use Ubic::Result qw(result);
use Ubic::Service::Shared::Dirs;
use parent qw(Ubic::Service::SimpleDaemon);

use Params::Validate qw(:all);
use File::Basename;
use JSON qw( decode_json from_json );
use LWP::UserAgent;
use LWP::Protocol::http;
use HTTP::Request::Common;
use URI::URL;

my %opt2arg = ();
for my $arg (qw(configuration server:announce server:announce-interval))
{
    my $opt = $arg;
    $opt2arg{$opt} = $arg;
}

use vars qw($params);

sub new {
    my $class = shift;

    $params = validate(@_, {
        user => { type => SCALAR, default => "root", optional => 1 },
        config => { type => SCALAR },
        rlimit_nofile => { type => SCALAR,
                           regex => qr/^\d+$/,
                           optional => 1,
                        },
        rlimit_core => {   type => SCALAR,
                           regex => qr/^(\-?\d+|unlimited)$/,
                           optional => 1 },
        rlimit_stack => {  type => SCALAR,
                           regex => qr/^\d+$/,
                           optional => 1  },
        timeout => { type => SCALAR },
        start_attempts => { type => SCALAR },
        ping_attempts => { type => SCALAR },
        check_timeout => { type => SCALAR, default => 60, optional => 1 },
        stop_timeout => { type => SCALAR, default => 300, optional => 1 },
    });

    my $ulimits;
    if (defined $params->{rlimit_nofile}) { $ulimits->{"RLIMIT_NOFILE"} = $params->{rlimit_nofile} };
    if (defined $params->{rlimit_core})   { $ulimits->{"RLIMIT_CORE"} = $params->{rlimit_core} };
    if (defined $params->{rlimit_stack})  { $ulimits->{"RLIMIT_STACK"} = $params->{rlimit_stack} };

    $params->{log_dir} = '/var/log/yarl/';

    my $bin = [
        '/usr/bin/yarl --config ' . $params->{config}
    ];

    my $daemon_user = $params->{user};
    return $class->SUPER::new({
        bin => $bin,
        user => 'root',
        env => { GOTRACEBACK => "crash" },
        ulimit => $ulimits || {},
        daemon_user => $daemon_user,
        ubic_log => $params->{log_dir}.'/ubic.log',
        stdout => $params->{log_dir}.'/ubic-stdout.log',
        stderr => $params->{log_dir}.'/ubic-stderr.log',
        auto_start => 1,
        stop_timeout => $params->{stop_timeout},
    });
}

sub start_impl {
    my $self = shift;

    Ubic::Service::Shared::Dirs::directory_checker( $params->{log_dir}, $params->{user});

    $self->SUPER::start_impl(@_);

    for my $attempt ( 1..$params->{start_attempts} ) {
        my $status = get_status($self, 1);
        if ($status->status eq 'running') {
            return $status;
        }
        sleep 4;
    }
    return result('not running');
}

sub get_status {
    my $self = $_[0];
    my $attempts = $_[1];

    my $cmd = "yarl-cli monitor communication >/dev/null 2>&1";
    my $sresult = "";

    for my $attempt ( 1..$attempts ) {
        my $daemon = check_daemon($self->pidfile);
        unless ($daemon) {
            sleep 1;
            return result('not running');
        }
        my $pid = fork;
        unless ($pid) {
            alarm($params->{timeout});
            exec($cmd);
        }
        my $x = waitpid($pid, 0);
        if ($? ne 0) {
            my $errcode = $? >> 8;
            $sresult = "error code $errcode from ext-check\ncheck cmd:\n$cmd ; echo \$?";
        } else {
            return result('running', "pid ".$daemon->pid);
        }
    }

    return result("broken", $sresult);
}

sub status_impl {
    my $self = shift;
    my $status = $self->SUPER::status_impl();

    return get_status($self, $params->{ping_attempts});
}

sub timeout_options {
    my $self = shift;
    { start => { step => 1, trials => 10 } };
}

sub reload {
    my ( $self ) = @_;
    my $status = check_daemon($self->pidfile) or die result('not running');
    kill HUP => $status->pid;

    return 'reloaded';
}

sub check_timeout {
    my $self = shift;
    return $params->{check_timeout};
}

1;
