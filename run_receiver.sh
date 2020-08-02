#!/bin/bash
#
# Specify this script as "eventhandler" in /etc/smsd.conf
#

REDIS_ADDR="xx.xx.xx.xx" /path/to/redissms_receiver "$1" "$2"
