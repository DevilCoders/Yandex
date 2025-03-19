#!/usr/bin/perl -w

use strict;

my $prev_file = "/tmp/graphite.netstat.prev";

# Signals interrupts
$SIG{"__WARN__"} = \&SigWarn;
$SIG{"__DIE__"} = \&SigDie;

# Interrupt SIGWARN
sub SigWarn(@) {
    my ($message) = @_;

    $message =~ s/ at .*? line \d+\.$//si;

    print(STDERR "[netstat] " . $message);
}

# Interrupt SIGDIE
sub SigDie(@) {
    &SigWarn(@_);
    exit(1);
}

my %ifaces = ();

my $time = time();
my $prev_time = $time;


### [ Reading ProcFS

    open(PROC, "< /proc/net/dev") or die($!);
    
    # Processing all interfaces
    foreach my $line (<PROC>) {
        chomp($line);
        
        next unless ($line);
        
        # eth0.1329: 1651365509266 293951052    0    0    0     0          0     14187 346472294917 345394068    0    0    0     0       0          0
        if (my ($iface, $bytes_rx, $packets_rx, $dropped_rx, $bytes_tx, $packets_tx, $dropped_tx) = $line =~ m/^\s*(?!veth)(.*?):\s*(\d+)\s+(\d+)\s+\d+\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+(\d+)\s+(\d+)\s+\d+\s+(\d+)\s+/si) {
            $ifaces{$iface}{"bytes_rx"} = $bytes_rx;
            $ifaces{$iface}{"bytes_tx"} = $bytes_tx;
            $ifaces{$iface}{"packets_rx"} = $packets_rx;
            $ifaces{$iface}{"packets_tx"} = $packets_tx;
            $ifaces{$iface}{"dropped_rx"} = $dropped_rx;
            $ifaces{$iface}{"dropped_tx"} = $dropped_tx;
        }
    }
    
    close(PROC);

### ]


### [ Storing values

    # Read previous values, if available
    if (open(RESULT, $prev_file)) {
        foreach my $result (<RESULT>) {
            chomp($result);
            
            next unless ($result);
            
            if (my ($iface, $bytes_rx, $bytes_tx, $packets_rx, $packets_tx, $dropped_rx, $dropped_tx) = $result =~ m/^iface:(.*?)\sbytes_rx:(\d+)\sbytes_tx:(\d+)\spackets_rx:(\d+)\spackets_tx:(\d+)\sdropped_rx:(\d+)\sdropped_tx:(\d+)$/si) {
                
                $ifaces{$iface}{"prev_bytes_rx"} = $bytes_rx;
                $ifaces{$iface}{"prev_bytes_tx"} = $bytes_tx;
                $ifaces{$iface}{"prev_packets_rx"} = $packets_rx;
                $ifaces{$iface}{"prev_packets_tx"} = $packets_tx;
                $ifaces{$iface}{"prev_dropped_rx"} = $dropped_rx;
                $ifaces{$iface}{"prev_dropped_tx"} = $dropped_tx;
            } else {
                ($prev_time) = $result =~ m/^time:(\d+)$/si;
            }
        }
        
        close(RESULT);
    }
    
    
    # Saving current values
    open(RESULT, ">", $prev_file) or die("Can't save current values" . $!);
    
    foreach my $iface (keys(%ifaces)) {
        print(RESULT
            "iface:"      . $iface                        . " " .
            "bytes_rx:"   . $ifaces{$iface}{"bytes_rx"}   . " " .
            "bytes_tx:"   . $ifaces{$iface}{"bytes_tx"}   . " " .
            "packets_rx:" . $ifaces{$iface}{"packets_rx"} . " " .
            "packets_tx:" . $ifaces{$iface}{"packets_tx"} . " " .
            "dropped_rx:" . $ifaces{$iface}{"dropped_rx"} . " " .
            "dropped_tx:" . $ifaces{$iface}{"dropped_tx"} , "\n"
        
        );
    }
    
    print(RESULT "time:" . $time, "\n");
    
    close(RESULT);

### ]


### [ Making result

    $time = 1 unless ($time = $time - $prev_time);


    # Bytes
    
    foreach my $iface (keys(%ifaces)) {
      if (defined($ifaces{$iface}{"bytes_rx"}) and defined($ifaces{$iface}{"bytes_tx"})) {
        printf ("%s.bytes.rx %d\n", $iface, (defined($ifaces{$iface}{"prev_bytes_rx"})) ? ($ifaces{$iface}{"bytes_rx"} - $ifaces{$iface}{"prev_bytes_rx"}) / $time : 0);
        printf ("%s.bytes.tx %d\n", $iface, (defined($ifaces{$iface}{"prev_bytes_tx"})) ? ($ifaces{$iface}{"bytes_tx"} - $ifaces{$iface}{"prev_bytes_tx"}) / $time : 0);
      }
    }
    
    # Packets
    foreach my $iface (keys(%ifaces)) {
      if (defined($ifaces{$iface}{"packets_rx"}) and defined($ifaces{$iface}{"packets_tx"})) {
        printf ("%s.packets.rx %d\n", $iface, (defined($ifaces{$iface}{"prev_packets_rx"})) ? ($ifaces{$iface}{"packets_rx"} - $ifaces{$iface}{"prev_packets_rx"}) / $time : 0);
        printf ("%s.packets.tx %d\n", $iface, (defined($ifaces{$iface}{"prev_packets_tx"})) ? ($ifaces{$iface}{"packets_tx"} - $ifaces{$iface}{"prev_packets_tx"}) / $time : 0);
      }
    }

    # Drops
    foreach my $iface (keys(%ifaces)) {
      if (defined($ifaces{$iface}{"dropped_rx"}) and defined($ifaces{$iface}{"dropped_tx"})) {
        printf("%s.dropped.rx %d\n", $iface, (defined($ifaces{$iface}{"prev_dropped_rx"})) ? ($ifaces{$iface}{"dropped_rx"} - $ifaces{$iface}{"prev_dropped_rx"}) / $time : 0);
        printf("%s.dropped.tx %d\n", $iface, (defined($ifaces{$iface}{"prev_dropped_tx"})) ? ($ifaces{$iface}{"dropped_tx"} - $ifaces{$iface}{"prev_dropped_tx"}) / $time : 0);
      }
    }

### ]


1;
