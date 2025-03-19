#!/bin/bash

on_install() {
    find /var/log/ -type f -exec cp /dev/null {} \;
    apt-get clean
    dhclient -r && rm /var/lib/dhcp/dhclient.*
}
