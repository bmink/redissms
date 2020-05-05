#!/bin/bash
#
# Specify this script as "eventhandler" in /etc/smsd.conf
#

REDIS_ADDR="192.168.1.128" /home/bmink/devel/redissms/redissms_receiver "$1" "$2"
