#!/usr/bin/perl
use POSIX;
use Time::Local;
use WWW::Curl::Easy;
#CONFIG
my $CONF   = "/etc/mail-fallback.conf";
if (-e $CONF) {
    require "$CONF";
}
#/CONFIG

open (my $lg, '>>', '/var/log/mail-fallback.log');
#open (STDERR, ">>&=", /dev/null); 
my $filename = '/etc/yandex/environment.type';
open(my $fh, '<:encoding(UTF-8)', $filename) or die "Could not open file '$filename' $!";
my $env = <$fh>;
chomp $env;

my @mid;
my $year = `date '+%Y'`;
my $now = timelocal(`date '+%S'`,`date '+%M'`,`date '+%H'`,`date '+%d'`,`date '+%b'`,$year);
my $now_log = localtime time;
$raw_times = `/usr/sbin/postqueue -p | awk '{print \$4,\$5,\$6,\$1}'`;
$n = "\n";
@times = split(/$n/,$raw_times);
foreach $time (@times)
{
	if ($time =~ m/(\w{3})\s(\d+)\s(\d+):(\d+):(\d+)\s(\w+)/)
	{
		$timestamp = timelocal($5,$4,$3,$2,$1,$year);
		if (($now - $timestamp) > 300)
		{
			
			$uuid = `/usr/sbin/postcat -q ${6} | grep 'X-Yandex-Kassa-OrderKey'`;
			if ($uuid){
                        	@uuid2 = split(/:/,$uuid);
                                push(@mid,${6}); 
				if ($uuid2[1]) 
               			{
			   		chomp($uuid2[1]) ;
			   		$uuid2[1] =~ s/^\s+|\s+$//g;	
			   		$ids=$ids.$uuid2[1].";"
   
                       		 }
		         }						
		}
	}
}
if ($ids) {
	@aids=split(/;/,$ids);
	$body="data=".$ids;
	$curl = WWW::Curl::Easy->new();
        @surls=split(/ /, $send_urls{$env});
	foreach $surl (@surls){
		$curl->setopt(CURLOPT_URL,  $prefix.$surl.$ruchka );
		$curl->setopt(CURLOPT_POST, 1);
		$curl->setopt(CURLOPT_TIMEOUT , 3);
		$curl->setopt(CURLOPT_POSTFIELDS, $body);
		$curl->setopt(CURLOPT_WRITEDATA, \$content);
		$ccode = $curl->perform();
		if ($ccode == 0) {
    			$status = $curl->getinfo(CURLINFO_HTTP_CODE);
    			if ($status == 200) {
        			#Removing mail
				$n=0;
				foreach $idrm (@mid) {
 					system ("/usr/sbin/postsuper -d ".$idrm.">/dev/null &2>/dev/null");
		        		print $lg $now_log." Remove ".$idrm." ".$aids[$n]."\n";
					$n++;
				}
				last;
    			} else {
        			print $lg $now_now." Status: $status\n";
    			}
		} else {
    			print $lg $now_log." Request failed .An error happened: $ccode ".$curl->strerror($ccode)." ".$curl->errbuf."\n";
		}
	}
}
close ($lg);
