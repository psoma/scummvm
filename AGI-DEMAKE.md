# SCI to AGI Demake (unofficial ScummVM fork) - v0.159

This fork adds an **AGI-style demake** mode to ScummVM's SCI engine. Early Sierra SCI games (SCI0 / SCI01, 16-colour EGA) are drawn the way Sierra's older AGI games looked: 160-pixel-wide "fat" pixels, no dithering, the 8x8 AGI font, AGI-style message boxes, and optional PCjr sound.

It is an unofficial, personal fork. It is not part of ScummVM and is not endorsed by the ScummVM team.

## Status

- **Tested:** Police Quest II: The Vengeance (DOS, EGA)
- **Option available but not yet tested:** Space Quest III and other SCI0 / SCI01 EGA games
- Version numbering stays below 1.0 until more games are tested

## How to use it

1. Add the game in ScummVM as usual
2. Select it and click **Game Options...**
3. On the **Game** tab:
   - Tick **AGI-style demake graphics** (this also forces "Skip EGA dithering pass" on)
   - Optionally tick **Force AGI sound** for PCjr sound emulation (this switches off "Prefer digital sound effects")
4. Click **OK** and start the game

## What it changes (demake mode only)

- **Graphics:** every pixel pair is collapsed to one 2-pixel-wide AGI pixel, with rules to keep thin lines, outlines, noses, eyes and wheels intact
- **Sprites:** walking characters, wide sprites (cars), and dialog portraits each have tuned conversion rules. Characters facing you keep a one pixel gap between their eyes where the face has room
- **Text:** the 8x8 AGI font. Full-screen page text uses tight letter spacing so pages fit. Message boxes keep fixed AGI spacing
- **Interface:** AGI-style message boxes (red inset frame), status line, menus, buttons and a chunky mouse cursor
- **Game-specific fixes (PQ2):** painted-in lettering redrawn in the AGI font (radio labels, gun sight labels, file tabs, drawer plate, mugshot numbers, surname on the personnel file photo, mugshot numbers on the inventory mugshots, scuba tank 1, the handwritten number on the back of the business card), inventory item frames whole on both sides wherever the item sits on screen, Sonny's normal head on his diving suits, and the police computer screen

## Diagnostics for adding games

In this test build (v0.159) the diagnostics are always on, no `scummvm.ini` setting needed. This is temporary and will go back to opt-in before the next public release.

ScummVM writes `agi_demake_dump.txt` (sprites) and `agi_demake_text.txt` (text) next to the executable, overwritten each session.

## Building

Build ScummVM as normal (see the main README). To build only the SCI engine:

```
./configure --disable-all-engines --enable-engine=sci
make
```

The changes are all under `engines/sci/` on the `agi-demake` branch.

## Licence

ScummVM is licensed under the GNU GPL (see COPYING). This fork is distributed under the same licence. The source for each release is the `agi-demake` branch at the matching tag.

## AI assistance

This code was written with substantial assistance from an AI model (Anthropic's Claude), directed and tested by the repository owner. In line with ScummVM's contribution guidelines, it is not intended to be submitted to the official ScummVM project.

## No game data

No Sierra game files are included. You need your own copies of the games.
