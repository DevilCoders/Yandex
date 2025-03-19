#! /usr/bin/perl

<>;
while (<>) {
        ($time, $gmt, $host, $ip, $column5, $column6, $column7, $answer, $others) = split (/ +/, $_, 9);
#       print "$host $answer\n";
        if ("-" ne $host) {
                if (($answer==200) || ($answer=~ m~50\d~) || ($answer==404)) {
                        $h_hosts{$host}{$answer}++;
                }
        }
}

$total200 = 0;
$total500 = 0;
$total404 = 0;

foreach $host ( keys %h_hosts ) {
        ($host_without_port, $port) = split (/:/,$host);
        print "$host_without_port\t";
        $answers500 = 0;
        for ($i = 0; $i <= 9; $i++) {
                $answer = "50".$i;
                $answers500 += $h_hosts{$host}{$answer};
        }
        #if (!$h_hosts{$host}{500}) { $h_hosts{$host}{500} = 0};
        if ($h_hosts{$host}{200}) {
                print "$h_hosts{$host}{200}\t";
                $total200 += $h_hosts{$host}{200};
        } else { print "0\t";}
        print "$answers500\t";
        $total500 += $answers500;
        if ($h_hosts{$host}{404}) {
                print "$h_hosts{$host}{404}\n";
                $total404 += $h_hosts{$host}{404};
        } else { print "0\n";}
}
