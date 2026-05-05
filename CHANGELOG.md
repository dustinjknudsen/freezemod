# Changelog

## v0.3.0 — Phase 3: Core Freeze Tag Loop

Forked from [themuffinator/muffmode](https://github.com/themuffinator/muffmode) (GPL-3.0).
Cross-compiled to Windows DLL via MinGW targeting Quake II Rerelease.

### Features
- Freeze on death: players freeze in place on fatal damage with randomised death animation and ice shell
- Thaw by teammate: living teammates thaw frozen players by standing adjacent for 3 seconds; awards a point to the thawer
- Frozen body push: enemy team contact shoves frozen bodies
- Round end: round ends when all players on one team are frozen; surviving team scores a point
- Capturelimit: match ends and rotates map when a team reaches the round limit
- Chasecam during freeze: frozen players can use `[`/`]` to follow living players; exiting chasecam snaps view back to the frozen body position
- Ghost anchor entity: frozen body position tracked by an invisible ghost entity so teammate thaw proximity is correct even when the player entity has been moved by chasecam

### Integration fixes
- Frozen players cannot pick up items
- Chasecam does not auto-thaw the frozen player
- No phantom thaw sound on chasecam exit
- Attacker center-screen "You froze X" message suppressed in freeze tag (broadcast kill line is sufficient)

### Known gaps (deferred to Phase 5)
- Thaw visual flash during active thaw not yet implemented
- Ice gib colors, screen tint, drop animation, ice shards deferred
- Bot AI does not actively seek frozen teammates to thaw

## v0.1.0 — Initial fork

Forked muffmode; established build system (Makefile + MinGW cross-compile), added `g_freeze.cpp` / `g_freeze.h` scaffold, wired `GT_FREEZE` gametype.
