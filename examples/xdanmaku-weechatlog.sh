#!/bin/sh

# Watches a WeeChat log file and pipes each new line written to it
# to XDanmaku's standard input stream.

# usage: xdanmaku-weechatlog.sh [SERVER [CHANNEL]]

# https://weechat.org/
# https://github.com/weechat/weechat
# https://weechat.org/files/doc/weechat/stable/weechat_user.en.html#buffer_logging
# https://blog.jeaye.com/2016/10/31/weechat-logs/

# Before proceeding you may want to make some WeeChat configuration changes:
# (Replace `#soranowoto' with any channel name.)
#
#         /set logger.level.irc.rizon.#soranowoto 3
#         /set logger.file.flush_delay = 0
#         /save
#
# Logger level 3 is for server level messages, excluding join, part, and quit
# messages. Flush delay is specified in whole seconds and, if enabled, would
# introduce undesirable delay in detecting new log messages before they are
# piped to XDanmaku. Setting it to 0 causes immediate writes for each message
# line. Unfortunately, I don't think `file.flush_delay' can be configured on a
# per-buffer basis.

prog="xdanmaku"
home="${WEECHAT_HOME:-${XDG_CONFIG_HOME:-$HOME/.config}/weechat}"
server="${1:-rizon}"
channel="${2:-#soranowoto}"
logs="$home/logs/irc.$server.$channel.weechatlog"

if ! test -f $logs; then echo "${0##*/}: $logs: No such file or directory"; exit 2; fi

if   command -v $prog; then bin="$prog"
elif test -x  ./$prog; then bin="./$prog"
elif test -x ../$prog; then bin="../$prog"
else echo "${0##*/}: $prog: Failed to locate the executable"; exit 1
fi

tail -n1 -f $logs | while read line; do echo "$line" | cut -f3-; done | $bin

# The while loop appears to be necessary due to `cut' buffering its output.
# https://stackoverflow.com/a/14362162
# I am unfamiliar with an option within WeeChat for specifying the format of
# logged lines, but if such a feature were available you could forgo cutting
# the output of `tail' altogether. In that case, the command would become:
#
#         tail -n1 -f $logs | $bin

# The `logs' variable format assumes `logger.file.mask' has the default format
# value of "$plugin.$name.weechatlog".
# https://www.weechat.org/files/doc/stable/weechat_user.en.html#option_logger.file.mask

# The `home' variable uses the default and fallback value logic as used by
# WeeChat itself. If your WeeChat config directory is located somewhere else,
# such as ~/.weechat, then you must either change the value of the `home'
# variable manually or set the WEECHAT_HOME environment variable to the
# appropriate value. For example, you might run:
#
#         WEECHAT_HOME=~/.weechat ./xdanmaku-weechatlog.sh
#
# https://www.weechat.org/files/doc/stable/weechat_user.en.html#xdg_directories
