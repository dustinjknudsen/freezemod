# Freeze Tag Server-Side Mod — Spec

## Source
Feedback from Ozy and Discord testing of Enhanced Edition (Feb 2026).
Goal: server-side mod, vanilla rerelease clients, deploys to baseq2.

## Required features
- 2-team only (Red/Blue) — drop 4-team support
- TDM-style HUD team indicator (Red/Blue tag images)
- Thaw in place — `thaw_in_place 1` cvar option to respawn at frozen location
- Mouse1 cycles chasecam (alongside `[`/`]`)
- Eyecam mode — first-person spectating through teammate
- Working +hook for vanilla clients (use BUTTON_HOLSTER, no client config needed)
- Scoreboard properly delivered to vanilla clients
- 3-2-1 countdown before new rounds (Clan Arena style)

## Architecture
- Fork muffmode (themuffinator/muffmode, GPL-3.0)
- Deploy to baseq2/ replacing game_x64.dll
- 2-team only
- Use muffmode's match state machine, EyeCam, voting, HUD infrastructure
- Port own freeze logic and hook mechanics from Enhanced Edition

## Out of scope (initially)
- 4-team support
- Mission pack content (rogue/, xatrix/)
- Other muffmode gametypes (Horde, Duel, CA, etc.)
