#!/bin/sh
# Plays a test sequence through the default output device, records what
# reaches it (the device's monitor), and checks the recording: the scale's
# length and pitch, a sample-by-sample match, and that play() and a
# background sound are mixed. See speaker_test.cpp for the sequence.
#
# Usage: tests/manual/speaker_test.sh path/to/speaker_test
#   (build it with: cmake --build build --target speaker_test)
#
# Needs PipeWire or PulseAudio, with pactl and parecord. You will hear about
# 3.5 seconds of tones. The recording is taken from the output stream, so the
# test passes even if the speakers are muted; listen to check them too.
set -u

bin=${1:?usage: $0 path/to/speaker_test}
command -v pactl >/dev/null && command -v parecord >/dev/null ||
    { echo "speaker_test: needs pactl and parecord (PipeWire or PulseAudio)"; exit 1; }

sink=$(pactl get-default-sink) || { echo "speaker_test: no default output device"; exit 1; }
echo "output device: $sink"
[ "$(pactl get-sink-mute "$sink")" = "Mute: yes" ] &&
    echo "note: the output is muted, so you won't hear the test (it can still pass)"

recording=$(mktemp --suffix=.wav)
# A low latency keeps parecord's buffer small: whatever is still buffered
# when it is stopped is lost.
parecord --latency-msec=20 -d "$sink.monitor" --file-format=wav --channels=1 --rate=44100 \
    "$recording" &
recorder=$!
# Background jobs ignore SIGINT in a non-interactive shell, so stop with TERM.
trap 'kill -TERM $recorder 2>/dev/null; rm -f "$recording"' EXIT

sleep 0.5
"$bin" play || exit 1
sleep 1
kill -TERM $recorder
wait $recorder 2>/dev/null

"$bin" check "$recording"
