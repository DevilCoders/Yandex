#!/usr/bin/perl -w

use strict;

my $prev_file = '/tmp/graphite.cpuload.prev';

if (open(SOURCE, "< /proc/stat")) {
    # Получаем текущие значения
    my ($us, $ni, $sy, $id, $io, $hi, $si) = <SOURCE> =~ m/cpu\s+(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)/si;
    my $time = $us + $ni + $sy + $id + $io + $hi + $si;
    
    close(SOURCE);
    
    # Получаем предыдущие значения
    my ($prev_us, $prev_ni, $prev_sy, $prev_id, $prev_io, $prev_hi, $prev_si) = ($us, $ni, $sy, $id, $io, $hi, $si);
    my $prev_time = $time;
    
    if (open(RESULT, $prev_file)) {
        foreach my $result (<RESULT>) {
    	    chomp($result);
    	    
            next unless ($result);
            
            if (($prev_us, $prev_ni, $prev_sy, $prev_id, $prev_io, $prev_hi, $prev_si, $prev_time) = $result =~ m/us:(\d+)\sni:(\d+)\ssy:(\d+)\sid:(\d+)\sio:(\d+)\shi:(\d+)\ssi:(\d+)\stime:(\d+)/si) {
                last;
            }
        }
        
        close(RESULT);
    }
    
    # Сохраняем текущие значения
    if (open(RESULT, ">", $prev_file)) {
        print(RESULT "us:$us ni:$ni sy:$sy id:$id io:$io hi:$hi si:$si time:$time");
    
        close(RESULT);
    } else {
        warn("Error (cpuload) $?: \"$!\"");
    }
    
    # Вычисляем загрузку процессора
    $time = 1 unless ($time = $time - $prev_time);
   
    my $sender = "";
    $sender .= sprintf("user %.2f\n", 100 * ($us - $prev_us) / $time);
    $sender .= sprintf("nice %.2f\n", 100 * ($ni - $prev_ni) / $time);
    $sender .= sprintf("system %.2f\n", 100 * ($sy - $prev_sy) / $time);
    $sender .= sprintf("idle %.2f\n", 100 * ($id - $prev_id) / $time);
    $sender .= sprintf("iowait %.2f\n", 100 * ($io - $prev_io) / $time);
    $sender .= sprintf("hard_irq %.2f\n", 100 * ($hi - $prev_hi) / $time);
    $sender .= sprintf("soft_irq %.2f\n", 100 * ($si - $prev_si) / $time);
    print "$sender";
} else {
    warn("Error (cpuload) $?: \"$!\"")
}


1;
