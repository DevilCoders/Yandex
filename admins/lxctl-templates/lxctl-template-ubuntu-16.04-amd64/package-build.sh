#!/bin/bash
        revision=$(cat revision | grep -v "#" | sed 's/ //g')
        mkdir -p debian/lxctl-template-ubuntu-16.04-amd64/var/lxc/templates/
        cp xenial.tar.gz debian/lxctl-template-ubuntu-16.04-amd64/var/lxc/templates/ubuntu-16.04-amd64-$(echo $revision).tar.gz
        cp xenial-autosetup.tar.gz debian/lxctl-template-ubuntu-16.04-amd64/var/lxc/templates/ubuntu-16.04-amd64-autosetup-$(echo $revision).tar.gz
        echo "var/lxc/templates/ubuntu-16.04-amd64-$(echo $revision).tar.gz var/lxc/templates/ubuntu-16.04-amd64.tar.gz" > debian/links
        echo "var/lxc/templates/ubuntu-16.04-amd64-$(echo $revision).tar.gz var/lxc/templates/xenial.tar.gz" >> debian/links
        echo "var/lxc/templates/ubuntu-16.04-amd64-autosetup-$(echo $revision).tar.gz var/lxc/templates/ubuntu-16.04-amd64-autosetup.tar.gz" >> debian/links
        echo "var/lxc/templates/ubuntu-16.04-amd64-autosetup-$(echo $revision).tar.gz var/lxc/templates/xenial-autosetup.tar.gz" >> debian/links

