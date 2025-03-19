package Ubic::Service::SimpleDaemonTorrents;
{
  $Ubic::Service::SimpleDaemonTorrents::VERSION = '1.61';
}

use strict;
use warnings;

# ABSTRACT: service module for daemonizing any binary


use Ubic::Daemon qw(start_daemon stop_daemon check_daemon);
use Ubic::Result qw(result);
use Ubic::Settings;
use URI::URL;
use LWP::Simple;
use Params::Validate qw(:all);
use parent qw(Ubic::Service::SimpleDaemon);

sub status_impl {
    my ($self) = @_;
    if (my $daemon = check_daemon($self->pidfile)) {

       	my $ping_url = URI::URL->new("http://localhost:10050/ping");
 	my $ua = LWP::UserAgent->new;
        $ua->timeout(40);
       	#sleep(5);
	my $ping_response;

	eval{
		local $SIG{ALRM} = sub { die "/ping check took too long\n" };

		alarm (45);
		$ping_response = $ua->get( $ping_url->abs );
		alarm (0);
	};
	if ($@) {
		print "smth going wrong while doing active check: ".$@;
	} else { # no timeout during check

		if (defined $ping_response && $ping_response->is_success){
	    		return result('running', "pid ".$daemon->pid);
		};
	}

        return result("broken", "/ping failed");
    }
    else {
        return result('not running');
    }
}

