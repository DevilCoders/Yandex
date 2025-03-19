package Ubic::Service::Karl;

use strict;
use warnings;

use Ubic::Daemon qw(:all);
use Ubic::Result qw(result);
use Ubic::Service::Shared::Dirs;
use parent qw(Ubic::Service::SimpleDaemon);

use Params::Validate qw(:all);
use JSON qw( decode_json from_json );
use LWP::UserAgent;
use HTTP::Request::Common;
use LWP::Protocol::http::SocketUnixAlt;

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
        user => { type => SCALAR, default => "karl", optional => 1 },
        check_timeout => { type => SCALAR, default => 30, optional => 1 },
        log_dir => { type => SCALAR, default => "/var/log/karl", optional => 1 },
        conf_file => { type => SCALAR, default => "/etc/karl/karl.yaml", optional => 1 },
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
    });

    my $ulimits;
    if (defined $params->{rlimit_nofile}) { $ulimits->{"RLIMIT_NOFILE"} = $params->{rlimit_nofile} };
    if (defined $params->{rlimit_core})   { $ulimits->{"RLIMIT_CORE"} = $params->{rlimit_core} };
    if (defined $params->{rlimit_stack})  { $ulimits->{"RLIMIT_STACK"} = $params->{rlimit_stack} };

    my $bin = [
        '/usr/bin/karl -c '.$params->{conf_file}
    ];

    my $daemon_user = $params->{user};
    return $class->SUPER::new({
        bin => $bin,
        user => 'root',
        ulimit => $ulimits || {},
        daemon_user => $daemon_user,
        ubic_log => $params->{log_dir}.'/ubic.log',
        stdout => $params->{log_dir}.'/stdout.log',
        stderr => $params->{log_dir}.'/stderr.log',
        auto_start => 1,
    });
}

sub start_impl {
    my $self = shift;

    Ubic::Service::Shared::Dirs::directory_checker( $params->{log_dir}, $params->{user});

    $self->SUPER::start_impl(@_);
}


sub status_impl {
    my $self = shift;
    my $status = $self->SUPER::status_impl();

    return $status if $status->status ne 'running';
    my $daemon = check_daemon($self->pidfile);

    # my $cmd = "";
    # $cmd = "karl-cli ping --tls 2>/dev/null";

    # my $broke = 0;
    # my $sresult = "";
    # for my $attempt ( 1..10 ) {

    #     my $pid = fork;
    #     unless ($pid) {
    #         alarm($params->{check_timeout});
    #         exec($cmd. " &>/dev/null");
    #     }
    #     my $x = waitpid($pid, 0);
    #     if ($? ne 0) {
    #         my $errcode = $? >> 8;
    #         $broke = 1;
    #         $sresult = "error code $errcode from ext-check\ncheck cmd: $cmd ; echo \$?";
    #     } else {
    #         return 'running';
    #     }
    # }
    # if ($broke) {
    #     return result("broken", $sresult);
    # }

    return result('running', "pid ".$daemon->pid);
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

1;
