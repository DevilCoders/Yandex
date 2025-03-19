#!/bin/sh                                                                                                                    
ETH='eth1'                                                                                                                   
T1=`cat /proc/net/dev|grep "$ETH"|perl -pe 's/\s+/ /g'|cut -d' ' -f 11`                                                      
sleep 1                                                                                                                      
T2=`cat /proc/net/dev|grep "$ETH"|perl -pe 's/\s+/ /g'|cut -d' ' -f 11`                                                      
                                                                                                                             
echo $T2-$T1 |bc -l  
