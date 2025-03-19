#!/usr/bin/perl

use strict;

our %emls;

do "/etc/juggler_flapper_media.conf";

my $events = `find /var/log/stat01g.media.yandex.net/media-jmon-*/*/juggler/sender/ -iname 'sender.log*' -mtime -7 | egrep -v sender.log\$ | xargs zcat -f | egrep 'Sent \\[primary\\] GOLEM' | awk '{print \$14" "\$19}'`;

my @events = split /\n/, $events;
my %projects;
my $hostname = `hostname -f`;
chomp $hostname;

for my $event (@events) {
	my $e = $event;
	if ($e =~ s/^([a-zA-Z0-9]+).*$/\1/) {
		push @{$projects{$e}}, $event;
	}
}

for my $k (keys %projects) {
	generate_project($k, @{$projects{$k}});
}

sub generate_project
{
	my ($project, @events) = @_;
	my %p_events;

	return if not defined $emls{$project};

	for my $e (@events) {
		my ($check, $status) = split / +/, $e;
		if (!defined($p_events{$check}[$#{$p_events{$check}}]) or $p_events{$check}[$#{$p_events{$check}}] ne $status) {
			push @{$p_events{$check}}, $status;
		}
	}

        my $project_out = uc($project) . ": \\n";
        for my $k (sort { @{$p_events{$b}} <=> @{$p_events{$a}} } keys %p_events) {
                my $num = 0;
                foreach (@{$p_events{$k}}){
                        $num++ if $_ =~ "CRIT";
                }
                $project_out .= "$k: " . $num . "\n" if $num;
        }
        $project_out .= "\n";

	print "$project " . $emls{$project} . "\n";

	`echo -e '$project_out' | mail -r 'juggler-events\@$hostname' -s 'Juggler flaps report for $project' $emls{$project}`;
}
