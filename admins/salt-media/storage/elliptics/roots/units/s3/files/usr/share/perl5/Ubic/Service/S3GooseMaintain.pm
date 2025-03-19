package Ubic::Service::S3GooseMaintain;

use strict;
use warnings;

use JSON qw(decode_json);
use Params::Validate qw(:all);
use LWP::UserAgent;
use HTTP::Request::Common;
use LWP::Protocol::http::SocketUnixAlt;
use URI::URL;
use LWP::Protocol::http;
use LWP::Simple;

use Ubic::Result qw(result);
use Ubic::Service::Shared::Dirs;
use Ubic::Daemon qw(:all);

use parent qw(Ubic::Service::SimpleDaemon);
use vars qw($params);

sub new {
    my $class = shift;
    my $params = validate_with(
        params => \@_,
        spec   => {
            config => { type => SCALAR },
            timeout_options => { type => HASHREF },
            run_dir => { type => SCALAR, default => "/var/run/s3-goose", optional => 1 },
        },
        allow_extra => 1,
    );


    my $daemon_config = delete $params->{config};
    my $timeout_options = delete $params->{timeout_options};
    my $run_dir = delete $params->{run_dir};
    my $user = $params->{user} || "root";

    $params->{bin} = "/usr/bin/s3-goose-maintain -c $daemon_config";
    $params->{env} = { GODEBUG => "madvdontneed=1" };

    my $obj = $class->SUPER::new($params);

    $obj->{timeout_options} = $timeout_options;
    $obj->{run_dir} = $run_dir;
    $obj->{user} = $user;
    $obj->{auto_start} = 1;

    return bless $obj => $class;
}

sub status_impl {
    my ($self) = @_;
    my $ping_uri = "ping";
    if (my $daemon = check_daemon($self->pidfile)) {
        if (defined $params->{ping_uri})  {
            $ping_uri = "$params->{ping_uri}";
        }

    LWP::Protocol::implementor( http => "LWP::Protocol::http" );
    my $sresult = "";
    my $broke = 0;
    for my $attempt ( 1..50 ) {
        my $ping_url = URI::URL->new("http://localhost:3377/$ping_uri");
        my $ua = LWP::UserAgent->new;
        $ua->timeout(5);
        my $ping_response = $ua->get( $ping_url->abs );
        if ($ping_response->is_success){
            return result('running', "pid ".$daemon->pid);
            $broke = 0;
        }
        else {
            $broke = 1;
            $sresult = "ping response: " . $ping_response->message;
        }
    }
    if ($broke) {
        return result("broken", $sresult);
    }
    }
    else {
        return result('not running');
    }
}

sub timeout_options {
    my $self = shift;
    return $self->{timeout_options};
}

sub reload {
    my ( $self ) = @_;
    my $status = check_daemon($self->pidfile) or die result('not running');
    kill HUP => $status->pid;

    return 'reloaded';
}

sub start_impl {
    my ($self) = @_;

    Ubic::Service::Shared::Dirs::directory_checker( $self->{run_dir}, $self->{daemon_user} );

    $self->SUPER::start_impl();

    return "starting";
}

1;


=head1 NAME

Ubic::Service::S3Goose - run Go S3 Backend as Ubic service

=head1 SYNOPSIS

# /etc/ubic/service/s3-goose-maintain.json
  {
      "module": "Ubic::Service::S3GooseMaintain",
      "options": {
          "config": "/etc/s3-goose/s3-goose.conf",
          "timeout_options": {
              "start": {
                  "step": 1,
                  "trials": 3
              },
              "ping": 3
          },
          "user": "root",
          "daemon_user": "root",
          "ulimit": {
              "RLIMIT_NOFILE": 65535,
              "RLIMIT_CORE": -1
          }
      }
  }

=head1 AUTHOR

MDS team <mds-dev@yandex-team.ru>

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2017 by Yandex LLC.

This is free software; you can redistribute it and/or modify it under
the same terms as the Perl 5 programming language system itself.

=cut
