#!/usr/bin/perl -T

use v5.010;

while (<>) {
    %keys = $_ =~ /(\S+?)=([\S ]*)\t*/g;

    if ($keys{request} eq '/ping') {
        next;
    }

    $c = $keys{status};
    $r = $keys{request};
    $h = $keys{vhost};
    $t = $keys{request_time};
    $ua = $keys{upstream_addr};

    $h =~ s/.yandex\.\w+$//;
    $h =~ y/\./_/;

    $n = "code." . "$h" . ".$c";
    $ac{$n}++;

    $n = "time." . "$h" . ".total_timings";
    $at{$n}->{$t}++;

    $n = "time.total.total_timings";
    $at{$n}->{$t}++;

    if ($c =~ /^2..$/) {
        $n = "time." . "$h" . ".2xx_timings";
        $at{$n}->{$t}++;

        $n = "time.total.2xx_timings";
        $at{$n}->{$t}++;
    }

    my %pages_re = (
                    all => '^.*?$',
                    index => '^\/\d*(\?.*?)?$',
                    program => '^\/\d+\/program',
                    channels => '^\/\d+\/channels',
                    special => '^\/special',
                    special_filfak => '\/special\/filfak',
                    my_favorites => '\/(\d+\/)?my\/favorites',
                    search => '\/(\d+\/)?search',
                    player => '\/(\d+\/)?channels/(\d+\/)?player',
                    stream => '\/(\d+\/)?channels/(\d+\/)?stream',
                    ajax => '\/ajax\/',
                    sport_teaser => '\/ajax\/i-tv-sport-teaser\/',
                    lists => '\/(\d+\/)?lists\/',
                    api => '^\/api\/',
                    online => '^\/online',
                    sport => '^/(\d+\/)?sport\/',
                   );

    for my $page (keys %pages_re) {
        my $page_re = $pages_re{$page};

        if ($r =~ /$page_re/) {
            $n = $page . ".code." . "$h" . ".$c";
            $ac{$n}++;

            $n = $page . ".time." . "$h" . ".total_timings";
            $at{$n}->{$t}++;

            $n = $page . ".time.total.total_timings";
            $at{$n}->{$t}++;

            if ($c =~ /^2..$/) {
                $n = $page . ".time." . "$h" . ".2xx_timings";
                $at{$n}->{$t}++;

                $n = $page . ".time.total.2xx_timings";
                $at{$n}->{$t}++;
            }
        }
    }

    # CADMIN-6487: считаем статистику только для трафика который идет в приложение
    if ($ua ne "[2a02:6b8::402]:443" and $r !~ /\.(json|js|ico|txt|gif|png|jpe?g|)$/) {
        $n = "apponly.code." . "$h" . ".$c";
        $ac{$n}++;
        $n = "apponly.time." . "$h" . ".total_timings";
        $at{$n}->{$t}++;
        $n = "apponly.time.total.total_timings";
        $at{$n}->{$t}++;

        if ($c =~ /^2..$/) {
            $n = "apponly.time." . "$h" . ".2xx_timings";
            $at{$n}->{$t}++;

            $n = "apponly.time.total.2xx_timings";
            $at{$n}->{$t}++;
        }
    }

}

for (keys %ac) {
    say "$_ $ac{$_}";
}

for $name (keys %at) {
    print "\@$name ";
    for (keys %{$at{$name}}) {
        printf "%s\@%d ", $_, $at{$name}->{$_};
    }
    print "\n";
}
