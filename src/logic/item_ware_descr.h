/*
 * Copyright (C) 2002-2003, 2006-2009 by the Widelands Development Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef ITEM_WARE_DESCR_H
#define ITEM_WARE_DESCR_H

#include <cstring>
#include <string>

#include <stdint.h>

#include "logic/instances.h"
#include "io/filewrite.h"
#include "writeHTML.h"

struct Profile;
struct Section;
class Image;

#define WARE_MENU_PIC_WIDTH   24  //< Default width for ware's menu icons
#define WARE_MENU_PIC_HEIGHT  24  //< Default height for ware's menu icons
#define WARE_MENU_PIC_PAD_X    3  //< Default padding between menu icons
#define WARE_MENU_PIC_PAD_Y    4  //< Default padding between menu icons

namespace Widelands {

/**
 * Wares can be stored in storages. They can be transferred across an
 * Economy. They can be traded.
 * Both items (lumber, stone, ...) and workers are considered wares.
 * Every ware has a unique name. Note that an item must not have the same
 * name as a worker.
 *
 * Item wares are defined on a per-world basis, workers are defined on a
 * per-tribe basis.
 * Since the life-times of world and tribe descriptions are a bit dodgy, the
 * master list of wares is kept by the Game class. The list is created just
 * before the game starts.
 *
 * Note that multiple tribes can define a worker with the same name. The
 * different "version" of a worker must perform the same job, but they can
 * look differently.
*/
struct Item_Ware_Descr : public Map_Object_Descr {
	typedef Ware_Index::value_t Index;
	Item_Ware_Descr
		(const Tribe_Descr & tribe, char const * const name,
		 char const * const descname, const std::string & directory,
		 Profile &, Section & global_s);

	virtual ~Item_Ware_Descr() {};

	const Tribe_Descr & tribe() const {return m_tribe;}

	/// \return index to ware's icon inside picture stack
	const Image* icon() const throw () {return m_icon;}
	std::string icon_name() const throw () {return m_icon_fname;}

	/// \return ware's localized descriptive text
	const std::string & helptext() const throw () {return m_helptext;}

	/// How much of the ware type that an economy should store in storages.
	/// The special value std::numeric_limits<uint32_t>::max() means that the
	/// the target quantity of this ware type will never be checked and should
	/// not be configurable.
	uint32_t default_target_quantity() const {return m_default_target_quantity;}

	bool has_demand_check() const {
		return default_target_quantity() != std::numeric_limits<uint32_t>::max();
	}

	/// Called when a demand check for this ware type is encountered during
	/// parsing. If there was no default target quantity set in the ware type's
	/// configuration, set the default value 1.
	void set_has_demand_check() {
		if (m_default_target_quantity == std::numeric_limits<uint32_t>::max())
			m_default_target_quantity = 1;
	}

	virtual void load_graphics();
#ifdef WRITE_GAME_DATA_AS_HTML
	void writeHTML(::FileWrite &) const;
#endif

	/// returns the preciousness of the ware. It is used by the computer player
	uint8_t preciousness() const {return m_preciousness;}

private:
	const Tribe_Descr & m_tribe;
	std::string m_helptext;   ///< Long descriptive text
	uint32_t    m_default_target_quantity;
	std::string m_icon_fname; ///< Filename of ware's main picture
	const Image* m_icon;       ///< Index of ware's picture in picture stack
	uint8_t     m_preciousness;
};

}

#endif
