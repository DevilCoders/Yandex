package Ubic::Service::S3Goose;

use strict;
use warnings;

use JSON qw(decode_json);
use File::Find;
use Params::Validate qw(:all);
use HTTP::Request::Common;
use LWP::UserAgent;
use LWP::Protocol::http;
use LWP::Protocol::http::SocketUnixAlt;
use LWP::Simple;
use URI::URL;

use Ubic::Result qw(result);
use Ubic::Service::Shared::Dirs;
use Ubic::Daemon qw(:all);

use parent qw(Ubic::Service::SimpleDaemon);

sub new {
    my $class = shift;
    my $params = validate_with(
        params => \@_,
        spec   => {
            config          => { type => SCALAR },
            timeout_options => { type => HASHREF },
            run_dir         => { type => SCALAR, default => "/var/run/s3-goose", optional => 1 },
            check_addr      => { type => SCALAR, default => "http://localhost:3355/", optional => => 1 },
            check_uri       => { type => SCALAR, default => "/ping", optional => 1 },
            socket_perms    => { type => SCALAR, default => "0775", optional => 1 },
            socket_owners   => { type => SCALAR, default => "", optional => 1 },
        },
        allow_extra => 1,
    );


    my $daemon_config = delete $params->{config};
    my $timeout_options = delete $params->{timeout_options};
    my $run_dir = delete $params->{run_dir};
    my $user = $params->{user} || "root";

    my $check_addr = delete $params->{check_addr};
    my $check_uri = delete $params->{check_uri};

    my $socket_perms = oct(delete $params->{socket_perms});
    my $socket_owners = delete $params->{socket_owners};

    $params->{bin} = "/usr/bin/s3-goose-proxy -c $daemon_config";

    my $obj = $class->SUPER::new($params);

    $obj->{timeout_options} = $timeout_options;
    $obj->{run_dir} = $run_dir;
    $obj->{user} = $user;
    $obj->{auto_start} = 1;

    $obj->{check_addr} = $check_addr;
    $obj->{check_uri} = $check_uri;

    $obj->{socket_perms} = $socket_perms;
    $obj->{socket_owners} = $socket_owners;

    return bless $obj => $class;
}

sub status_impl {
    my ($self) = @_;

    my $daemon = check_daemon($self->pidfile);
    if (not defined($daemon) ) {
        return result('not running');
    }

    # Perform smart service status check after all common checks done
    my $check_attempts = 3;
    my $status = "";
    my $status_message = "";
    for my $attempt ( 1..$check_attempts ) {
        ($status, $status_message) = $self->get_service_status;
        if ($status eq 'running') {
            # Ensure other services will have access to the unix sockets of the daemon, if any
            find(sub { $self->change_unix_socket_perms }, $self->{run_dir});
            last;
        }
        sleep 1;
    }

    if ($status_message eq "") {
        $status_message = "pid ".$daemon->pid;
    }

    return result($status, $status_message);
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

    find(sub { $self->remove_unix_socket_files }, $self->{run_dir});
    $self->SUPER::start_impl();

    return "starting";
}

sub get_service_status {
    my ($self) = @_;

    my $check_addr = $self->{check_addr};
    my $check_uri = $self->{check_uri};
    my $check_timeout = 5;

    $check_addr =~ s{/+$}{}; # 'some-text/' -> 'some-text' (trim trailing slash)
    $check_uri =~ s{^/+}{}; # '/some/path -> 'some/path' (trim leading slashes)
    my $check_url = "$check_addr/$check_uri";

    if ((rindex $check_addr, "unix:", 0) == 0 or (rindex $check_addr, "/", 0) == 0) {
        # URL with 'unix:' or '/' prefix is a unix socket path
        $check_addr =~ s{^(unix:)?/*}{}; # unix:/<path-to-file> -> <path-to-file>

        # The LWP::Protocol::http::SocketUnixAlt expects quite strange structure of address:
        #   <proto>:<absolute unix socket path without leading slash>//<request URI>
        $check_url = "http:$check_addr//$check_uri";
        LWP::Protocol::implementor( http => "LWP::Protocol::http::SocketUnixAlt" );

    } else {
        # <check_url> should have regular <schema>://<addr>:<port>/<uri> structure here.
        LWP::Protocol::implementor( http => "LWP::Protocol::http" );
    }

    my $ua = LWP::UserAgent->new;
    $ua->timeout($check_timeout);
    my $ping_response = $ua->get( $check_url );

    if ($ping_response->is_success){
        return ("running", "")
    }

    return ("broken", "ping response: " . $ping_response->message);
}

sub remove_unix_socket_files() {
    my ($self) = @_;
    my $file_path = $File::Find::name;

    # Treat only unix sockets
    if (not -S $file_path) {
        return
    }

    unlink $file_path;
}

sub change_unix_socket_perms() {
    my ($self) = @_;
    my $file_path = $File::Find::name;

    # Treat only unix sockets
    if (not -S $file_path) {
        return
    }

    my $perms = $self->{socket_perms};
    # Make sure socket owner is a user set in '<daemon_user>' or '<user>' options
    my $daemon_user = $self->{daemon_user} || $self->{user};
    my ($uid, $gid) = ( getpwuid( getpwnam($daemon_user) ) )[2,3]; # get user ID and its primary group ID

    # When 'socket_owners' option is set, use <user>:<group> from this option instead of <daemon_user> or <user>
    my ($uname, $gname) = split(":", $self->{socket_owners}, 2);
    $uid = ($uname eq "") ? $uid : getpwnam($uname);
    $gid = ($gname eq "") ? $gid : getgrnam($gname);

    chown $uid, $gid, $file_path;
    chmod $perms, $file_path;
}

1;


=head1 NAME

Ubic::Service::S3Goose - run Go S3 Backend as Ubic service

=head1 SYNOPSIS

# /etc/ubic/service/s3-goose.json
  {
      "module": "Ubic::Service::S3Goose",
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
          "check_addr": "http://localhost:3355/",
          "check_uri": "/ping",
          "socket_perms": "0775",
          "socket_owners": ":www-data",
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
