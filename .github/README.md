# SCI to AGI Demake (unofficial ScummVM fork) - v0.245

This fork adds an **AGI-style demake** mode to ScummVM's SCI engine. Early Sierra SCI games (SCI0 / SCI01, 16-colour EGA) are drawn the way Sierra's older AGI games looked: 160-pixel-wide "fat" pixels, no dithering, the 8x8 AGI font, AGI-style message boxes, and optional PCjr sound.

It is an unofficial, personal fork. It is not part of ScummVM and is not endorsed by the ScummVM team.

## Status

- **Tested:** Police Quest II: The Vengeance (DOS, EGA) and Hero's Quest / Quest for Glory I (DOS, EGA)
- **Option available but not yet tested:** every release (any platform) of the other SCI0 / SCI01 games (KQ1 SCI, KQ4, LSL2, LSL3, SQ3, QfG2, Codename: Iceman, Conquests of Camelot, The Colonel's Bequest, Hoyle 1 and 2, Mixed-Up Mother Goose, Fun Seeker's Guide, Christmas 1988), and the EGA releases of games that also came out in VGA
- The options show for these games even if they were added to the launcher before installing this fork
- Version numbering stays below 1.0 until more games are tested

## How to use it

1. Add the game in ScummVM as usual
2. Select it and click **Game Options...**
3. On the **Game** tab:
   - Tick **AGI-style demake graphics** (this also forces "Skip EGA dithering pass" on)
   - Optionally set **Input box** (pulldown under the AGI demake option): *AGI Type* (default) or *Pop-up type*
   - Optionally tick **Force AGI sound** for PCjr sound emulation (this switches off "Prefer digital sound effects")
4. Click **OK** and start the game

## What it changes (demake mode only)

- **Graphics:** every pixel pair is collapsed to one 2-pixel-wide AGI pixel, with rules to keep thin lines, outlines, noses, eyes and wheels intact
- **Sprites:** walking characters, wide sprites (cars), and dialog portraits each have tuned conversion rules. Characters facing you keep a one pixel gap between their eyes where the face has room
- **Text:** the 8x8 AGI font. Full-screen page text uses tight letter spacing so pages fit. Message boxes keep fixed AGI spacing
- **Interface:** AGI-style message boxes (red inset frame), status line, menus, buttons and a chunky mouse cursor
- **Game-specific fixes (PQ2):**
  - Lettering too small for AGI pixels is redrawn in the AGI font or by hand: radio labels, gun sight labels, file tabs, drawer plates, mugshot and personnel file placards, business cards, notes, signs and the bomb timer
  - Addresses and phone numbers on envelopes and Colby's card are added to their description text
  - Faces where the eyes merged, the shooting range targets, inventory item frames, and Sonny's head on his diving suits
  - The police computer screen, and a few text boxes re-wrapped to fit
  - Car park fence stays in front of the parked cars
  - Police station hallway walls (drawn as a grey and white checkerboard) shown white, with their edges and lines kept straight
- **Status line** (every game): written the AGI way, "Score: X out of Y" on the left and "Sound: on" or "Sound: off" on the right. Lines without a score are left as the game wrote them
- **Input box** (pulldown directly under the AGI demake option, games with a typed parser): *AGI Type* (default) puts the typed line on the bottom row of the screen with a `>` prompt and `_` cursor, like AGI, and the game keeps running while you type. The prompt stays on screen wherever the game takes typed commands (checked once per room by quietly offering the game a key it never uses), and is hidden on start-up and title screens (no status line). It stays through animations where the game briefly takes control, and you can keep typing (Enter waits until the game hands control back). While a message box is up the prompt stays and anything typed is cleared, as in AGI. Up to 38 characters. Backspace deletes, Esc clears the line, Enter sends it. Shortcuts that fill Sierra's box (QfG1 Ctrl+A "ask about", Ctrl+L "look at") fill the AGI line instead. While a message box is open, keys go to the box. *Pop-up type* keeps Sierra's own input box, which pauses the game. Takes effect next time the game starts
- **Dither handler** (pulldown directly under the AGI demake option, any game): *Flat approximation* (default) shows each dithered colour pair as one colour. *Chunky dither* draws it as a checkerboard of fat pixels in its two colours. Takes effect next time the game starts
- **Game-specific fixes (Hero's Quest, in progress):**
  - Title sign lettering: every line re-spaced with a hole in each letter
  - Intro: "PRESENTS" and "So You Want To Be A Hero" letters spaced apart. Credits and the character select lettering redrawn from a cleaned-up alphabet in Sierra's colours, still centred
  - New hero screen: frame all the way round, name box lined up, "Start" button, wider gap before the points values (also on the character sheet in game), "points to allocate" pop-up fits the screen
  - Hero (walking and standing): Sierra's 1 pixel grey outline dropped, eyes split and centred on his face
  - Selected button: its second frame line sits above the frame, clear of the text
  - Name box text centred in the box, "Start" centred over "Cancel"
  - Look-around at the start: turned head without the grey outline or a double row of hair
  - Town signs: "DRY GOODS" (second O fixed), "MART" (fruit stall), "INN" (Hero's Tale), "Sheriff". Katta's green eyes (behind the bar and the waitress), the hero sitting down (inn, and facing right in room 83) without the grey outline, stray red pixels at the Sheriff's steps
  - Town square ground shows Sierra's cobble lines in dark grey. Guild hall notice board moved to sit under a wider, readable "QUESTS" plate, saurus trophy readable. Ice man's red eyes
  - Fight screen HP, SP and MP labels readable. Graveyard trees green and ground reddish. Green meeps' fur and the clump they throw stand out from the grass
  - Centaur garden soil and Erana's Peace ground coloured from the originals. "You killed a ..." fight messages wrapped so they clear the enemy's HP box. Death messages fit on screen, with even gaps round the Restore, Restart and Quit buttons (and Quit / Don't Quit). The Baron's eyes apart. Thatched roofs yellow on the castle grounds. Gate guard's face, castle door guards' eyes, the man in the turban at the inn, the sign in room 28 with its white rim, and the hero raking (with his rake) without the grey outline. Inventory screen items moved left, the right column clear of the frame with the coin and weight rows closer, two word spells and long item names wrapped
  - Skull gate bars solid instead of striped, and the talking skull without the dark box round it
  - Town roads at night dark grey with the cobbles' black lines, shadows black. Knife throwing labels and turn highlights in fat letters. Otto's face (eyes, brows, tooth, symmetrical head, in town and in the Sheriff's house), Baba Yaga's eyes, Yorick's room (its signs and the fake door), Elsa's eyes and her room's torches, Yorick walking in, the brigand fortress (gate planks, courtyard colours, the dining hall sign, the brigands' eyes, the hero facing you there), the Sheriff's painting frame
  - Text lines 9 pixels apart so descenders don't touch the line below. Room sprites don't draw over open message boxes. Speech boxes widened to fit their longest word (Erasmus, Fenrus). Mage's Maze buttons and TIME box readable, ladders with both rails
  - "Spielburg" sign letters solid black. Brown+green foliage shows green. Path texture straying under a barrier stays path coloured
  - Flat approximation colours: brown+black and green+dark grey show dark grey, light green+green shows light green, cyan+black shows cyan. On walkable ground green+dark grey shows green and green+black shows dark grey. Off it (including anything he walks behind) red+black shows red, brown+black shows brown and light grey+brown shows light grey

## Diagnostics for adding games

To help tune a new game, add this line to the game's section of `scummvm.ini`:

```
agi_demake_debug=true
```

ScummVM then writes `agi_demake_dump.txt` (sprites: source, result, screen position) and `agi_demake_text.txt` (text boxes and windows) and `agi_demake_pic.txt` (each room picture as drawn, before sprites, plus its depth-priority and walkable-area screens) next to the executable, overwritten each session. Leave it off for normal play.

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
