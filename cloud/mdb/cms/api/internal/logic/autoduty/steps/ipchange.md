# Handle IP changed

CMS remembers IPs before mutating ops. When dom0 returns, it finds out
the actual IPs configuration. If something changes (happens when rack was moved)
it executes steps to bring clusters to live correctly.
