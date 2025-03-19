package TimeUtils;

use 5.008004;
use strict;
use warnings;

my $default_type = 'common';

my %types = (
    'common', '(?:^|\s)\[([^\]]+)\]',
    'java', '(\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d)',
    'syslog', '(\w+\s+\d+\s+\d+:\d+:\d+)',
    'lighttpd', ' - \[([^\]]+)\]',
    'baida', '(\d+\/\w+\/\d{4}:\d\d:\d\d:\d\d\s[^\s]+)',
    'tskv', '(\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d)',
    'tskv2', '(\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d)',
    'tskv-timestamp', 'timestamp=(\d+)',
    'squid', '^(\d+)\.',
    'mongodb', '^.*T([0-9\:]+)\..*',
    'phantom', '^\d+\s+(\d+-\d+-\d+\s+\d+:\d+:\d+)',
    'clck-error', '\tts=(\d+)\t',
    'alertlog', '^([STMWF][uoehra][neduit].*\s+\d\d\d\d)$',
    'statbox', '^\[(.+? \d\d:\d\d:\d\d)',
    'listenerlog', '(\d+-\w+-\d+\s+\d+:\d+:\d+)',
    'syslog-ng', '^(\d+-\d+-\d+T\d+:\d+:\d+)\+\d+:\d+',
    'yplatform', '^\[(\d+\/\w+\/\d{4}:\d\d:\d\d:\d\d)\s[^\s]+\]',
    'mulcagate', '\[.+?\s+(\d+:\d+:\d+)',
    'crypta', '^\[(\d{4}\/\d\d\/\d\d\s\d\d:\d\d:\d\d)',
    'handystats', '^(\d+)\d\d\d\s',
    'gofetcher', '(\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d)',
    'loadlog', '^[-\d]+\t(\d+)\t',
    'passagelog', '^[^\t]+\t\d+\t\d+\t(\d+)\d{6}\t',
    'imap', '^\[\d{4}-\w+-\d{2} (\d\d:\d\d:\d\d\.\d{6})',
    'imap-timing', '^\[\d{4}-\d{2}-\d{2} (\d\d:\d\d:\d\d\.\d{3})',
    'duffman-http-log', '([0-9][0-9]:[0-9][0-9]:[0-9][0-9])',
    'nginx-json', '"time_local":"\d\d/\w+/\d{4}:(\d\d:\d\d:\d\d)',
    'envoy-json', '"start_time":"[0-9:T\-\+]+ (\d+)"'
);

sub default_type() {
    return $default_type;
}

sub default_regexp() {
    return $types{$default_type};
}

sub get_regexp($) {
    my $type = shift;
    return $types{$type} if exists($types{$type});
    return undef;
}

sub valid_types() {
    my @valid;
    foreach (sort keys %types) {
        push(@valid, $_);
    }
    return @valid;
}

1;
