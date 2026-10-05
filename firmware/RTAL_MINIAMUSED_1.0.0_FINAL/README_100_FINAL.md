# RTAL MiniAmused 1.0.0 FINAL

Release build derived directly from the successfully tested 0.9.0 RELEASE-CANDIDATE1.

## Release policy
- Feature freeze retained.
- No synthesizer, filter, envelope or delay DSP changes from RC1.
- No MIDI routing, CC mapping, clock or transport changes from RC1.
- No preset format or preset recall behavior changes from RC1.
- Runtime/startup version labels cleaned for 1.0.0 FINAL.

## Confirmed release behavior
- 128 preset slots: 000..127.
- MIDI Program Change 0..127 maps directly to preset 000..127.
- Persistent MIDI receive channel: OMNI or CH1..CH16.
- System realtime MIDI clock/transport remains independent of channel filtering.
- OVL TIME range: 1..5 seconds; factory/default value: 2 seconds.
- Six-page LVGL 8.4 UI retained.
- Delay FREE/SYNC, divisions, legacy WIDTH, ping-pong modes and true stereo width retained.

This is the release candidate code promoted to FINAL without functional changes.
