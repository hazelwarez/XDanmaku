#!/bin/bash

prog="xdanmaku"

if   command -v $prog; then bin="$prog"
elif test -x  ./$prog; then bin="./$prog"
elif test -x ../$prog; then bin="../$prog"
else echo "${0##*/}: $prog: Failed to locate the executable"; exit 1
fi

# First, play a video and attach an IPC socket to the process:
# mpv --input-ipc-server=/tmp/mpvsocket input.mkv
#
# https://unix.stackexchange.com/q/455183/
# https://github.com/christoph-heinrich/mpv-subtitle-lines

LAST_MPV_SUBTEXT=""
while true; do
	# Only fetch subtitle text output when the video is not paused
	if [[ ! "$(echo '{ "command": ["get_property", "pause"] }' | socat - /tmp/mpvsocket | jq --raw-output '.["data"]')" =~ true ]]; then
		MPV_SUBTEXT=$(echo '{ "command": ["get_property", "sub-text"] }' | socat - /tmp/mpvsocket | jq --raw-output '.["data"]')
		# Replace newlines in the text with spaces
		MPV_SUBTEXT="${MPV_SUBTEXT//$'\n'/ }"
		# Do not feed the line into XDanmaku if the text hasn't changed
		# NOTE: this is naive and may skip valid repeated lines
		if [ "$MPV_SUBTEXT" != "$LAST_MPV_SUBTEXT" ]; then
			if [ -n "$MPV_SUBTEXT" ]; then
				echo "$MPV_SUBTEXT"
			fi
		fi
		LAST_MPV_SUBTEXT="$MPV_SUBTEXT"
	fi
	sleep 0.5
# done
done | $bin ${XDANMAKU_FLAGS}
