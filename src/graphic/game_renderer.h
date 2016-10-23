/*
 * Copyright (C) 2010-2013 by the Widelands Development Team
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

#ifndef WL_GRAPHIC_GAME_RENDERER_H
#define WL_GRAPHIC_GAME_RENDERER_H

#include <memory>

#include "base/macros.h"
#include "base/point.h"

namespace Widelands {
class Player;
class EditorGameBase;
}

class RenderTarget;

/**
 * This abstract base class renders the main game view into an
 * arbitrary @ref RenderTarget.
 *
 * Specializations exist for different OpenGL rendering paths.
 *
 * Users of this class should keep instances alive for as long as possible,
 * so that target-specific optimizations (such as caching data) can
 * be effective.
 *
 * Every instance can only perform one render operation per frame. When
 * multiple views of the map are open, each needs its own instance of
 * GameRenderer.
 */
class GameRenderer {
public:
	virtual ~GameRenderer();

	// Create a game renderer instance.
	static std::unique_ptr<GameRenderer> create();

	// Renders the map from a player's point of view into the given
	// drawing window. 'view_offset' is the offset of the upper left
	// corner of the window into the map, in pixels.
	void rendermap(RenderTarget& dst,
	               const Widelands::EditorGameBase& egbase,
	               const Point& view_offset,
	               const Widelands::Player& player);

	// Renders the map from an omniscient perspective. This is used
	// for spectators, players that see all, and in the editor.
	void rendermap(RenderTarget& dst,
	               const Widelands::EditorGameBase& egbase,
	               const Point& view_offset);

protected:
	GameRenderer();

	virtual void draw(RenderTarget& dst,
	                  const Widelands::EditorGameBase& egbase,
	                  const Point& view_offset,
	                  const Widelands::Player* player) = 0;

	// Draws the objects (animations & overlays).
	void draw_objects(RenderTarget& dst,
	                  const Widelands::EditorGameBase& egbase,
	                  const Point& view_offset,
	                  const Widelands::Player* player,
	                  int minfx,
	                  int maxfx,
	                  int minfy,
	                  int maxfy);

	DISALLOW_COPY_AND_ASSIGN(GameRenderer);
};

#endif  // end of include guard: WL_GRAPHIC_GAME_RENDERER_H
