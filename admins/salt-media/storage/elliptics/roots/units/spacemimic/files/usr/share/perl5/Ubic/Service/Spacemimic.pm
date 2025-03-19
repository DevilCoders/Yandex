package Ubic::Service::Spacemimic;

use strict;
use warnings;

use Ubic::Daemon qw(:all);
use Ubic::Result qw(result);
use Ubic::Service::Shared::Dirs;
use parent qw(Ubic::Service::SimpleDaemon);
use Params::Validate qw(:all);
use LogGiver::common;
use FCGI::Client;
use IO::Socket::UNIX;
use URI::URL;
use LWP::Protocol::http;
use LWP::Simple;


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
        user => { type => SCALAR, default => "spacemimic", optional => 1 },
        log_dir => { type => SCALAR, default => "/var/log/elliptics/srw/spacemimic/", optional => 1 },
        ping_uri => { type => SCALAR, default => "320.agodin.E100582:79488821749962451537709153851", optional => 1 },
        rlimit_nofile => { type => SCALAR,
                           regex => qr/^\d+$/,
                           optional => 1,
                        },
        rlimit_core => {   type => SCALAR,
                           regex => qr/^\-?\d+$/,
                           optional => 1 },
        rlimit_stack => {  type => SCALAR,
                           regex => qr/^\d+$/,
                           optional => 1  },
    });

    #
    # check ulimits
    #
    my $ulimits;
    if (defined $params->{rlimit_nofile}) { $ulimits->{"RLIMIT_NOFILE"} = $params->{rlimit_nofile} };
    if (defined $params->{rlimit_core})   { $ulimits->{"RLIMIT_CORE"} = $params->{rlimit_core} };
    if (defined $params->{rlimit_stack})  { $ulimits->{"RLIMIT_STACK"} = $params->{rlimit_stack} };

    my $bin = [
        '/usr/bin/spacemimic --config=/etc/elliptics/srw//mimic.conf --port=11000 --workers=5'
    ];

    my $daemon_user = $params->{user};
    return $class->SUPER::new({
        bin => $bin,
        user => 'root',
        ulimit => $ulimits || {},
        daemon_user => $daemon_user,
    ubic_log => $params->{log_dir}.'/ubic-spacemimic.log',
    stdout => $params->{log_dir}.'/stdout-spacemimic.log',
    stderr => $params->{log_dir}.'/stderr-spacemimic.log',
    auto_start => 1,
    });
}

sub start_impl {
    my $self = shift;

    if ( ! -d $params->{log_dir}) {
    Ubic::Service::Shared::Dirs::directory_checker( $params->{log_dir}, $params->{user});
    Ubic::Service::Shared::Dirs::directory_checker( $params->{run_dir}, $params->{user});
    }

    $self->SUPER::start_impl(@_);
}

sub status_impl {
    my ($self) = @_;
    my $ping_uri = "320.agodin.E100582:79488821749962451537709153851";
    LWP::Protocol::implementor( http => "LWP::Protocol::http" );
    if (my $daemon = check_daemon($self->pidfile)) {
        if (defined $params->{ping_uri})  {
            $ping_uri = "$params->{ping_uri}";
        }
        my $ping_url = URI::URL->new("http://localhost:11000/get/$ping_uri");
        my $ua = LWP::UserAgent->new;
        $ua->timeout(30);
        my $ping_response = $ua->get( $ping_url->abs );
        if ($ping_response->is_success){
            return result('running', "pid ".$daemon->pid);
        }
        else{
            return result("broken", "ping response: " . $ping_response->message);
        }
    }
    else {
        return result('not running');
    }
}

sub timeout_options {
    my $self = shift;
    { start => { step => 1, trials => 3 } };
}

sub reload {
    my ( $self ) = @_;
    my $status = check_daemon($self->pidfile) or die result('not running');
    kill HUP => $status->pid;

    return 'reloaded';
}

1;
