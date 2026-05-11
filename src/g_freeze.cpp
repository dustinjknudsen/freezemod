// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "g_freeze.h"
#include "monsters/m_player.h"

freeze_team_t  freeze[2];
int            gib_queue;
/*freeze*/
thaw_record_t  thaw_records[MAX_CLIENTS_KEX];
/*freeze*/

// Maps muffmode team_t (TEAM_RED=3, TEAM_BLUE=4) to freeze[] index (0, 1).
static inline int freeze_idx(team_t t) {
	return t == TEAM_RED ? 0 : 1;
}

// Reverse: freeze index (0/1) → team_t
static inline team_t freeze_team_t_from_idx(int i) {
	return i == 0 ? TEAM_RED : TEAM_BLUE;
}

static int moan[8];
static cvar_t *ft_start_weapon;
static cvar_t *ft_start_armor;

// Bitmask for "thawer proximity hint shown" in client_respawn_t::help.
static constexpr int thaw_help = bit_v<0>;


// ============================================================
// Initialization
// ============================================================

void freezeSpawn() {
	freeze[0] = freeze[1] = freeze_team_t{};
	for (int i = 0; i < 2; i++) {
		freeze[i].update = true;
		freeze[i].last_update = level.time;
	}
	gib_queue = 0;

	moan[0] = gi.soundindex("insane/insane1.wav");
	moan[1] = gi.soundindex("insane/insane2.wav");
	moan[2] = gi.soundindex("insane/insane3.wav");
	moan[3] = gi.soundindex("insane/insane4.wav");
	moan[4] = gi.soundindex("insane/insane6.wav");
	moan[5] = gi.soundindex("insane/insane8.wav");
	moan[6] = gi.soundindex("insane/insane9.wav");
	moan[7] = gi.soundindex("insane/insane10.wav");

	gi.configstring(CS_GENERAL + 5, ">");

	ft_start_weapon = gi.cvar("ft_start_weapon", "0", CVAR_NOFLAGS);
	ft_start_armor  = gi.cvar("ft_start_armor",  "0", CVAR_NOFLAGS);
}

// ============================================================
// Gib helpers
// ============================================================

bool gibCheck() {
	if (gib_queue > 35)
		return true;
	gib_queue++;
	return false;
}

THINK(gibThink) (gentity_t *self) -> void {
	gib_queue--;
	G_FreeEntity(self);
}

// ============================================================
// Damage intercept (wired to T_Damage in Phase 3)
// ============================================================

bool playerDamage(gentity_t *targ, gentity_t *attacker, int damage, mod_t mod) {
	if (!targ->client)
		return false;
	if (mod.id == MOD_TELEFRAG)
		return false;
	if (!attacker->client)
		return false;
	if (targ->health > 0) {
		if (targ == attacker)
			return false;
		if (targ->client->sess.team != attacker->client->sess.team &&
		    targ->client->respawn_time + 3_sec < level.time)
			return false;
	} else {
		if (targ->client->frozen) {
			if (frandom() < 0.1f)
				ThrowGib(targ, "models/objects/debris2/tris.md2", damage, GIB_NONE, 1.f);
			return true;
		} else
			return false;
	}
	if (g_friendly_fire->integer)
		return true;
	return false;
}

bool freezeCheck(gentity_t *ent, mod_t mod) {
	if (ent->deadflag)
		return false;
	if (IsCombatDisabled())
		return false;
	if (mod.friendly_fire)
		return false;
	switch (mod.id) {
	case MOD_SUICIDE:
	case MOD_EXIT:
	case MOD_BFG_LASER:
	case MOD_BFG_EFFECT:
	case MOD_TELEFRAG:
	case MOD_TELEFRAG_SPAWN:
	case MOD_NUKE:
		return false;
	default:
		break;
	}
	return true;
}

// ============================================================
// Freeze animation and state
// ============================================================

void freezeAnim(gentity_t *ent) {
	ent->client->anim_priority = ANIM_DEATH;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		if (rand() & 1) {
			ent->s.frame = FRAME_crpain1 - 1;
			ent->client->anim_end = FRAME_crpain1 + rand() % 4;
		} else {
			ent->s.frame = FRAME_crdeath1 - 1;
			ent->client->anim_end = FRAME_crdeath1 + rand() % 5;
		}
	} else {
		switch (rand() % 8) {
		case 0:
			ent->s.frame = FRAME_run1 - 1;
			ent->client->anim_end = FRAME_run1 + rand() % 6;
			break;
		case 1:
			ent->s.frame = FRAME_pain101 - 1;
			ent->client->anim_end = FRAME_pain101 + rand() % 4;
			break;
		case 2:
			ent->s.frame = FRAME_pain201 - 1;
			ent->client->anim_end = FRAME_pain201 + rand() % 4;
			break;
		case 3:
			ent->s.frame = FRAME_pain301 - 1;
			ent->client->anim_end = FRAME_pain301 + rand() % 4;
			break;
		case 4:
			ent->s.frame = FRAME_jump1 - 1;
			ent->client->anim_end = FRAME_jump1 + rand() % 6;
			break;
		case 5:
			ent->s.frame = FRAME_death101 - 1;
			ent->client->anim_end = FRAME_death101 + rand() % 6;
			break;
		case 6:
			ent->s.frame = FRAME_death201 - 1;
			ent->client->anim_end = FRAME_death201 + rand() % 6;
			break;
		case 7:
			ent->s.frame = FRAME_death301 - 1;
			ent->client->anim_end = FRAME_death301 + rand() % 6;
			break;
		}
	}

	if (frandom() < 0.2f)
		gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("boss3/d_hit.wav"), 1, ATTN_NORM, 0);

	Weapon_Grapple_DoReset(ent->client);
	ent->client->frozen = true;
	ent->client->frozen_time = level.time + gtime_t::from_sec(g_frozen_time->value);
	ent->client->resp.thawer = nullptr;
	ent->client->thaw_time = HOLD_FOREVER;
	ent->deadflag = true;
	ent->solid = SOLID_NOT;
	ent->svflags |= SVF_DEADMONSTER;
	gi.linkentity(ent);
	P_AssignClientSkinnum(ent); // poi_icon = 1 (X indicator) for frozen teammates
	gi.configstring(CONFIG_CHASE_PLAYER_NAME + (ent - g_entities - 1),
		G_Fmt("{} (frozen)", ent->client->pers.netname).data());
	// Ghost is solid (SOLID_BBOX, NOCLIENT) — acts as a team-neutral physical obstacle.
	// The engine skips same-team SVF_PLAYER collision; the ghost has no team encoding.
	CreateFrozenBodyGhost(ent);
}

// ============================================================
// Per-frame mechanics
// ============================================================

void playerView(gentity_t *ent) {
	if ((level.time.milliseconds() / 100) % 8)
		return;

	float other_dot = 0.3f;
	gentity_t *best_other = nullptr;
	vec3_t ent_origin = ent->s.origin;
	ent_origin[2] += ent->viewheight;

	vec3_t forward;
	AngleVectors(ent->s.angles, forward, nullptr, nullptr);

	for (uint32_t i = 0; i < (uint32_t)game.maxclients; i++) {
		gentity_t *other = g_entities + 1 + i;
		if (!other->inuse) continue;
		if (other->client->resp.spectator) continue;
		if (other == ent) continue;
		if (other->health <= 0 && !other->client->frozen) continue;

		vec3_t other_origin = other->s.origin;
		other_origin[2] += other->viewheight;

		vec3_t dist = other_origin - ent_origin;
		if (dist.length() > 800) continue;

		trace_t trace = gi.trace(ent_origin, vec3_origin, vec3_origin, other_origin, ent, MASK_OPAQUE);
		if (trace.fraction != 1) continue;

		dist.normalize();
		float dot = dist.dot(forward);
		if (dot > other_dot) {
			other_dot = dot;
			best_other = other;
		}
	}

	ent->client->viewed = best_other;
}

void playerThaw(gentity_t *ent) {
	const vec3_t &body_origin = ent->client->frozen_body
		? ent->client->frozen_body->s.origin
		: ent->s.origin;

	// Sticky thawer: if current thawer is still alive and in range, keep them.
	gentity_t *cur = ent->client->resp.thawer;
	if (cur && cur->inuse && cur->health > 0 && !cur->client->frozen) {
		vec3_t eorg;
		for (int j = 0; j < 3; j++)
			eorg[j] = body_origin[j] - (cur->s.origin[j] + (cur->mins[j] + cur->maxs[j]) * 0.5f);
		if (eorg.length() <= MELEE_DISTANCE + 32)
			return;
	}

	for (uint32_t i = 0; i < (uint32_t)game.maxclients; i++) {
		gentity_t *other = g_entities + 1 + i;
		if (!other->inuse) continue;
		if (other->client->resp.spectator) continue;
		if (other == ent) continue;
		if (other->health <= 0) continue;
		if (other->client->frozen) continue;
		if (other->client->sess.team != ent->client->sess.team) continue;
		// Respawn grace: players within 2 seconds of spawning can't steal a thaw.
		// if (level.time < other->client->pers.last_spawn_time + 2_sec) continue;

		vec3_t eorg;
		for (int j = 0; j < 3; j++)
			eorg[j] = body_origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5f);
		if (eorg.length() > MELEE_DISTANCE + 32)
			continue;

		if (!(other->client->resp.help & thaw_help)) {
			other->client->showscores = false;
			other->client->resp.help |= thaw_help;
			gi.LocCenter_Print(other, "Wait here a second to free them.");
			gi.sound(other, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
		}

		ent->client->resp.thawer = other;
		if (ent->client->thaw_time == HOLD_FOREVER) {
			ent->client->thaw_time = level.time + 3_sec;
			gi.sound(ent, CHAN_BODY, gi.soundindex("world/steam3.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}
	ent->client->resp.thawer = nullptr;
	ent->client->thaw_time = HOLD_FOREVER;
}

void playerBreak(gentity_t *ent, int force) {
	/*freeze*/
	// In chasecam modes, ent->s.origin is at the camera position (near the followed player).
	// Snap back to the actual freeze spot so break effects spawn at the right location.
	if (ent->client->frozen_body) {
		ent->s.origin = ent->client->frozen_body->s.origin;
		gi.linkentity(ent);
	}
	/*freeze*/

	ent->client->respawn_time = level.time + 1_sec;

	if (ent->waterlevel == 3)
		gi.sound(ent, CHAN_BODY, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("world/brkglas.wav"), 1, ATTN_NORM, 0);

	int n = rand() % (gib_queue > 10 ? 5 : 3);
	if (rand() & 1) {
		switch (n) {
		case 0: ThrowGib(ent, "models/objects/gibs/arm/tris.md2",   force, GIB_NONE, 1.f); break;
		case 1: ThrowGib(ent, "models/objects/gibs/bone/tris.md2",  force, GIB_NONE, 1.f); break;
		case 2: ThrowGib(ent, "models/objects/gibs/bone2/tris.md2", force, GIB_NONE, 1.f); break;
		case 3: ThrowGib(ent, "models/objects/gibs/chest/tris.md2", force, GIB_NONE, 1.f); break;
		case 4: ThrowGib(ent, "models/objects/gibs/leg/tris.md2",   force, GIB_NONE, 1.f); break;
		}
	}
	while (n--)
		ThrowGib(ent, "models/objects/debris1/tris.md2", force, GIB_NONE, 1.f);

	ent->takedamage = false;
	ent->movetype = MOVETYPE_TOSS;
	ThrowClientHead(ent, force);

	ent->client->frozen = false;
	ent->client->eliminated = false;
	ent->client->freeze_chase_mode = 0;
	if (ent->client->follow_target)
		FreeFollower(ent);
	freeze[freeze_idx(ent->client->sess.team)].update = true;
	gi.configstring(CONFIG_CHASE_PLAYER_NAME + (ent - g_entities - 1),
		ent->client->pers.netname);
	ent->client->ps.stats[STAT_CHASE] = 0;
	/*freeze*/
	{
		int slot = (int)(ent - g_entities) - 1;
		if (slot >= 0 && slot < (int)MAX_CLIENTS_KEX) {
			if (!thaw_records[slot].round_end_break) {
				thaw_records[slot].origin = ent->s.origin;
				thaw_records[slot].in_place = true;
			}
			thaw_records[slot].round_end_break = false;
		}
	}
	/*freeze*/
	RemoveFrozenBodyGhost(ent);
	ent->svflags &= ~SVF_NOCLIENT;
}

void playerUnfreeze(gentity_t *ent) {
	if (level.time > ent->client->frozen_time && level.time > ent->client->respawn_time) {
		playerBreak(ent, 50);
		return;
	}
	/*freeze*/
	if (ent->waterlevel > 0) {
		if (ent->watertype & CONTENTS_LAVA) {
			// Lava: cap remaining timer to 1 second (frame-rate independent)
			gtime_t lava_limit = level.time + 1_sec;
			if (ent->client->frozen_time > lava_limit)
				ent->client->frozen_time = lava_limit;
		} else if (ent->watertype & CONTENTS_SLIME) {
			// Slime: cap remaining timer to 3 seconds
			gtime_t slime_limit = level.time + 3_sec;
			if (ent->client->frozen_time > slime_limit)
				ent->client->frozen_time = slime_limit;
		} else if (ent->waterlevel == 3 && !((level.time.milliseconds() / 100) % 4)) {
			// Water: existing 150ms/frame acceleration
			ent->client->frozen_time -= 150_ms;
		}
	}
	/*freeze*/

	if (level.time > ent->client->thaw_time) {
		gentity_t *thawer = ent->client->resp.thawer;
		if (!thawer || !thawer->inuse || thawer->client->frozen || thawer->health <= 0) {
			ent->client->resp.thawer = nullptr;
			ent->client->thaw_time = HOLD_FOREVER;
		} else {
			if (!IsScoringDisabled()) {
				thawer->client->resp.score++;
				thawer->client->resp.thawed++;
				freeze[freeze_idx(ent->client->sess.team)].thawed++;
			}
			if (rand() & 1)
				gi.LocBroadcast_Print(PRINT_HIGH, "{} thaws {} like a package of frozen peas.\n",
					thawer->client->resp.netname, ent->client->resp.netname);
			else
				gi.LocBroadcast_Print(PRINT_HIGH, "{} evicts {} from their igloo.\n",
					thawer->client->resp.netname, ent->client->resp.netname);
			thawer->client->pers.thaw_protect_time = level.time + 2_sec;
			playerBreak(ent, 100);
		}
	}
}

void playerMove(gentity_t *ent) {
	if (!ClientIsPlaying(ent->client) || ent->client->resp.spectator)
		return;

	vec3_t forward;
	AngleVectors(ent->s.angles, forward, nullptr, nullptr);

	for (uint32_t i = 0; i < (uint32_t)game.maxclients; i++) {
		gentity_t *other = g_entities + 1 + i;
		if (!other->inuse) continue;
		if (other->client->resp.spectator) continue;
		if (other == ent) continue;
		if (!other->client->frozen) continue;
		if (other->client->sess.team == ent->client->sess.team) continue;

		vec3_t eorg;
		for (int j = 0; j < 3; j++)
			eorg[j] = ent->s.origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5f);
		if (eorg.length() > MELEE_DISTANCE) continue;

		other->velocity = forward * 600.f;
		other->velocity[2] = 200;
		gi.linkentity(other);
	}
}

void freezeMain(gentity_t *ent) {
	if (!ent->inuse)
		return;
	playerView(ent);
	if (ent->client->resp.spectator)
		return;
	if (ent->client->frozen) {
		// Bots auto-moan; human players moan on fire press (handled in the fire handler).
		if (ent->svflags & SVF_BOT)
			cmdMoan(ent);
		playerThaw(ent);
		playerUnfreeze(ent);
		freezeEffects(ent);
		if (ent->client->follow_target) {
			UpdateChaseCam(ent);
		} else if (ent->movetype == MOVETYPE_FREECAM) {
			// follow_target was cleared externally (FreeClientFollowers); auto-switch or snap to body
			GetFollowTarget(ent);
			if (!ent->client->follow_target && ent->client->frozen_body) {
				ent->s.origin = ent->client->frozen_body->s.origin;
				ent->movetype = MOVETYPE_NONE;
				gi.linkentity(ent);
			}
		} else {
			// At physical position: sync ghost so proximity checks and thaw-in-place
			// track where the body actually is (e.g. after being pushed).
			UpdateFrozenBodyGhost(ent);
		}
	} else if (ent->health > 0)
		playerMove(ent);
}

// ============================================================
// Ghost entity (frozen body spectator target)
// ============================================================

static THINK(FrozenBodyGhostThink) (gentity_t *ghost) -> void {
	ghost->nextthink = level.time + 100_ms;

	gentity_t *owner = (gentity_t *)ghost->owner;
	if (!owner || !owner->inuse || !owner->client || !owner->client->frozen ||
		owner->client->frozen_body != ghost) {
		G_FreeEntity(ghost);
		return;
	}

	// Detect crush: bounding box overlapping solid world geometry
	trace_t tr = gi.trace(ghost->s.origin, ghost->mins, ghost->maxs, ghost->s.origin, ghost, MASK_SOLID);
	if (tr.startsolid) {
		if (ghost->timestamp == 0_ms)
			ghost->timestamp = level.time;
		else if (level.time - ghost->timestamp > 3_sec)
			playerBreak(owner, 100);
	} else {
		ghost->timestamp = 0_ms;
	}
}

void CreateFrozenBodyGhost(gentity_t *ent) {
	if (ent->client->frozen_body || !ent->client->frozen)
		return;

	gentity_t *ghost = G_Spawn();
	if (!ghost)
		return;

	ghost->s = ent->s;
	ghost->s.number = ghost - g_entities;
	ghost->classname = "frozen_body_ghost";
	ghost->svflags = SVF_NOCLIENT;
	ghost->solid = SOLID_BBOX;
	ghost->movetype = MOVETYPE_NONE;
	ghost->takedamage = false;
	ghost->owner = ent;
	ghost->s.origin = ent->s.origin;
	// Always use standing player dims — ent->mins/maxs are corrupted by
	// player_die (maxs[2]=-8) and subsequent pmove (maxs[2]=4 for PM_DEAD).
	ghost->mins = PLAYER_MINS;
	ghost->maxs = PLAYER_MAXS;
	ghost->think = FrozenBodyGhostThink;
	ghost->nextthink = level.time + 100_ms;
	gi.linkentity(ghost);

	ent->client->frozen_body = ghost;
}

void UpdateFrozenBodyGhost(gentity_t *ent) {
	if (!ent->client->frozen_body)
		return;
	gentity_t *ghost = ent->client->frozen_body;
	ghost->s.origin   = ent->s.origin;
	ghost->s.angles   = ent->s.angles;
	ghost->s.frame    = ent->s.frame;
	ghost->s.skinnum  = ent->s.skinnum;
	ghost->s.effects  = ent->s.effects;
	ghost->s.renderfx = ent->s.renderfx;
	ghost->s.modelindex = ent->s.modelindex;
	gi.linkentity(ghost);
}

void RemoveFrozenBodyGhost(gentity_t *ent) {
	if (!ent->client->frozen_body)
		return;
	G_FreeEntity(ent->client->frozen_body);
	ent->client->frozen_body = nullptr;
}

// ============================================================
// Visual effects
// ============================================================

void playerShell(gentity_t *ent) {
	ent->s.effects |= EF_COLOR_SHELL;
	if (ent->client->sess.team == TEAM_RED)
		ent->s.renderfx |= RF_SHELL_RED;
	else
		ent->s.renderfx |= RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE; // white for Blue
}

bool powerupBlinkVisible() {
	return ((level.time.milliseconds() / 100) % 3) == 0;
}

void freezeEffects(gentity_t *ent) {
	if (level.intermission_time != gtime_t{})
		return;
	if (!ent->client->frozen)
		return;
	if (!ent->client->resp.thawer || ((level.time.milliseconds() / 100) % 16) < 8) {
		playerShell(ent);
	} else {
		ent->s.effects &= ~EF_COLOR_SHELL;
		ent->s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}
}

// ============================================================
// Frozen player help / moan
// ============================================================

static constexpr int frozen_help = bit_v<1>;

void cmdMoan(gentity_t *ent) {
	if (!(ent->client->resp.help & frozen_help) && !(ent->svflags & SVF_BOT)) {
		ent->client->showscores = false;
		ent->client->resp.help |= frozen_help;
		gi.LocCenter_Print(ent, "You have been frozen.\nWait to be saved.\nTap Jump to cycle views. Tap Fire or [ ] to switch players.");
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
	}
	if (ent->client->moan_time > level.time)
		return;
	ent->client->moan_time = level.time + 2_sec;
	if (ent->svflags & SVF_BOT)
		ent->client->moan_time += random_time(5_sec, 30_sec);
	if (ent->waterlevel == 3) {
		if (rand() & 1)
			gi.sound(ent, CHAN_AUTO, gi.soundindex("flipper/flpidle1.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_AUTO, gi.soundindex("flipper/flpsrch1.wav"), 1, ATTN_NORM, 0);
	} else
		gi.sound(ent, CHAN_AUTO, moan[rand() % 8], 1, ATTN_NORM, 0);
}

// ============================================================
// Win condition
// ============================================================

static const char *freeze_team_name[] = { "Red", "Blue" };

void breakTeam(int team) {
	gtime_t break_time = level.time;

	for (uint32_t i = 0; i < (uint32_t)game.maxclients; i++) {
		gentity_t *ent = g_entities + 1 + i;
		if (!ent->inuse) continue;
		if (ent->client->frozen) {
			/*freeze*/
			{
				int slot = (int)(ent - g_entities) - 1;
				if (slot >= 0 && slot < (int)MAX_CLIENTS_KEX)
					thaw_records[slot].round_end_break = true;
			}
			/*freeze*/
			ent->client->frozen_time = break_time;
			break_time += 250_ms;
			continue;
		}
		if (ent->health > 0) {
			playerHealth(ent);
			playerWeapon(ent);
		}
	}
	freeze[team].break_time = break_time + 1_sec;
	if (rand() & 1)
		gi.LocBroadcast_Print(PRINT_HIGH, "{} team was run circles around by their foe.\n", freeze_team_name[team]);
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "{} team was less than a match for their foe.\n", freeze_team_name[team]);
}

void updateTeam(int team) {
	int frozen = 0, alive = 0, total = 0;
	team_t team_enum = freeze_team_t_from_idx(team);

	for (uint32_t i = 0; i < (uint32_t)game.maxclients; i++) {
		gentity_t *ent = g_entities + 1 + i;
		if (!ent->inuse) continue;
		if (ent->client->resp.spectator) continue;
		if (ent->client->sess.team != team_enum) continue;
		total++;
		if (ent->client->frozen) frozen++;
		if (ent->health > 0 && !ent->client->frozen) alive++;
	}
	freeze[team].frozen = frozen;
	freeze[team].alive  = alive;

	if (total > 0 && !alive) {
		for (int i = 0; i < 2; i++) {
			if (freeze[i].alive) {
				G_AdjustTeamScore(freeze_team_t_from_idx(i), 1);
				freeze[i].update = true;
			}
		}
		breakTeam(team);
		gi.positioned_sound(vec3_origin, world, CHAN_VOICE | CHAN_RELIABLE,
			gi.soundindex("world/xian1.wav"), 1, ATTN_NONE, 0);
	}
}

bool endCheck() {
	for (int i = 0; i < 2; i++) {
		if (level.time > freeze[i].last_update) {
			updateTeam(i);
			freeze[i].update    = false;
			freeze[i].last_update = level.time + 3_sec;
		}
	}
	if (capturelimit->value) {
		int limit = (int)capturelimit->value;
		for (int i = 0; i < 2; i++) {
			if (level.team_scores[freeze_team_t_from_idx(i)] >= limit)
				return true;
		}
	}
	return false;
}

void freezeIntermission() {
	int best = -1, winners = 0, team = 0;

	for (int i = 0; i < 2; i++) {
		int s = level.team_scores[freeze_team_t_from_idx(i)];
		if (s > best) { best = s; winners = 1; team = i; }
		else if (s == best) { winners++; }
	}
	if (winners > 1) {
		// Tiebreak on thaws
		best = -1; winners = 0;
		for (int i = 0; i < 2; i++) {
			if (freeze[i].thawed > best) { best = freeze[i].thawed; winners = 1; team = i; }
			else if (freeze[i].thawed == best) { winners++; }
		}
	}
	if (winners != 1) {
		gi.LocBroadcast_Print(PRINT_CENTER, "Stalemate!\n");
		gi.LocBroadcast_Print(PRINT_HIGH,   "Stalemate!\n");
		return;
	}
	gi.LocBroadcast_Print(PRINT_CENTER, "{} TEAM IS THE WINNER!\n", freeze_team_name[team]);
	gi.LocBroadcast_Print(PRINT_HIGH,   "{} TEAM IS THE WINNER!\n", freeze_team_name[team]);
}

// ============================================================
// Spawn helpers
// ============================================================

void playerHealth(gentity_t *ent) {
	ent->client->pers.inventory.fill(0);
	ent->client->pu_time_quad       = 0_ms;
	ent->client->pu_time_protection = 0_ms;
	ent->flags &= ~FL_POWER_ARMOR;
	ent->health      = ent->client->pers.max_health;
	ent->s.sound     = 0;
	ent->client->weapon_sound = 0;
}

static void putInventory(const char *s, gentity_t *ent) {
	gitem_t *item = FindItem(s);
	if (!item)
		return;
	ent->client->pers.inventory[item->id] = 1;
	gitem_t *ammo = GetItemByIndex(item->ammo);
	if (ammo)
		ent->client->pers.inventory[ammo->id] = ammo->quantity;
	ent->client->newweapon = item;
}

// Weapon bitmask values for ft_start_weapon cvar
enum {
	FT_WEP_SHOTGUN        = 1 << 0,
	FT_WEP_SUPERSHOTGUN   = 1 << 1,
	FT_WEP_MACHINEGUN     = 1 << 2,
	FT_WEP_CHAINGUN       = 1 << 3,
	FT_WEP_GRENADELAUNCHER= 1 << 4,
	FT_WEP_ROCKETLAUNCHER = 1 << 5,
	FT_WEP_HYPERBLASTER   = 1 << 6,
	FT_WEP_RAILGUN        = 1 << 7,
};

void playerWeapon(gentity_t *ent) {
	gitem_t *blaster = FindItem("blaster");
	if (blaster) {
		ent->client->pers.inventory[blaster->id] = 1;
		ent->client->newweapon = blaster;
	}

	if (ft_start_armor && ft_start_armor->value > 0) {
		gitem_t *armor = FindItem("jacket armor");
		if (armor)
			ent->client->pers.inventory[armor->id] = (int)(ft_start_armor->value / 2) * 2;
	}

	if (ft_start_weapon) {
		int mask = (int)ft_start_weapon->value;
		if (mask & FT_WEP_SHOTGUN)         putInventory("shotgun",          ent);
		if (mask & FT_WEP_SUPERSHOTGUN)    putInventory("super shotgun",     ent);
		if (mask & FT_WEP_MACHINEGUN)      putInventory("machinegun",        ent);
		if (mask & FT_WEP_CHAINGUN)        putInventory("chaingun",          ent);
		if (mask & FT_WEP_GRENADELAUNCHER) putInventory("grenade launcher",  ent);
		if (mask & FT_WEP_ROCKETLAUNCHER)  putInventory("rocket launcher",   ent);
		if (mask & FT_WEP_HYPERBLASTER)    putInventory("hyperblaster",      ent);
		if (mask & FT_WEP_RAILGUN)         putInventory("railgun",           ent);
	}
}
