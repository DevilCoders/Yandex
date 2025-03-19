package Ubic::Service::S3MdsProxyArc;

use strict;
use warnings;

use Net::INET6Glue;
use JSON qw(decode_json);
use Params::Validate qw(:all);
use URI::URL;
use LWP::Simple;
use LWP::Protocol::http;

use Ubic::Service::Shared::Dirs;
use Ubic::Result qw(result);

use parent qw(Ubic::Service::SimpleDaemon);


sub new {
    my $class = shift;
    my $params = validate_with(
        params => \@_,
        spec   => {
            config => { type => SCALAR },
            timeout_options => { type => HASHREF },
        },
        allow_extra => 1,
    );

    my $config = delete $params->{config};
    my $timeout_options = delete $params->{timeout_options};

    $params->{bin} = "/usr/bin/s3-mds-proxy-arc --config $config";

    my $obj = $class->SUPER::new($params);

    # Pass some useful params
    $obj->{config} = $config;
    $obj->{timeout_options} = $timeout_options;

    return bless $obj => $class;
}

sub _config {
    my ($self) = @_;

    my $config;
    {
        local $/;
        open(my $fh, "<", $self->{config})
            or die "cannot open < $self->{config}: $!";
        my $json = <$fh>;
        $config = decode_json($json);
        close $fh;
    }

    return $config;
}

sub status_impl {
    my ($self) = @_;

    # NOTE: If the application daemon isn't running return its status
    my $status = $self->SUPER::status_impl();
    return $status if $status->status ne "running";

    my $config = $self->_config();
    my $system_endpoints = $config->{application}{system_endpoints};
    my $workers_num = $config->{application}{workers_num} || 1;

    my $ping_timeout = $self->{timeout_options}{ping}{timeout} || 1;
    my $ping_attempts = $self->{timeout_options}{ping}{attempts} || 1;

    LWP::Protocol::implementor( http => "LWP::Protocol::http" );

    foreach my $endpoint ( @{$system_endpoints} ) {
        $endpoint =~ s!^tcp://!http://!;
        my $ping_base_url = URI::URL->new("/system/ping", $endpoint);

        WORKER: for my $worker_id ( 0..$workers_num-1 ) {
            for my $attempt ( 1..$ping_attempts ) {
                my $ua = LWP::UserAgent->new;
                $ua->timeout($ping_timeout);

                my $ping_url = $ping_base_url->clone;
                $ping_url->base->port($ping_url->base->port + $worker_id);

                my $ping_response = $ua->get( $ping_url->abs );
                next WORKER if $ping_response->is_success;

                print { *STDERR } "invalid ping response from worker " . $worker_id .
                    ": " . $ping_response->message . "\n";
            }

            return result("broken", "failed to ping worker " . $worker_id);
        }
    }

    return "running";
}

sub start_impl {
    my ($self) = @_;

    my $config = $self->_config();
    # Validate that all system endpoints are in the form of "tcp://..."
    foreach my $endpoint ( @{$config->{application}{system_endpoints}} ) {
        die "invalid system endpoint: $endpoint"
            if $endpoint !~ m!^tcp://!;
    }

    Ubic::Service::Shared::Dirs::directory_checker( $self->{run_dir}, $self->{daemon_user} );
    Ubic::Service::Shared::Dirs::directory_checker( "/var/log/s3-mds-proxy-arc/", $self->{daemon_user} );

    $self->SUPER::start_impl();

    return "starting";
}

sub timeout_options {
    my ($self) = @_;

    return $self->{timeout_options};
}


1;


=head1 NAME

Ubic::Service::S3MdsProxy - run S3-MDS Proxy as Ubic service

=head1 SYNOPSIS

# /etc/ubic/service/s3-mds-proxy-arc.json
  {
      "module": "Ubic::Service::S3MdsProxyArc",
      "options": {
          "config": "/etc/s3-mds-proxy/s3-mds-proxy-arc.conf",
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

This software is copyright (c) 2016 by Yandex LLC.

This is free software; you can redistribute it and/or modify it under
the same terms as the Perl 5 programming language system itself.

=cut
