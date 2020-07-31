#!/bin/bash
#
# Install this script in crontab to run every minute.
#

if [[ ! $(pidof redissms_sender) ]]; then
	logger "Starting redissms_sender"
	REDIS_ADDR="xx.xx.xx.xx" \
		nohup /path/to/redissms_sender 2>&1 >/dev/null &
fi

