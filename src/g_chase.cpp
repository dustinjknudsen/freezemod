// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
/*freeze*/
#include "g_freeze.h"
/*freeze*/

void FreeFollower(gentity_t *ent) {
	if (!ent)
		return;

	if (!ent->client->follow_target)
		return;

	/*freeze*/
	// Clear our instance bit from the target we're leaving so it no longer appears dark to others.
	{
		int obs_idx = (int)(ent - g_entities) - 1;
		gentity_t *old_targ = ent->client->follow_target;
		if (GT(GT_FREEZE) && obs_idx >= 0 && obs_idx < 8 && old_targ->inuse) {
			old_targ->s.instance_bits &= ~(uint8_t)(1u << obs_idx);
			if (!old_targ->s.instance_bits)
				old_targ->svflags &= ~SVF_INSTANCED;
		}
	}
	/*freeze*/

	ent->client->follow_target = nullptr;
	ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);

	ent->client->ps.kick_angles = {};
	ent->client->ps.gunangles = {};
	ent->client->ps.gunoffset = {};
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunskin = 0;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunrate = 0;
	ent->client->ps.screen_blend = {};
	ent->client->ps.damage_blend = {};
	ent->client->ps.rdflags = RDF_NONE;

	/*freeze*/
	ent->client->freeze_chase_mode = 0;
	ent->svflags &= ~(SVF_NOCLIENT | SVF_INSTANCED);
	ent->s.instance_bits = 0;
	// Restore player model zeroed by vanilla chasecam path if still frozen
	if (ent->client->frozen) {
		ent->s.modelindex = MODELINDEX_PLAYER;
		ent->s.modelindex2 = MODELINDEX_PLAYER;
		// Restore ghost to invisible (position anchor only); no longer in chasecam.
		if (ent->client->frozen_body) {
			ent->client->frozen_body->svflags = (ent->client->frozen_body->svflags & ~SVF_INSTANCED) | SVF_NOCLIENT;
			ent->client->frozen_body->s.instance_bits = 0;
			gi.linkentity(ent->client->frozen_body);
		}
	}
	/*freeze*/
}

void FreeClientFollowers(gentity_t *ent) {
	if (!ent)
		return;

	for (auto ec : active_clients()) {
		if (!ec->client->follow_target)
			continue;
		if (ec->client->follow_target == ent)
			FreeFollower(ec);
	}
}

void UpdateChaseCam(gentity_t *ent) {
	vec3_t	o, ownerv, goal;
	gentity_t	*targ = ent->client->follow_target;
	vec3_t	forward, right;
	trace_t	trace;
	vec3_t	oldgoal;
	vec3_t	angles;
	
	// is our follow target gone?
	if (!targ || !targ->inuse || !targ->client || !ClientIsPlaying(targ->client) ||
		(targ->client->eliminated && !(targ->client->frozen && ent->client->sess.team == TEAM_SPECTATOR))) {
		if (ent->client->frozen) {
			FreeFollower(ent);
			GetFollowTarget(ent);
			if (!ent->client->follow_target && ent->client->frozen_body) {
				ent->s.origin = ent->client->frozen_body->s.origin;
				ent->movetype = MOVETYPE_NONE;
				gi.linkentity(ent);
			}
			return;
		}
		FreeClientFollowers(targ);
		return;
	}

	ownerv = targ->s.origin;
	oldgoal = ent->s.origin;

	/*freeze*/
	// In freeze tag, freeze_chase_mode controls view for everyone (frozen + spectators).
	// Outside freeze tag, non-frozen followers use g_eyecam as before.
	bool use_eyecam = GT(GT_FREEZE)
		? (ent->client->freeze_chase_mode == 1)
		: (bool)g_eyecam->integer;
	bool freeze_thirdperson = ent->client->frozen && ent->client->freeze_chase_mode == 2;

	// instance_bits bit for this observer (0-based slot index; uint8_t covers slots 1-8)
	int obs_idx = (int)(ent - g_entities) - 1;
	bool can_instance = GT(GT_FREEZE) && obs_idx >= 0 && obs_idx < 8;
	// Clear our bit each frame before we decide whether to re-set it below.
	if (can_instance)
		targ->s.instance_bits &= ~(uint8_t)(1u << obs_idx);
	/*freeze*/

	// Q2Eaks eyecam handling
	if (use_eyecam) {
		/*freeze*/
		// Set SVF_INSTANCED so the engine uses instance_bits for per-client visibility.
		// Set our bit so the engine skips sending the target model to us (no back-of-head).
		// Other clients' bits are 0, so they see the model normally (no dark gray).
		targ->svflags |= SVF_INSTANCED;
		if (can_instance)
			targ->s.instance_bits |= (uint8_t)(1u << obs_idx);
		// For frozen players in eyecam: keep entity at body for team indicator.
		// For live spectators: hide entity to prevent ghost overlay at target's position.
		if (GT(GT_FREEZE) && ent->client->frozen) {
			ent->svflags &= ~SVF_NOCLIENT;
			if (can_instance) {
				ent->svflags |= SVF_INSTANCED;
				ent->s.instance_bits |= (uint8_t)(1u << obs_idx);
			}
			ent->s.modelindex = MODELINDEX_PLAYER;
			ent->s.modelindex2 = MODELINDEX_PLAYER;
		} else if (GT(GT_FREEZE)) {
			ent->svflags |= SVF_NOCLIENT;
		} else {
			ent->svflags &= ~SVF_NOCLIENT;
		}
		/*freeze*/

		// copy everything from ps but pmove, pov, stats, and team
		ent->client->ps.viewangles = targ->client->ps.viewangles;
		ent->client->ps.viewoffset = targ->client->ps.viewoffset;
		ent->client->ps.kick_angles = targ->client->ps.kick_angles;
		ent->client->ps.gunangles = targ->client->ps.gunangles;
		ent->client->ps.gunoffset = targ->client->ps.gunoffset;
		ent->client->ps.gunindex = targ->client->ps.gunindex;
		ent->client->ps.gunskin = targ->client->ps.gunskin;
		ent->client->ps.gunframe = targ->client->ps.gunframe;
		ent->client->ps.gunrate = targ->client->ps.gunrate;
		ent->client->ps.screen_blend = targ->client->ps.screen_blend;
		ent->client->ps.damage_blend = targ->client->ps.damage_blend;
		ent->client->ps.rdflags = targ->client->ps.rdflags;

		// do pmove stuff so view looks right, but not pm_flags
		ent->client->ps.pmove.origin = targ->client->ps.pmove.origin;
		ent->client->ps.pmove.velocity = targ->client->ps.pmove.velocity;
		ent->client->ps.pmove.pm_time = targ->client->ps.pmove.pm_time;
		ent->client->ps.pmove.gravity = targ->client->ps.pmove.gravity;
		// Zero delta_angles - view is fully authoritative, avoids jitter from spectator cmd_angles mismatch
		ent->client->ps.pmove.delta_angles = {};
		ent->client->ps.pmove.viewheight = targ->client->ps.pmove.viewheight;

		ent->client->pers.hand = targ->client->pers.hand;
		ent->client->pers.weapon = targ->client->pers.weapon;

		//FIXME: color shells and damage blends not working

		// unadjusted view and origin handling
		angles = targ->client->v_angle;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		// Align ent origin with pmove (view position) for consistency
		goal = targ->client->ps.pmove.origin;
	}
	// vanilla / third-person chasecam
	else {
		/*freeze*/
		// Only clear SVF_INSTANCED if no other observer still has their bit set on this target.
		if (!targ->s.instance_bits)
			targ->svflags &= ~SVF_INSTANCED;
		/*freeze*/

		ownerv[2] += targ->viewheight;

		angles = targ->client->v_angle;
		if (angles[PITCH] > 56)
			angles[PITCH] = 56;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		o = ownerv + (forward * -30);

		if (o[2] < targ->s.origin[2] + 20)
			o[2] = targ->s.origin[2] + 20;

		// jump animation lifts
		if (!targ->groundentity)
			o[2] += 16;

		trace = gi.traceline(ownerv, o, targ, MASK_SOLID);

		goal = trace.endpos;

		goal += (forward * 2);

		// pad for floors and ceilings
		o = goal;
		o[2] += 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] -= 6;
		}

		o = goal;
		o[2] -= 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] += 6;
		}

		ent->client->ps.gunindex = 0;
		ent->client->ps.gunskin = 0;
		if (GT(GT_FREEZE) && ent->client->frozen) {
			// Keep model visible for team indicator; hide from self via instancing.
			if (can_instance) {
				ent->svflags |= SVF_INSTANCED;
				ent->s.instance_bits |= (uint8_t)(1u << obs_idx);
			}
			ent->s.modelindex = MODELINDEX_PLAYER;
			ent->s.modelindex2 = MODELINDEX_PLAYER;
		} else {
			ent->s.modelindex = 0;
			ent->s.modelindex2 = 0;
			ent->s.modelindex3 = 0;
		}
	}

	/*freeze*/
	if (freeze_thirdperson) {
		// Entity stays at body for team indicator; camera goes to goal via ps.pmove.origin.
		// SVF_NOCLIENT removed — instancing (set above) hides entity from self only.
		ent->svflags &= ~SVF_NOCLIENT;
		if (can_instance) {
			ent->svflags |= SVF_INSTANCED;
			ent->s.instance_bits |= (uint8_t)(1u << obs_idx);
		}
		ent->client->ps.pmove.pm_type = PM_FREEZE;
		ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;
		ent->client->ps.pmove.viewheight = 0;
		ent->client->ps.viewoffset = {};
		ent->client->ps.pmove.origin = goal;
		if (ent->client->frozen_body)
			ent->s.origin = ent->client->frozen_body->s.origin;
		else
			ent->s.origin = goal;
	} else {
	/*freeze*/
		if (targ->deadflag)
			ent->client->ps.pmove.pm_type = PM_DEAD;
		else
			ent->client->ps.pmove.pm_type = PM_FREEZE;

		// For frozen players: entity at body (indicator), camera at goal via ps.pmove.origin.
		if (GT(GT_FREEZE) && ent->client->frozen && ent->client->frozen_body) {
			ent->client->ps.pmove.origin = goal;
			ent->s.origin = ent->client->frozen_body->s.origin;
		} else {
			ent->s.origin = goal;
		}
	/*freeze*/
	}
	/*freeze*/

	if (!use_eyecam)
		ent->client->ps.pmove.delta_angles = targ->client->v_angle - ent->client->resp.cmd_angles;

	if (targ->deadflag) {
		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = targ->client->killer_yaw;
	} else {
		ent->client->ps.viewangles = targ->client->v_angle;
		ent->client->v_angle = targ->client->v_angle;
		AngleVectors(ent->client->v_angle, ent->client->v_forward, nullptr, nullptr);
	}

	gentity_t *e = targ ? targ : ent;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) || e->client->eliminated ? 0 : 1;

	ent->viewheight = 0;
	if (!use_eyecam)
		ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;
	/*freeze*/
	// Spectators in mode 2 use the vanilla path, which doesn't zero viewheight.
	// Eyecam (mode 1) copies the target's viewheight; clear it on the switch to third-person.
	if (GT(GT_FREEZE) && !ent->client->frozen && ent->client->freeze_chase_mode == 2) {
		ent->client->ps.pmove.viewheight = 0;
		ent->client->ps.viewoffset = {};
	}
	/*freeze*/

	/*freeze*/
	if (GT(GT_FREEZE) && targ->client->frozen) {
		ent->client->ps.screen_blend = {};
		if (targ->client->sess.team == TEAM_RED)
			G_AddBlend(0.6f, 0.0f, 0.0f, 0.1f, ent->client->ps.screen_blend);
		else
			G_AddBlend(1.0f, 1.0f, 1.0f, 0.1f, ent->client->ps.screen_blend);
	}
	/*freeze*/

	/*freeze*/
	// Let the frozen player see their own frozen body from the chased player's perspective.
	// The ghost is normally SVF_NOCLIENT; make it visible only to this observer.
	if (ent->client->frozen && ent->client->frozen_body && can_instance) {
		gentity_t *ghost = ent->client->frozen_body;
		ghost->svflags = (ghost->svflags & ~SVF_NOCLIENT) | SVF_INSTANCED;
		// All bits set except observer's: everyone else cannot see the ghost.
		ghost->s.instance_bits = (uint8_t)(~(1u << obs_idx));
		// Sync visual state — modelindex may be 0 on ent in eyecam, so force MODELINDEX_PLAYER.
		ghost->s.modelindex = MODELINDEX_PLAYER;
		ghost->s.modelindex2 = MODELINDEX_PLAYER;
		ghost->s.frame   = ent->s.frame;
		ghost->s.skinnum = ent->s.skinnum;
		ghost->s.effects = ent->s.effects;
		ghost->s.renderfx = ent->s.renderfx;
		gi.linkentity(ghost);
	}
	/*freeze*/

	/*freeze*/

	gi.linkentity(ent);
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
static void SanitizeString(const char *in, char *out) {
	while (*in) {
		if (*in < ' ') {
			in++;
			continue;
		}
		*out = tolower(*in);
		out++;
		in++;
	}

	*out = '\0';
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int ClientNumberFromString(gentity_t *to, char *s) {
	gclient_t	*cl;
	uint32_t	idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	
	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if ((unsigned)idnum >= (unsigned)game.maxclients) {
			gi.LocClient_Print(to, PRINT_HIGH, "Bad client slot: {}\n\"", idnum);
			return -1;
		}

		cl = &game.clients[idnum];
		if (!cl->pers.connected) {
			gi.LocClient_Print(to, PRINT_HIGH, "Client {} is not active.\n\"", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString(s, s2);
	for (idnum = 0, cl = game.clients; idnum < game.maxclients; idnum++, cl++) {
		if (!cl->pers.connected)
			continue;
		SanitizeString(cl->resp.netname, n2);
		if (!strcmp(n2, s2)) {
			return idnum;
		}
	}

	gi.LocClient_Print(to, PRINT_HIGH, "User {} is not on the server.\n\"", s);
	return -1;
}

void FollowNext(gentity_t *ent) {
	ptrdiff_t i;
	gentity_t *e;

	if (!ent->client->follow_target)
		return;

	i = ent->client->follow_target - g_entities;
	do {
		i++;
		if (i > game.maxclients)
			i = 1;
		e = g_entities + i;
		if (!e->inuse)
			continue;
		if (ent->client->eliminated && ent->client->sess.team != e->client->sess.team)
			continue;
		if (ClientIsPlaying(e->client) && (!e->client->eliminated || (e->client->frozen && ent->client->sess.team == TEAM_SPECTATOR)))
			break;
	} while (e != ent->client->follow_target);

	ent->client->follow_target = e;
	ent->client->follow_update = true;
	ent->client->sess.spectator_state = SPECTATOR_FOLLOW;
}

void FollowPrev(gentity_t *ent) {
	int		 i;
	gentity_t *e;

	if (!ent->client->follow_target)
		return;

	i = ent->client->follow_target - g_entities;
	do {
		i--;
		if (i < 1)
			i = game.maxclients;
		e = g_entities + i;
		if (!e->inuse)
			continue;
		if (ent->client->eliminated && ent->client->sess.team != e->client->sess.team)
			continue;
		if (ClientIsPlaying(e->client) && (!e->client->eliminated || (e->client->frozen && ent->client->sess.team == TEAM_SPECTATOR)))
			break;
	} while (e != ent->client->follow_target);

	ent->client->follow_target = e;
	ent->client->follow_update = true;
	ent->client->sess.spectator_state = SPECTATOR_FOLLOW;
}

void FollowCycle(gentity_t *ent, int dir) {
	int			clientnum;
	int			original;
	gclient_t	*cl = ent->client;
	gentity_t		*follow_ent = nullptr;

	// if they are playing a duel game, count as a loss
	if (GT(GT_DUEL) && ent->client->sess.team == TEAM_FREE)
		ent->client->sess.losses++;

	// first set them to spectator
	if (cl->sess.spectator_state == SPECTATOR_NOT && !cl->eliminated)
		SetTeam(ent, TEAM_SPECTATOR, false, false, false);

	clientnum = cl->sess.spectator_client;
	original = clientnum;
	do {
		clientnum = (clientnum + dir) % game.maxclients;
		follow_ent = &g_entities[clientnum + 1];

		// can only follow connected clients
		if (!follow_ent->client->pers.connected)
			continue;
		
		// can't follow another spectator
		if (!ClientIsPlaying(follow_ent->client))
			continue;

		if (follow_ent->client->eliminated && !(follow_ent->client->frozen && ent->client->sess.team == TEAM_SPECTATOR))
			continue;

		if (ent->client->eliminated && ent->client->sess.team != follow_ent->client->sess.team)
			continue;

		// this is good, we can use it
		//q3
		cl->sess.spectator_client = clientnum;
		cl->sess.spectator_state = SPECTATOR_FOLLOW;

		//q2
		ent->client->follow_target = follow_ent;
		ent->client->follow_update = true;

		return;
	} while (clientnum != original);

	// leave it where it was
}

void GetFollowTarget(gentity_t *ent) {
	for (auto ec : active_clients()) {
		if (ec->inuse && ClientIsPlaying(ec->client) && (!ec->client->eliminated || (ec->client->frozen && ent->client->sess.team == TEAM_SPECTATOR))) {
			if (ent->client->eliminated && ent->client->sess.team != ec->client->sess.team)
				continue;
			ent->client->follow_target = ec;
			ent->client->follow_update = true;
			ent->client->sess.spectator_state = SPECTATOR_FOLLOW;
			UpdateChaseCam(ent);
			return;
		}
	}
	/*
	if (ent->client->chase_msg_time <= level.time) {
		if (ent->client->sess.initialised) {
			gi.LocCenter_Print(ent, "$g_no_players_chase");
			ent->client->chase_msg_time = level.time + 5_sec;
		} else {
			G_Menu_Join_Open(ent);
		}
	}
	*/
}
