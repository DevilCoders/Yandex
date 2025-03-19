#!/usr/bin/env perl

use strict;
use warnings;
use HTTP::Tiny;
use JSON::PP;
use POSIX qw(strftime);
use Sys::Hostname;
use Digest::SHA qw(sha256_hex);
use Getopt::Long qw(GetOptions :config gnu_getopt);

sub true {1}
sub false {0}

sub get_options {
    my ($opts, $config) = @_;
    my %cmd_args;
    GetOptions(\%cmd_args, @{$opts}) or die join("\n\t", ("Usage: $0", map {'--' . $_} @{$opts})) . "\n";
    foreach (keys %cmd_args) {$config->{$_} = $cmd_args{$_};}
    return $config;
}

sub current_time {
    return strftime('%Y.%m.%d %H:%M:%S', localtime());
}

sub http_response {
    my ($response, $decode_json) = @_;
    my $content;
    if ($response->{'success'}) {$content = $decode_json ? decode_json($response->{'content'}) : $response->{'content'};}
    else {die sprintf("Error %s [%s], %s: %s\n", $response->@{qw(status url reason content)});}
    return $content;
}

sub request_post {
    my ($url, $headers, $post_content, $decode_json) = @_;
    my $http = HTTP::Tiny->new('verify_SSL' => false);
    my $response = $http->post($url, { 'headers' => $headers, 'content' => encode_json($post_content) });
    my $content = http_response($response, $decode_json);
    return $content;
}

sub request_get {
    my ($url, $headers, $query_params, $decode_json) = @_;
    my $http = HTTP::Tiny->new('verify_SSL' => false);
    my $full_url = defined($query_params) ? sprintf('%s?%s', $url, $http->www_form_urlencode($query_params)) : $url;
    my $response = $http->get($full_url, { 'headers' => $headers });
    my $content = http_response($response, $decode_json);
    return $content;
}

sub get_iam_token {
    my ($oauth_token) = @_;
    my $url = 'https://gw.db.yandex-team.ru/iam/v1/tokens';
    my $headers = { 'Content-Type' => 'application/json' };
    my $post_content = { 'yandexPassportOauthToken' => $oauth_token };
    my $content = request_post($url, $headers, $post_content, true);
    return $content;
}

sub get_mdb_redis_cluster_hosts {
    my ($mdb_redis_cluster, $iam_token) = @_;
    my $url = sprintf('https://gw.db.yandex-team.ru/managed-redis/v1/clusters/%s/hosts', $mdb_redis_cluster);
    my $headers = { 'Authorization' => 'Bearer ' . $iam_token->{'iamToken'} };
    my $content = request_get($url, $headers, undef, true);
    my %mdb_hosts;
    foreach (@{$content->{'hosts'}}) {push @{$mdb_hosts{$_->{'zoneId'}}}, $_->{'name'};}
    return %mdb_hosts;
}

sub get_host_dc {
    my ($host) = @_;
    my $url = sprintf('https://c.yandex-team.ru/api/hosts/%s', $host);
    my $content = request_get($url, {}, { 'format' => 'json' }, true);
    return $content->[0]->{'datacenter'};
}

# Settings
my $haproxy_config_filename = '/etc/haproxy/haproxy.cfg';
my @php_config_filenames = qw(
    /home/www/kinopoisk.ru/config/.env.salt
    /etc/php/5.6/fpm/secrets.conf
    /etc/php/5.6/fpm/secrets-env.sh
);
my $php_redis_sentinel_env_name = 'SYMFONY_REDIS_CACHE_SENTINELS';
my $sentinel_port = 26379;
my $redis_port = 6379;
my %dc_mapping = ('iva' => 'vla');

my $mdb_oauth_token = $ENV{'MDB_OAUTH_TOKEN'};
my $mdb_redis_cluster = $ENV{'MDB_REDIS_CLUSTER'};
my $current_dc = get_host_dc(hostname);
#

my $config = get_options([ 'restart' ], { 'restart' => false });

# Get IAM-token
my $iam_token = get_iam_token($mdb_oauth_token);

# Get MDB cluster hosts
my %mdb_cluster_hosts = get_mdb_redis_cluster_hosts($mdb_redis_cluster, $iam_token);

# Sort hosts in right order
if (exists($dc_mapping{$current_dc})) {$current_dc = $dc_mapping{$current_dc};}
my $only_another_dcs = true;
my @ordered_dc_list;
if (exists($mdb_cluster_hosts{$current_dc})) {
    $only_another_dcs = false;
    @ordered_dc_list = ($current_dc, sort grep {$_ ne $current_dc} keys %mdb_cluster_hosts);
}
else {@ordered_dc_list = sort keys %mdb_cluster_hosts;}

# Make hosts lists
my @redis_hosts_sentinel;
my @haproxy_upstreams;
foreach my $dc (@ordered_dc_list) {
    push @redis_hosts_sentinel, map {sprintf('%s:%d', $_, $sentinel_port)} sort @{$mdb_cluster_hosts{$dc}};
    push @haproxy_upstreams,
        map {
            sprintf(
                'server %s %s:%d check port %d inter 1s fall 3 rise 5 on-marked-down shutdown-sessions%s',
                $_,
                $_,
                $redis_port,
                $redis_port,
                ($only_another_dcs or ($dc eq $current_dc)) ? '' : ' backup'
            )
        }
            sort @{$mdb_cluster_hosts{$dc}};
}

# Generate environment variable for PHP
my $php_redis_sentinel_env = join(',', @redis_hosts_sentinel);

# Generate HAProxy config
my $haproxy_config = sprintf(
    'global
        log 127.0.0.1 local0 notice
        maxconn 65535
        user haproxy
        group haproxy
        quiet
        stats socket /tmp/haproxy_stat

defaults
        log global
        mode tcp
        option clitcpka
        option srvtcpka
        retries 3
        maxconn 65535
        timeout connect 500
        timeout client 90000
        timeout server 90000
        balance leastconn

resolvers mydns
        parse-resolv-conf
        resolve_retries 3
        timeout resolve 1s
        timeout retry 1s
        hold other 30s
        hold refused 30s
        hold nx 30s
        hold timeout 30s
        hold valid 10s
        hold obsolete 30s

listen stats
        bind *:18888,:::18888
        mode http
        stats enable
        stats uri /

listen redis_16379
        bind :::16379 v4v6
        option allbackups
        option tcp-check
        tcp-check connect
        tcp-check send PING\r\n
        tcp-check expect rstring ^(\+OK|\+PONG|-NOAUTH)
        tcp-check send QUIT\r\n
        tcp-check expect string +OK
        %s
',
    join("\n        ", @haproxy_upstreams)
);

# Check old HAProxy config
if (-f -r $haproxy_config_filename) {
    my $haproxy_config_new_hash = sha256_hex($haproxy_config);
    my $haproxy_config_old_hash = Digest::SHA->new(256)->addfile($haproxy_config_filename, 'b')->hexdigest;
    if ($haproxy_config_old_hash eq $haproxy_config_new_hash) {exit;}
}

printf("[%s] MDB Redis cluster: %s\n",
    current_time,
    JSON::PP->new->utf8->space_after->canonical->encode(\%mdb_cluster_hosts)
);

# Write environment variable to PHP files
foreach my $filename (@php_config_filenames) {
    # Read and filter file content
    my @file_content;
    if (open(my $fh, '<', $filename)) {
        while (<$fh>) {
            chomp;
            if (/$php_redis_sentinel_env_name/) {
                my $env_name_in_file = (split(/\s*=\s*/, $_, 2))[0];
                push @file_content, sprintf('%s="%s"', $env_name_in_file, $php_redis_sentinel_env);
                next;
            }
            push @file_content, $_;
        }
    }
    else {die sprintf("Could not open file '%s': %s\n", $filename, $!);}
    # Write file
    printf("[%s] Writing PHP config: %s\n", current_time, $filename);
    if (open(my $fh, '>', $filename)) {foreach (@file_content) {print $fh $_ . "\n";}}
    else {die sprintf("Could not open file '%s': %s\n", $filename, $!);}
}

# Make HAProxy config file
printf("[%s] Writing HAProxy config: %s\n", current_time, $haproxy_config_filename);
if (open(my $fh, '>', $haproxy_config_filename)) {print $fh $haproxy_config;}
else {die sprintf("Could not open file '%s': %s\n", $haproxy_config_filename, $!);}

# Restart services
if ($config->{'restart'}) {
    printf("[%s] Restarting services: HAProxy, PHP\n", current_time);
    system(qw(/bin/systemctl restart haproxy.service));
    system(qw(/usr/bin/ubic restart php));
}
