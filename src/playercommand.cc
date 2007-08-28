/*
 * Copyright (C) 2004, 2007 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "playercommand.h"

#include "error.h"
#include "game.h"
#include "instances.h"
#include "player.h"
#include "soldier.h"
#include "wexception.h"
#include "widelands_fileread.h"
#include "widelands_filewrite.h"
#include "widelands_map_map_object_saver.h"
#include "widelands_map_map_object_loader.h"
#include "widelands_streamread.h"
#include "widelands_streamwrite.h"

enum {
	PLCMD_UNUSED=0,
	PLCMD_BULLDOZE,
	PLCMD_BUILD,
	PLCMD_BUILDFLAG,
	PLCMD_BUILDROAD,
	PLCMD_FLAGACTION,
	PLCMD_STARTSTOPBUILDING,
	PLCMD_ENHANCEBUILDING,
	PLCMD_CHANGETRAININGOPTIONS,
	PLCMD_DROPSOLDIER,
	PLCMD_CHANGESOLDIERCAPACITY,
/// TESTING STUFF
	PLCMD_ENEMYFLAGACTION,
};

/*** class PlayerCommand ***/

PlayerCommand::PlayerCommand (int t, char s):BaseCommand (t)
{
	sender=s;
}

PlayerCommand::~PlayerCommand ()
{
}

PlayerCommand* PlayerCommand::deserialize (WidelandsStreamRead & des)
{
	switch (des.Unsigned8()) {
		case PLCMD_BULLDOZE:
			return new Cmd_Bulldoze(des);
		case PLCMD_BUILD:
			return new Cmd_Build(des);
		case PLCMD_BUILDFLAG:
			return new Cmd_BuildFlag(des);
		case PLCMD_BUILDROAD:
			return new Cmd_BuildRoad(des);
		case PLCMD_FLAGACTION:
			return new Cmd_FlagAction(des);
		case PLCMD_STARTSTOPBUILDING:
			return new Cmd_StartStopBuilding(des);
		case PLCMD_ENHANCEBUILDING:
			return new Cmd_EnhanceBuilding(des);
		case PLCMD_CHANGETRAININGOPTIONS:
			return new Cmd_ChangeTrainingOptions(des);
		case PLCMD_DROPSOLDIER:
			return new Cmd_DropSoldier(des);
		case PLCMD_CHANGESOLDIERCAPACITY:
			return new Cmd_ChangeSoldierCapacity(des);
   ///   TESTING STUFF
      case PLCMD_ENEMYFLAGACTION:
			return new Cmd_EnemyFlagAction(des);
		default:
			throw wexception("PlayerCommand::deserialize(): Invalid command id encountered");
	}
}

/*
 * Write this player command to a file. Call this from base classes
 */
#define PLAYER_COMMAND_VERSION 1
void PlayerCommand::PlayerCmdWrite
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_COMMAND_VERSION);

	BaseCommand::BaseCmdWrite(fw, egbase, mos);
	// Now sender
	fw.Unsigned8 (sender);
}
void PlayerCommand::PlayerCmdRead
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_COMMAND_VERSION) {
		BaseCommand::BaseCmdRead(fr, egbase, mol);
		sender = fr.Unsigned8 ();
	} else throw wexception
		("Unknown version in PlayerCommand::PlayerCmdRead: %u", packet_version);
}

/*** class Cmd_Bulldoze ***/

Cmd_Bulldoze::Cmd_Bulldoze (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	serial = des.Unsigned32();
}

void Cmd_Bulldoze::execute (Game* g)
{
	Player* player = g->get_player(get_sender());
	Map_Object* obj = g->objects().get_object(serial);

	if (obj && obj->get_type() >= Map_Object::BUILDING)
		player->bulldoze(static_cast<PlayerImmovable*>(obj));
}

void Cmd_Bulldoze::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_BULLDOZE);
	ser.Unsigned8 (get_sender());
	ser.Unsigned32(serial);
}
#define PLAYER_CMD_BULLDOZE_VERSION 1
void Cmd_Bulldoze::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_BULLDOZE_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);
		const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial=mol.get_object_by_file_index(fileserial)->get_serial();
	} else throw wexception
		("Unknown version in Cmd_Bulldoze::Read: %u", packet_version);
}
void Cmd_Bulldoze::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_BULLDOZE_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);
	// Now serial
	const Map_Object * const obj = egbase.objects().get_object(serial);
	assert(mos.is_object_known(obj));
	fw.Unsigned32(mos.get_object_file_index(obj));
}

/*** class Cmd_Build ***/

Cmd_Build::Cmd_Build (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	id     = des.Signed16  ();
	coords = des.Coords32  ();
}

void Cmd_Build::execute (Game* g)
{
	Player *player = g->get_player(get_sender());
	player->build(coords, id);
}

void Cmd_Build::serialize (WidelandsStreamWrite & ser) {
	ser.Unsigned8 (PLCMD_BUILD);
	ser.Unsigned8 (get_sender());
	ser.Signed16  (id);
	ser.Coords32  (coords);
}
#define PLAYER_CMD_BUILD_VERSION 1
void Cmd_Build::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_BUILD_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);
		id          = fr.Unsigned16();
		try {coords = fr.Coords32  (egbase.map().extent());}
		catch (const WidelandsStreamRead::Width_Exceeded e) {
			throw wexception
				("Cmd_Build::Read: reading coords: width (%u) exceeded (x = %i)",
				 e.w, e.x);
		}
		catch (const WidelandsStreamRead::Height_Exceeded e) {
			throw wexception
				("Cmd_Build::Read: reading coords: height (%u) exceeded (y = %i)",
				 e.h, e.y);
		}
	} else throw wexception
		("Unknown version in Cmd_Build::Read: %u", packet_version);
}

void Cmd_Build::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_BUILD_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);
	fw.Unsigned16(id);
	fw.Coords32  (coords);
}


/*** class Cmd_BuildFlag ***/

Cmd_BuildFlag::Cmd_BuildFlag (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	coords = des.Coords32  ();
}

void Cmd_BuildFlag::execute (Game* g)
{
	Player *player = g->get_player(get_sender());
	player->build_flag(coords);
}

void Cmd_BuildFlag::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_BUILDFLAG);
	ser.Unsigned8 (get_sender());
	ser.Coords32  (coords);
}
#define PLAYER_CMD_BUILDFLAG_VERSION 1
void Cmd_BuildFlag::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_BUILDFLAG_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);
		try {coords = fr.Coords32(egbase.map().extent());}
		catch (const WidelandsStreamRead::Width_Exceeded e) {
			throw wexception
				("Cmd_BuildFlag::Read: reading coords: width (%u) exceeded (x = "
				 "%i)",
				 e.w, e.x);
		}
		catch (const WidelandsStreamRead::Height_Exceeded e) {
			throw wexception
				("Cmd_BuildFlag::Read: reading coords: height (%u) exceeded (y = "
				 "%i)",
				 e.h, e.y);
		}
	} else throw wexception
		("Unknown version in Cmd_BuildFlag::Read: %u", packet_version);
}
void Cmd_BuildFlag::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_BUILDFLAG_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);
	fw.Coords32  (coords);
}

/*** class Cmd_BuildRoad ***/

Cmd_BuildRoad::Cmd_BuildRoad (int t, int p, Path & pa) :
PlayerCommand(t, p),
path         (&pa),
start        (pa.get_start()),
nsteps       (pa.get_nsteps()),
steps        (0)
{}

Cmd_BuildRoad::Cmd_BuildRoad (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	start  = des.Coords32  ();
	nsteps = des.Unsigned16();

	// we cannot completely deserialize the path here because we don't have a Map
	path=0;
	steps=new char[nsteps];

	for (Path::Step_Vector::size_type i = 0; i < nsteps; ++i)
		steps[i] = des.Unsigned8();
}

Cmd_BuildRoad::~Cmd_BuildRoad ()
{
	delete path;

	delete[] steps;
}

void Cmd_BuildRoad::execute (Game* g)
{
	if (path==0) {
		assert (steps!=0);

		path = new Path(start);
		for (Path::Step_Vector::size_type i = 0; i < nsteps; ++i)
			path->append (g->map(), steps[i]);
	}

	Player *player = g->get_player(get_sender());
	player->build_road(*path);
}

void Cmd_BuildRoad::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_BUILDROAD);
	ser.Unsigned8 (get_sender());
	ser.Coords32  (start);
	ser.Unsigned16(nsteps);

	assert (path!=0 || steps!=0);

	for (Path::Step_Vector::size_type i = 0; i < nsteps; ++i)
		ser.Unsigned8(path ? (*path)[i] : steps[i]);
}
#define PLAYER_CMD_BUILDROAD_VERSION 1
void Cmd_BuildRoad::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_BUILDROAD_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);
		start  = fr.Coords32  (egbase.map().extent());
		nsteps = fr.Unsigned16();
		steps= new char[nsteps];

		for (Path::Step_Vector::size_type i = 0; i < nsteps; ++i)
			steps[i]=fr.Unsigned8();
	} else throw wexception
		("Unknown version in Cmd_BuildRoad::Read: %u", packet_version);
}
void Cmd_BuildRoad::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_BUILDROAD_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);
	fw.Coords32  (start);
	fw.Unsigned16(nsteps);
	for (Path::Step_Vector::size_type i = 0; i < nsteps; ++i)
		fw.Unsigned8(path ? (*path)[i] : steps[i]);
}


/*** Cmd_FlagAction ***/
Cmd_FlagAction::Cmd_FlagAction (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	action = des.Unsigned8 ();
	serial = des.Unsigned32();
}

void Cmd_FlagAction::execute (Game* g)
{
	Player* player = g->get_player(get_sender());
	Map_Object* obj = g->objects().get_object(serial);

	if (obj && obj->get_type() == Map_Object::FLAG && static_cast<PlayerImmovable*>(obj)->get_owner() == player)
		player->flagaction (static_cast<Flag*>(obj), action);
}

void Cmd_FlagAction::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_FLAGACTION);
	ser.Unsigned8 (get_sender());
	ser.Unsigned8 (action);
	ser.Unsigned32(serial);
}

#define PLAYER_CMD_FLAGACTION_VERSION 1
void Cmd_FlagAction::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_FLAGACTION_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);

		action = fr.Unsigned8 ();

		// Serial
		const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial=mol.get_object_by_file_index(fileserial)->get_serial();
	} else throw wexception
		("Unknown version in Cmd_FlagAction::Read: %u", packet_version);
}
void Cmd_FlagAction::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_FLAGACTION_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);
	// Now action
	fw.Unsigned8 (action);

	// Now serial
	const Map_Object * const obj = egbase.objects().get_object(serial);
	assert(mos.is_object_known(obj));
	fw.Unsigned32(mos.get_object_file_index(obj));
}

/*** Cmd_StartStopBuilding ***/

Cmd_StartStopBuilding::Cmd_StartStopBuilding (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	serial=des.Unsigned32();
}

void Cmd_StartStopBuilding::execute (Game* g)
{
	Player* player = g->get_player(get_sender());
	Map_Object* obj = g->objects().get_object(serial);

	if (obj && obj->get_type() >= Map_Object::BUILDING)
		player->start_stop_building(static_cast<PlayerImmovable*>(obj));
}

void Cmd_StartStopBuilding::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_STARTSTOPBUILDING);
	ser.Unsigned8 (get_sender());
	ser.Unsigned32(serial);
}
#define PLAYER_CMD_STOPBUILDING_VERSION 1
void Cmd_StartStopBuilding::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_STOPBUILDING_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);

		// Serial
		const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial=mol.get_object_by_file_index(fileserial)->get_serial();
	} else throw wexception
		("Unknown version in Cmd_StartStopBuilding::Read: %u", packet_version);
}
void Cmd_StartStopBuilding::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_STOPBUILDING_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);

	// Now serial
	const Map_Object * const obj = egbase.objects().get_object(serial);
	assert(mos.is_object_known(obj));
	fw.Unsigned32(mos.get_object_file_index(obj));
}


/*** Cmd_EnhanceBuilding ***/

Cmd_EnhanceBuilding::Cmd_EnhanceBuilding (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	serial = des.Unsigned32();
	id     = des.Unsigned16();
}

void Cmd_EnhanceBuilding::execute (Game* g)
{
	if
		(Building * const building =
		 dynamic_cast<Building * const>(g->objects().get_object(serial)))
		g->get_player(get_sender())->enhance_building(building, id);
}

void Cmd_EnhanceBuilding::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_ENHANCEBUILDING);
	ser.Unsigned8 (get_sender());
	ser.Unsigned32(serial);
	ser.Unsigned16(id);
}
#define PLAYER_CMD_ENHANCEBUILDING_VERSION 1
void Cmd_EnhanceBuilding::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_ENHANCEBUILDING_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);

		const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial =mol.get_object_by_file_index(fileserial)->get_serial();

		id = fr.Unsigned16();
	} else throw wexception
		("Unknown version in Cmd_EnhanceBuilding::Read: %u", packet_version);
}
void Cmd_EnhanceBuilding::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_ENHANCEBUILDING_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);

	// Now serial
	const Map_Object * const obj = egbase.objects().get_object(serial);
	assert(mos.is_object_known(obj));
	fw.Unsigned32(mos.get_object_file_index(obj));

	// Now id
	fw.Unsigned16(id);
}


/*** class Cmd_ChangeTrainingOptions ***/
Cmd_ChangeTrainingOptions::Cmd_ChangeTrainingOptions
(WidelandsStreamRead & des)
:
PlayerCommand (0, des.Unsigned8())
{
	serial    = des.Unsigned32();  //  Serial of the building
	attribute = des.Unsigned16();  //  Attribute to modify
	value     = des.Unsigned16();  //  New vale
}

void Cmd_ChangeTrainingOptions::execute (Game* g)
{
	Player* player = g->get_player(get_sender());
	Map_Object* obj = g->objects().get_object(serial);

	/* � Maybe we must check that the building is a training house ? */
	if ((obj) && (obj->get_type() >= Map_Object::BUILDING)) {
		player->change_training_options(static_cast<PlayerImmovable*>(obj), attribute, value);
	}

}

void Cmd_ChangeTrainingOptions::serialize (WidelandsStreamWrite & ser) {
	ser.Unsigned8 (PLCMD_CHANGETRAININGOPTIONS);
	ser.Unsigned8 (get_sender());
	ser.Unsigned32(serial);
	ser.Unsigned16(attribute);
	ser.Unsigned16(value);
}


#define PLAYER_CMD_CHANGETRAININGOPTIONS_VERSION 1
void Cmd_ChangeTrainingOptions::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_CHANGETRAININGOPTIONS_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);

		// Serial
		const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial = mol.get_object_by_file_index(fileserial)->get_serial();

		attribute = fr.Unsigned16();
		value     = fr.Unsigned16();
	} else throw wexception
		("Unknown version in Cmd_ChangeTrainingOptions::Read: %u",
		 packet_version);
}

void Cmd_ChangeTrainingOptions::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_CHANGETRAININGOPTIONS_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);

	// Now serial
	const Map_Object * const obj = egbase.objects().get_object(serial);
	assert(mos.is_object_known(obj));
	fw.Unsigned32(mos.get_object_file_index(obj));

	fw.Unsigned16(attribute);
	fw.Unsigned16(value);
}

/*** class Cmd_DropSoldier ***/

Cmd_DropSoldier::Cmd_DropSoldier(WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	serial  = des.Unsigned32(); //  Serial of the building
	soldier = des.Unsigned32(); //  Serial of soldier
}

void Cmd_DropSoldier::execute (Game* g)
{
	Player* player = g->get_player(get_sender());
	Map_Object* obj = g->objects().get_object(serial);
	Map_Object* sold = g->objects().get_object(soldier);

	/* � Maybe we must check that the building is a training house ? */
	if ((obj) && (sold) && (obj->get_type() >= Map_Object::BUILDING) && (((Worker*)sold)->get_worker_type() == Worker_Descr::SOLDIER)) {
		player->drop_soldier(static_cast<PlayerImmovable*>(obj), static_cast<Soldier*>(sold));
	}
}

void Cmd_DropSoldier::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_DROPSOLDIER);
	ser.Unsigned8 (get_sender());
	ser.Unsigned32(serial);
	ser.Unsigned32(soldier);
}

#define PLAYER_CMD_DROPSOLDIER_VERSION 1
void Cmd_DropSoldier::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_DROPSOLDIER_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);

		// Serial
  const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial=mol.get_object_by_file_index(fileserial)->get_serial();

		// Soldier serial
		int soldierserial=fr.Unsigned32();
		assert(mol.is_object_known(soldierserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		soldier=mol.get_object_by_file_index(soldierserial)->get_serial();
	} else throw wexception
		("Unknown version in Cmd_DropSoldier::Read: %u", packet_version);
}

void Cmd_DropSoldier::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_DROPSOLDIER_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);

	// Now serial
	{
		const Map_Object * const obj = egbase.objects().get_object(serial);
		assert(mos.is_object_known(obj));
		fw.Unsigned32(mos.get_object_file_index(obj));
	}

	// Now soldier serial
	{
		const Map_Object * const obj = egbase.objects().get_object(serial);
		assert(mos.is_object_known(obj));
		fw.Unsigned16(mos.get_object_file_index(obj));
	}

}

/*** Cmd_ChangeSoldierCapacity ***/

Cmd_ChangeSoldierCapacity::Cmd_ChangeSoldierCapacity(WidelandsStreamRead & des)
:
PlayerCommand (0, des.Unsigned8())
{
	serial = des.Unsigned32();
	val    = des.Unsigned16();
}

void Cmd_ChangeSoldierCapacity::execute (Game* g)
{
	Player* player = g->get_player(get_sender());
	Map_Object* obj = g->objects().get_object(serial);

	if (obj && obj->get_type() >= Map_Object::BUILDING)
		player->change_soldier_capacity(static_cast<PlayerImmovable*>(obj), val);
}

void Cmd_ChangeSoldierCapacity::serialize (WidelandsStreamWrite & ser)
{
	ser.Unsigned8 (PLCMD_CHANGESOLDIERCAPACITY);
	ser.Unsigned8 (get_sender());
	ser.Unsigned32(serial);
	ser.Unsigned16(val);
}

#define PLAYER_CMD_CHANGESOLDIERCAPACITY_VERSION 1
void Cmd_ChangeSoldierCapacity::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_CHANGESOLDIERCAPACITY_VERSION) {
      // Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);

      // Serial
		const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial = mol.get_object_by_file_index(fileserial)->get_serial();

      // Now new capacity
		val = fr.Unsigned16();
	} else throw wexception
		("Unknown version in Cmd_ChangeSoldierCapacity::Read: %u",
		 packet_version);
}

void Cmd_ChangeSoldierCapacity::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_CHANGESOLDIERCAPACITY_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);

	// Now serial
	const Map_Object * const obj = egbase.objects().get_object(serial);
	assert(mos.is_object_known(obj));
	fw.Unsigned32(mos.get_object_file_index(obj));

	// Now capacity
	fw.Unsigned16(val);

}

/// TESTING STUFF
/*** Cmd_EnemyFlagAction ***/

Cmd_EnemyFlagAction::Cmd_EnemyFlagAction (WidelandsStreamRead & des) :
PlayerCommand (0, des.Unsigned8())
{
	action   = des.Unsigned8 ();
	serial   = des.Unsigned32();
	attacker = des.Unsigned8 ();
	number   = des.Unsigned8 ();
	type     = des.Unsigned8 ();
}

void Cmd_EnemyFlagAction::execute (Game* g)
{
	Player* player = g->get_player(get_sender());
	Map_Object* obj = g->objects().get_object(serial);
	PlayerImmovable* imm = static_cast<PlayerImmovable*>(obj);

	Player* real_player = g->get_player(attacker);

	log("player(%d)    imm->get_owner (%d)   real_player (%d)\n",
		player->get_player_number(),
		imm->get_owner()->get_player_number(),
		real_player->get_player_number());

	if (obj &&
		obj->get_type() == Map_Object::FLAG &&
		imm->get_owner() != real_player)
		real_player->enemyflagaction (static_cast<Flag*>(obj), action, attacker, number, type);
	else
		log ("Cmd_EnemyFlagAction Player invalid.\n");
}

void Cmd_EnemyFlagAction::serialize (WidelandsStreamWrite & ser) {
	ser.Unsigned8 (PLCMD_ENEMYFLAGACTION);
	ser.Unsigned8 (get_sender());
	ser.Unsigned8 (action);
	ser.Unsigned32(serial);
	ser.Unsigned8 (attacker);
	ser.Unsigned8 (number);
	ser.Unsigned8 (type);
}
#define PLAYER_CMD_ENEMYFLAGACTION_VERSION 2
void Cmd_EnemyFlagAction::Read
(WidelandsFileRead               & fr,
 Editor_Game_Base                & egbase,
 Widelands_Map_Map_Object_Loader & mol)
{
	const Uint16 packet_version = fr.Unsigned16();
	if (packet_version == PLAYER_CMD_ENEMYFLAGACTION_VERSION) {
		// Read Player Command
		PlayerCommand::PlayerCmdRead(fr, egbase, mol);

		// action
		action   = fr.Unsigned8 ();

		// Serial
		const Uint32 fileserial = fr.Unsigned32();
		assert(mol.is_object_known(fileserial)); //  FIXME NEVER USE assert TO VALIDATE INPUT!!!
		serial = mol.get_object_by_file_index(fileserial)->get_serial();

		// param
		attacker = fr.Unsigned8 ();
		number   = fr.Unsigned8 ();
		type     = fr.Unsigned8 ();
	} else throw wexception
		("Unknown version in Cmd_FlagAction::Read: %u", packet_version);
}

void Cmd_EnemyFlagAction::Write
(WidelandsFileWrite             & fw,
 Editor_Game_Base               & egbase,
 Widelands_Map_Map_Object_Saver & mos)
{
	// First, write version
	fw.Unsigned16(PLAYER_CMD_ENEMYFLAGACTION_VERSION);
	// Write base classes
	PlayerCommand::PlayerCmdWrite(fw, egbase, mos);
	// Now action
	fw.Unsigned8 (action);

	// Now serial
	const Map_Object * const obj = egbase.objects().get_object(serial);
	assert(mos.is_object_known(obj));
	fw.Unsigned32(mos.get_object_file_index(obj));

	// Now param
	fw.Unsigned8 (attacker);
	fw.Unsigned8 (number);
	fw.Unsigned8 (type);
}
