/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
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

#include "game_player_economies_data_packet.h"

#include "game.h"
#include "player.h"
#include "transport.h"
#include "widelands_fileread.h"
#include "widelands_filewrite.h"

#include "upcast.h"

namespace Widelands {

#define CURRENT_PACKET_VERSION 2


void Game_Player_Economies_Data_Packet::Read
(FileSystem & fs, Game * game, Map_Map_Object_Loader * const)
throw (_wexception)
{
	FileRead fr;

	fr.Open(fs, "binary/player_economies");

	Map   const &       map        = game->map();
	Map_Index     const max_index  = map.max_index();
	Extent        const extent     = map.extent();
	Player_Number const nr_players = map.get_nrplayers();

	const uint16_t packet_version = fr.Unsigned16();
	if (1 <= packet_version and packet_version <= CURRENT_PACKET_VERSION) {
		iterate_players_existing(p, nr_players, *game, player) {
			Player::economy_vector & economies = player->m_economies;
			uint16_t const nr_economies = economies.size();

			if (packet_version == 1) {
				uint16_t const nr_economies_from_file = fr.Unsigned16();
				if (nr_economies != nr_economies_from_file)
					throw wexception
						("Game_Player_Economies_Data_Packet::Read: in "
						 "binary/player_economies:%u: player %u: read number of "
						 "economies as %u, but this was read as %u elsewhere",
						 fr.GetPos() - 2, p,
						 nr_economies_from_file, nr_economies);
			}

			Player::economy_vector ecos(nr_economies);
			Player::economy_vector::const_iterator const ecos_end = ecos.end();
			for
				(Player::economy_vector::iterator it = ecos.begin();
				 it != ecos_end;
				 ++it)
				if
					(upcast
					 (Flag const,
					  flag,
					  (packet_version == 1 ?
					   map[fr.Coords32(extent)] : map[fr.Map_Index32(max_index)])
					  .get_immovable()))
					*it = flag->get_economy();
				else
					throw wexception
						("Game_Player_Economies_Data_Packet::Read: in "
						 "binary/player_economies:%u: player %u: there is no flag "
						 "at the specified location",
						 fr.GetPos() - 4, p);
			for (uint16_t j = 0; j < nr_economies; ++j) // Issue first balance
				(economies[j] = ecos[j])->balance_requestsupply();
		}
	} else
		throw wexception
			("Unknown version in Game_Player_Economies_Data_Packet: %u",
			 packet_version);
}

/*
 * Write Function
 */
void Game_Player_Economies_Data_Packet::Write
(FileSystem & fs, Game * game, Map_Map_Object_Saver * const)
throw (_wexception)
{
	FileWrite fw;

	fw.Unsigned16(CURRENT_PACKET_VERSION);

	Map const & map = game->map();
	Field const & field_0 = map[0];
	Player_Number const nr_players = map.get_nrplayers();
	iterate_players_existing_const(p, nr_players, *game, player) {
		Player::economy_vector const & economies = player->m_economies;
		Player::economy_vector::const_iterator const economies_end =
			economies.end();
		for
			(Player::economy_vector::const_iterator it = economies.begin();
			 it != economies_end;
			 ++it)
			//  Walk the map so that we find a representant.
			for (Field const * field = &field_0;; ++field) {
				assert(field < &map[map.max_index()]); //  should never reach end
				if (upcast(Flag const, flag, field->get_immovable()))
					if (flag->get_economy() == *it) {
						fw.Map_Index32(field - &field_0);
						break;
					}
			}
	}

	fw.Write(fs, "binary/player_economies");
}

};
