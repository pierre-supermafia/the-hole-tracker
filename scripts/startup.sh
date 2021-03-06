#!/bin/bash

DIR=`dirname "$0"`

# Reset log
echo -n > $DIR/log

if [ ! -z "$(xrandr --query | grep 'DP-1 disconnected')" ]
then
	# No active display, running through xvfb
	xvfb-run $DIR/tracker.sh
else
	# Screen connected, running directly
	$DIR/tracker.sh
fi

