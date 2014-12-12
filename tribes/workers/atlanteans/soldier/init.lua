dirname = path.dirname(__file__)

animations = {
   idle = {
      pictures = { dirname .. "idle_\\d+.png" },
      hotspot = { 10, 36 },
      fps = 5
   },
   atk_ok_e = {
      pictures = { dirname .. "atk_ok_e_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 10
   },
   atk_fail_e = {
      pictures = { dirname .. "atk_fail_e_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 10
   },
   eva_ok_e = {
      pictures = { dirname .. "eva_ok_e_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 20
   },
   eva_fail_e = {
      pictures = { dirname .. "eva_fail_e_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 10
   },
   atk_ok_w = {
      pictures = { dirname .. "atk_ok_w_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 10
   },
   atk_fail_w = {
      pictures = { dirname .. "atk_fail_w_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 10
   },
   eva_ok_w = {
      pictures = { dirname .. "eva_ok_w_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 20
   },
   eva_fail_w = {
      pictures = { dirname .. "eva_fail_w_\\d+.png" },
      hotspot = { 36, 40 },
      fps = 10
   },
   die_w = {
      pictures = { dirname .. "die_\\d+.png" },
      hotspot = { 10, 36 },
      fps = 10
   },
   die_e = {
      pictures = { dirname .. "eva_fail_w_\\d+.png" },
      hotspot = { 10, 36 },
      fps = 10
   }
}
add_worker_animations(animations, "walk", dirname, "walk", {20, 34}, 10)


tribes:new_soldier_type {
   name = "atlanteans_soldier",
   -- TRANSLATORS: This is a worker name used in lists of workers
   descname = _"Soldier",

   buildcost = {
		atlanteans_carrier = 1,
		tabard = 1,
		trident_light = 1
	},

	-- TRANSLATORS: Helptext for a worker: Soldier
   helptext = _"Defend and Conquer!",
   animations = animations,

	max_hp_level = 1,
	max_attack_level = 4,
	max_defense_level = 2,
	max_evade_level = 2,

	-- Initial values and per level increments.
	hp = 13500,
	hp_incr_per_level = 4000,
	attack = 1200-1600, -- NOCOM(#GunChleoc): Why are there 2 numbers here?
	attack_incr_per_level = 800,
	defense = 6,
	defense_incr_per_level = 8,
	evade = 30,
	evade_incr_per_level = 17,

	-- NOCOM(#GunChleoc): Canonical filenames, then we can get rid of these?
	hp_level_0_pic = "hp_level0.png",
	hp_level_1_pic = "hp_level1.png",
	evade_level_0_pic = "evade_level0.png",
	evade_level_1_pic = "evade_level1.png",
	evade_level_2_pic = "evade_level2.png",
	attack_level_0_pic = "attack_level0.png",
	attack_level_1_pic = "attack_level1.png",
	attack_level_2_pic = "attack_level2.png",
	attack_level_3_pic = "attack_level3.png",
	attack_level_4_pic = "attack_level4.png",
	defense_level_0_pic = "defense_level0.png",
	defense_level_1_pic = "defense_level1.png",
	defense_level_2_pic = "defense_level2.png",

	-- Animations for battle
	-- NOCOM (#GunChleoc): Can we just rename the animations on top and be done with it?
	attack_success_w = "atk_ok_w",
	attack_success_e = "atk_ok_e",
	attack_failure_w = "atk_fail_w",
	attack_failure_e = "atk_fail_e"
	evade_success_w = "eva_ok_w",
	evade_success_e = "eva_ok_e",
	evade_failure_w = "eva_fail_w",
	evade_failure_e = "eva_fail_e",
	die_w = "die_w",
	die_e = "die_e"
}
