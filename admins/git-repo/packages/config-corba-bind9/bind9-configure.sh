#!/bin/bash

v4_config () {
    rm /etc/bind/named.conf.options
    ln -s /etc/bind/named.conf.options-v4
}

v6_config () {
    rm /etc/bind/named.conf.options
    ln -s /etc/bind/named.conf.options-v6
}

v6_config

