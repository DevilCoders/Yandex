#!/usr/bin/perl -w

print "Content-type: text/html\n\n";
print  '<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">' ;
print "<HTML>\n<HEAD>\n<TITLE>test</TITLE>\n</HEAD>\n<BODY>\n" ;
print "$ENV{REMOTE_USER}<BR>";
my $uid = getpwnam($ENV{REMOTE_USER});

# set guids of cvs-groups, if any
my $newguids = "70 70" ;
if ( (getgrnam('corbacvs'))[3] =~ /$ENV{REMOTE_USER}/ ) {
    $newguids = $newguids . " 72" ; }

if ( (getgrnam('noccvs'))[3] =~ /$ENV{REMOTE_USER}/ ) {
    $newguids = $newguids . " 100" ; }
my $grn =  (getgrnam('noccvs'))[3];
print $grn ;
# set uid accordint to username
$< = getpwnam($ENV{REMOTE_USER});
$> = getpwnam($ENV{REMOTE_USER});

printf ( "EGids: %s<BR>UIDS: %s %s<BR>\n", $), $<, $> );
print "</BODY>\n</HTML>\n" ;
exit 0;
