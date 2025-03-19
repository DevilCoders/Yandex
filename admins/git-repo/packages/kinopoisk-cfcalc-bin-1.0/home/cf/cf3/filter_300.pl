#!/usr/bin/perl -F, -lan 
BEGIN{ $old_u=0;$cnt = 0}
$uid = $F[1] + 0;
if ( $uid != $old_u ) {
        $old_u = $uid;
        $cnt = 0;
    shift @F;
        print join(',',@F);
        ++ $cnt;
} elsif( $cnt < 300 ) {
    shift @F;
        print join(',',@F);
        ++ $cnt;
}

