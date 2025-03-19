#!/bin/bash
echo -e "\033[1;36mBootstrap TC agent"
sudo /usr/local/bin/skm decrypt
sudo ssh-keygen -y -f /home/robot-yc-ci/.ssh/id_rsa > /home/robot-yc-ci/.ssh/id_rsa.pub
sudo ssh-keygen -y -f /home/robot-yc-ci/.ssh/robot-ycloud-dev.key > /home/robot-yc-ci/.ssh/robot-ycloud-dev.pub
sudo ssh-keygen -y -f /home/robot-yc-ci/.ssh/robot-ycloud-testing.key > /home/robot-yc-ci/.ssh/robot-ycloud-testing.pub
sudo chattr +i /home/robot-yc-ci/.docker/config.json
sudo /usr/sbin/tca-sbuild-bootstrap docker teamcity-agents cleanup
