#pragma once

struct freeze_team_t {
	int     thawed;
	bool    update;
	gtime_t last_update;
	int     frozen;
	int     alive;
	gtime_t break_time;
};

extern freeze_team_t freeze[2]; // [0]=Red, [1]=Blue
extern int           gib_queue;

/*freeze*/
struct thaw_record_t {
    vec3_t origin;
    bool   in_place;
    bool   round_end_break;
};
// Indexed by (ent - g_entities - 1). Separate from gclient_t to avoid sizeof change.
extern thaw_record_t thaw_records[MAX_CLIENTS_KEX];
/*freeze*/

// initialization
void freezeSpawn();

// per-frame (hooked in Phase 3)
void freezeMain(gentity_t *ent);

// damage intercept (hooked in Phase 3)
bool freezeCheck(gentity_t *ent, mod_t mod);
bool playerDamage(gentity_t *targ, gentity_t *attacker, int damage, mod_t mod);

// freeze state transitions
void freezeAnim(gentity_t *ent);
void playerThaw(gentity_t *ent);
void playerBreak(gentity_t *ent, int force);
void playerUnfreeze(gentity_t *ent);
void playerMove(gentity_t *ent);

// visuals
void playerShell(gentity_t *ent);
void freezeEffects(gentity_t *ent);
bool powerupBlinkVisible();
void playerView(gentity_t *ent);
void CreateFrozenBodyGhost(gentity_t *ent);
void UpdateFrozenBodyGhost(gentity_t *ent);
void RemoveFrozenBodyGhost(gentity_t *ent);

// win condition
void updateTeam(int team);
void breakTeam(int team);
bool endCheck();
void freezeIntermission();

// spawn helpers
void playerHealth(gentity_t *ent);
void playerWeapon(gentity_t *ent);

// misc
void cmdMoan(gentity_t *ent);
void gibThink(gentity_t *self);
bool gibCheck();
