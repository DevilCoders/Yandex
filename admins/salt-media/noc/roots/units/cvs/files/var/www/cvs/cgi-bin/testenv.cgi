#!/usr/bin/perl -w

print "Content-type: text/html\n\n";
print  '<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">' ;
print "<HTML>\n<HEAD>\n<TITLE>test</TITLE>\n</HEAD>\n<BODY>\n" ;
print "$ENV{REMOTE_USER}<BR>";
my $uid = getpwnam($ENV{REMOTE_USER});

printf ( "EGids: %s<BR>UIDS: %s %s<BR>\n", $), $<, $> );
print "</BODY>\n</HTML>\n" ;
exit 0;
