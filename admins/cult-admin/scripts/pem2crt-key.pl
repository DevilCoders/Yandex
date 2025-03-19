#!/usr/bin/perl -w

if ($#ARGV != 0 && $#ARGV != 1) {
    print "usage: $0 <input.pem> [basename]\n";
    exit -1;
}

my $name;
my $input = "$ARGV[0]";

if (! -e $input) {
        die "No such file $input";
} elsif (! -r $input) {
        die "Can't read file $input";
}

open(PEM, "<", "$input");

if ($#ARGV == 1) {
        $name = $ARGV[1];
} else {
        ($name = "$input") =~ s/\.[^.]+$//;
}

my $crtname = $name . ".crt";
my $keyname = $name . ".key";

print "$crtname\n";
print "$keyname\n";

open(CRT, ">", "$crtname");
open(KEY, ">". "$keyname");

while (<PEM>) {
        if (/-----BEGIN CERTIFICATE-----/../-----END CERTIFICATE-----/) {
                print CRT;
        }

        if (/-----BEGIN PRIVATE KEY-----/../----END PRIVATE KEY-----/) {
                print KEY;
        }
}

exit 0;
