# SCI to AGI Demake (unofficial ScummVM fork) - v0.312

This fork adds an **AGI-style demake** mode to ScummVM's SCI engine. Early Sierra SCI games (SCI0 / SCI01, 16-colour EGA) are drawn the way Sierra's older AGI games looked: 160-pixel-wide "fat" pixels, no dithering, the 8x8 AGI font, AGI-style message boxes, and optional PCjr sound.

It is an unofficial, personal fork. It is not part of ScummVM and is not endorsed by the ScummVM team.

## Status

- **Tested:** Police Quest II: The Vengeance, Hero's Quest / Quest for Glory I, and Space Quest III: The Pirates of Pestulon (all DOS, EGA)
- **Option available but not yet tested:** the other SCI0 / SCI01 games (KQ1 SCI, KQ4, LSL2, LSL3, QfG2, Codename: Iceman, Conquests of Camelot, The Colonel's Bequest, Hoyle 1 and 2, Mixed-Up Mother Goose, Fun Seeker's Guide, Christmas 1988), and the EGA releases of games that also came out in VGA
- Version numbering stays below 1.0 until more games are tested

## How to use it

1. Add the game in ScummVM as usual
2. Select it and click **Game Options...**
3. On the **Game** tab:
   - Tick **AGI-style demake graphics** (this also forces "Skip EGA dithering pass" on)
   - Optionally set **Input box**: *AGI Type* (default) or *Pop-up type*
   - Optionally set **Dither handler**: *Flat approximation* (default) or *Chunky dither*
   - Optionally tick **Force AGI sound** for PCjr sound emulation
4. Click **OK** and start the game

## What it changes (demake mode only)

- **Graphics:** every pixel pair is collapsed to one 2-pixel-wide AGI pixel, with rules to keep thin lines, outlines, eyes and faces intact
- **Text:** the 8x8 AGI font, AGI-style message boxes (red inset frame), status line ("Score: X out of Y" / "Sound: on"), menus, buttons and a chunky mouse cursor
- **Input box:** *AGI Type* puts the typed line on the bottom row with a `>` prompt, like AGI, and the game keeps running while you type. *Pop-up type* keeps Sierra's own box, which pauses the game
- **Dither handler:** *Flat approximation* shows each dithered colour pair as one colour. *Chunky dither* draws it as a checkerboard of fat pixels

## Game-specific fixes

- **Police Quest II:** small lettering (radio, files, cards, signs, bomb timer) redrawn in the AGI font or by hand, plus faces, targets, the police computer and a few re-wrapped text boxes
- **Hero's Quest:** title, intro and credits lettering redrawn, the hero and other characters' faces and outlines tidied, town signs and fight screen labels readable, and text boxes and buttons laid out to fit
- **Space Quest III:** every speech bubble redrawn in the AGI font, the ship's computer, navigation and weapons screens relabelled, Astro Chicken and the robot fight fitted to AGI, Roger's sprites and the characters' faces tidied, and typing kept out of screens where it doesn't belong

## Diagnostics for adding games

To help tune a new game, add this line to the game's section of `scummvm.ini`:

```
agi_demake_debug=true
```

ScummVM then writes `agi_demake_dump.txt` (sprites), `agi_demake_text.txt` (text boxes and windows) and `agi_demake_pic.txt` (room pictures) next to the executable, overwritten each session. Leave it off for normal play.

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
