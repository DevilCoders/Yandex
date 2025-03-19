#!/bin/bash
        revision=$(cat revision | grep -v "#" | sed 's/ //g')
        mkdir -p debian/lxctl-template-ubuntu-14.04-amd64/var/lxc/templates/
        cp trusty.tar.gz debian/lxctl-template-ubuntu-14.04-amd64/var/lxc/templates/ubuntu-14.04-amd64-$(echo $revision).tar.gz
        cp trusty-autosetup.tar.gz debian/lxctl-template-ubuntu-14.04-amd64/var/lxc/templates/ubuntu-14.04-amd64-autosetup-$(echo $revision).tar.gz
        echo "var/lxc/templates/ubuntu-14.04-amd64-$(echo $revision).tar.gz var/lxc/templates/ubuntu-14.04-amd64.tar.gz" > debian/links
        echo "var/lxc/templates/ubuntu-14.04-amd64-$(echo $revision).tar.gz var/lxc/templates/trusty.tar.gz" >> debian/links
        echo "var/lxc/templates/ubuntu-14.04-amd64-autosetup-$(echo $revision).tar.gz var/lxc/templates/ubuntu-14.04-amd64-autosetup.tar.gz" >> debian/links
        echo "var/lxc/templates/ubuntu-14.04-amd64-autosetup-$(echo $revision).tar.gz var/lxc/templates/trusty-autosetup.tar.gz" >> debian/links

