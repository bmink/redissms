#!/bin/bash
#
# Install this script in crontab to run once per minute.
#

if [[ ! $(pidof redissms_sender) ]]; then
	logger "Starting redissms_sender"
	REDIS_ADDR="192.168.1.128" \
		nohup /home/bmink/devel/redissms/redissms_sender 2>&1 >/dev/null &
fi

