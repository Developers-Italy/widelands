/*
 * Copyright (C) 2002, 2006-2010 by the Widelands Development Team
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

// NOCOM(#sirver): check for ME also in conf files and therelike.
#include <cassert>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "constants.h"
#include "helper.h"
#include "io/filesystem/layered_filesystem.h"
#include "log.h"
#include "profile/profile.h"
#include "sound/sound_handler.h"
#include "wexception.h"

#include "diranimations.h"
#include "graphic.h"
#include "image.h"
#include "image_cache.h"
#include "image_transformations.h"
#include "logic/instances.h"  // For Map_Object_Descr.
#include "surface.h"

#include "animation.h"

using namespace std;


namespace  {

// NOCOM(#sirver): docu
void parse_point(const string& def, Point* p) {
	vector<string> split_vector;
	boost::split(split_vector, def, boost::is_any_of(","));
	if (split_vector.size() != 2)
		throw wexception("Invalid point definition: %s", def.c_str());

	p->x = boost::lexical_cast<int32_t>(split_vector[0]);
	p->y = boost::lexical_cast<int32_t>(split_vector[1]);
}

void parse_rect(const string& def, Rect* r) {
	vector<string> split_vector;
	boost::split(split_vector, def, boost::is_any_of(","));
	if (split_vector.size() != 4)
		throw wexception("Invalid rect definition: %s", def.c_str());

	r->x = boost::lexical_cast<int32_t>(split_vector[0]);
	r->y = boost::lexical_cast<int32_t>(split_vector[1]);
	r->w = boost::lexical_cast<uint32_t>(split_vector[2]);
	r->h = boost::lexical_cast<uint32_t>(split_vector[3]);
}

/**
 * Implements the Animation interface for a packed animation, that is an animation
 * that is contained in a singular image.
 */
class PackedAnimation : public Animation {
public:
	virtual ~PackedAnimation() {}
	PackedAnimation(const string& directory, Section & s);

	// Implements Animation.
	virtual uint16_t width() const {return width_;}
	virtual uint16_t height() const {return height_;}
	virtual uint16_t nr_frames() const {return nframes_;}
	virtual uint32_t frametime() const {return frametime_;}
	virtual const Point& hotspot() const {return hotspot_;};
	virtual const Image& representative_image(const RGBColor& clr) const;
	void blit(uint32_t time, const Point&, const Rect& srcrc, const RGBColor* clr, Surface*) const;
	virtual void trigger_soundfx(uint32_t framenumber, uint32_t stereo_position) const;

private:
	// NOCOM(#sirver): no support for playercolor at the moment.
	struct Region {
		Point target_offset;
		uint16_t w, h;
		std::vector<Point> source_offsets;  // indexed by frame nr.
	};

	uint16_t width_, height_;
	uint16_t nframes_;
	uint32_t frametime_;
	Point hotspot_;

	const Image* image_;
	const Image* pcmask_;
	std::vector<Region> regions_;

	/// mapping of soundeffect name to frame number, indexed by frame number .
	map<uint32_t, string> sfx_cues;
};

PackedAnimation::PackedAnimation(const string& directory, Section& s)
		: width_(0), height_(0), nframes_(0), frametime_(FRAME_LENGTH), image_(NULL), pcmask_(NULL) {
	// Read mapping from frame numbers to sound effect names and load effects
	while (Section::Value * const v = s.get_next_val("sfx")) {
		char * parameters = v->get_string(), * endp;
		unsigned long long int const value = strtoull(parameters, &endp, 0);
		const uint32_t frame_number = value;
		try {
			if (endp == parameters or frame_number != value)
				throw wexception("expected %s but found \"%s\"", "frame number", parameters);
			parameters = endp;
			force_skip(parameters);
			g_sound_handler.load_fx(directory, parameters);
			map<uint32_t, string>::const_iterator const it =
				sfx_cues.find(frame_number);
			if (it != sfx_cues.end())
				throw wexception
					("redefinition for frame %u to \"%s\" (previously defined to "
					 "\"%s\")",
					 frame_number, parameters, it->second.c_str());
		} catch (const _wexception & e) {
			throw wexception("sfx: %s", e.what());
		}
		sfx_cues[frame_number] = parameters;
	}

	int32_t const fps = s.get_int("fps");
	if (fps < 0)
		throw wexception("fps is %i, must be non-negative", fps);
	if (fps > 0)
		frametime_ = 1000 / fps;
	hotspot_ = s.get_Point("hotspot");

	// Load the graphis
	string pic_fn = directory + s.get_safe_string("pics");
	image_ = g_gr->images().get(pic_fn);
	boost::replace_all(pic_fn, ".png", "");
	if (g_fs->FileExists(pic_fn + "_pc.png")) {
		pcmask_ = g_gr->images().get(pic_fn + "_pc.png");
	}

	// Parse dimensions.
	Point p;
	parse_point(s.get_string("dimensions"), &p);
	width_ = p.x;
	height_ = p.y;

	// Parse regions
	NumberGlob glob("region_??");
	string region_name;
	while (glob.next(&region_name)) {
		string value=s.get_string(region_name.c_str(), "");
		if (value.empty())
			break;

		boost::trim(value);
		vector<string> split_vector;
		boost::split(split_vector, value, boost::is_any_of(":"));
		if (split_vector.size() != 2)
			throw wexception("%s: line is ill formatted. Should be <rect>:<offsets>", region_name.c_str());

		vector<string> offset_strings;
		boost::split(offset_strings, split_vector[1], boost::is_any_of(";"));
		if (nframes_ && nframes_ != offset_strings.size())
			throw wexception
				("%s: region has different number of frames than previous (%i != %"PRIuS").",
				 region_name.c_str(), nframes_, offset_strings.size());
		nframes_ = offset_strings.size();

		Rect region_rect;
		parse_rect(split_vector[0], &region_rect);

		Region r;
		r.target_offset.x = region_rect.x;
		r.target_offset.y = region_rect.y;
		r.w = region_rect.w;
		r.h = region_rect.h;
		BOOST_FOREACH(const string& offset_string, offset_strings) {
			parse_point(offset_string, &p);
			r.source_offsets.push_back(p);
		}
		regions_.push_back(r);
	}
	log("#sirver regions_.size(): %u\n", regions_.size());
	log("#sirver nr_frames(): %u\n", nr_frames());
}

void PackedAnimation::trigger_soundfx
	(uint32_t time, uint32_t stereo_position) const {
	const uint32_t framenumber = time / frametime_ % nr_frames();
	const map<uint32_t, string>::const_iterator sfx_cue = sfx_cues.find(framenumber);
	if (sfx_cue != sfx_cues.end())
		g_sound_handler.play_fx(sfx_cue->second, stereo_position, 1);
}

const Image& PackedAnimation::representative_image(const RGBColor& clr) const {
// NOCOM(#sirver): implement this
	return *g_gr->images().get("pics/but1.png");
}

void PackedAnimation::blit
	(uint32_t time, const Point& dst, const Rect& srcrc, const RGBColor* clr, Surface* target) const
{
	assert(target);
	const uint32_t framenumber = time / frametime_ % nr_frames();

	const Image* use_image = image_;
	if (clr && pcmask_) {
		use_image = ImageTransformations::player_colored(*clr, image_, pcmask_);
	}

	// NOCOM(#sirver): do not ignore srcrc
	target->blit(dst, use_image->surface(), Rect(0, 0, width_, height_));

	BOOST_FOREACH(const Region& r, regions_) {
		log("#sirver r.target_offset.x: %i\n", r.target_offset.x);
		log("#sirver r.target_offset.y: %i\n", r.target_offset.y);
		log("#sirver r.w: %i,r.h: %i\n", r.w, r.h);
		target->blit(dst + r.target_offset, use_image->surface(), Rect(r.source_offsets[framenumber], r.w, r.h));
	}
}

/**
 * Implements the Animation interface for an animation that is unpacked on disk, that
 * is every frame and every pc color frame is an singular file on disk.
 */
class NonPackedAnimation : public Animation {
public:
	virtual ~NonPackedAnimation() {}
	NonPackedAnimation(const string& directory, Section & s);

	// Implements Animation.
	virtual uint16_t width() const {return frames_[0]->width();}
	virtual uint16_t height() const {return frames_[0]->height();}
	virtual uint16_t nr_frames() const {return frames_.size();}
	virtual uint32_t frametime() const {return frametime_;}
	virtual const Point& hotspot() const {return hotspot_;};
	virtual const Image& representative_image(const RGBColor& clr) const {return get_frame(0, &clr);}
	void blit(uint32_t time, const Point&, const Rect& srcrc, const RGBColor* clr, Surface*) const;
	virtual void trigger_soundfx(uint32_t framenumber, uint32_t stereo_position) const;

private:
	// Returns the given frame image with the given clr (if not NULL).
	const Image& get_frame(uint32_t time, const RGBColor* playercolor = NULL) const;

	uint32_t frametime_;
	Point hotspot_;
	bool hasplrclrs_;

	vector<const Image*> frames_;
	vector<const Image*> pcmasks_;

	/// mapping of soundeffect name to frame number, indexed by frame number .
	map<uint32_t, string> sfx_cues;
};

NonPackedAnimation::NonPackedAnimation(const string& directory, Section& s)
		: frametime_(FRAME_LENGTH),
		  hasplrclrs_(false) {
	// Read mapping from frame numbers to sound effect names and load effects
	while (Section::Value * const v = s.get_next_val("sfx")) {
		char * parameters = v->get_string(), * endp;
		unsigned long long int const value = strtoull(parameters, &endp, 0);
		const uint32_t frame_number = value;
		try {
			if (endp == parameters or frame_number != value)
				throw wexception("expected %s but found \"%s\"", "frame number", parameters);
			parameters = endp;
			force_skip(parameters);
			g_sound_handler.load_fx(directory, parameters);
			map<uint32_t, string>::const_iterator const it =
				sfx_cues.find(frame_number);
			if (it != sfx_cues.end())
				throw wexception
					("redefinition for frame %u to \"%s\" (previously defined to "
					 "\"%s\")",
					 frame_number, parameters, it->second.c_str());
		} catch (const _wexception & e) {
			throw wexception("sfx: %s", e.what());
		}
		sfx_cues[frame_number] = parameters;
	}

	int32_t const fps = s.get_int("fps");
	if (fps < 0)
		throw wexception("fps is %i, must be non-negative", fps);
	if (fps > 0)
		frametime_ = 1000 / fps;

	hotspot_ = s.get_Point("hotspot");

	//  In the filename template, the last sequence of '?' characters (if any)
	//  is replaced with a number, for example the template "idle_??" is
	//  replaced with "idle_00". Then the code looks if there is a PNG with that
	//  name, increments the number and continues . on until it can not find any
	//  file. Then it is assumed that there are no more frames in the animation.
	string picnametempl;
	if (char const * const pics = s.get_string("pics")) {
		picnametempl = directory + pics;
	} else {
		picnametempl = directory + s.get_name();
	}
	// Strip the .png extension if it has one.
	boost::replace_all(picnametempl, ".png", "");

	NumberGlob glob(picnametempl);
	string filename_wo_ext;
	while (glob.next(&filename_wo_ext)) {
		const string filename = filename_wo_ext + ".png";
		if (!g_fs->FileExists(filename))
			break;

		const Image* image = g_gr->images().get(filename);
		if
			(frames_.size() &&
			 (frames_[0]->width() != image->width() or frames_[0]->height() != image->height()))
					throw wexception
						("wrong size: (%u, %u), should be (%u, %u) like the first frame",
						 image->width(), image->height(), frames_[0]->width(), frames_[0]->height());
		frames_.push_back(image);

		//TODO Do not load playercolor mask as opengl texture or use it as
		//     opengl texture.
		const string pc_filename = filename_wo_ext + "_pc.png";
		if (g_fs->FileExists(pc_filename)) {
			hasplrclrs_ = true;
			const Image* pc_image = g_gr->images().get(pc_filename);
			if (frames_[0]->width() != pc_image->width() or frames_[0]->height() != pc_image->height())
				throw wexception
					("playercolor mask has wrong size: (%u, %u), should "
					 "be (%u, %u) like the animation frame",
					 pc_image->width(), pc_image->height(), frames_[0]->width(), frames_[0]->height());
			pcmasks_.push_back(pc_image);
		}
	}

	if (frames_.empty())
		throw wexception("animation %s has no frames", picnametempl.c_str());

	if (pcmasks_.size() and pcmasks_.size() != frames_.size())
		throw wexception
			("animation has %"PRIuS" frames but playercolor mask has %"PRIuS" frames",
			 frames_.size(), pcmasks_.size());
}

void NonPackedAnimation::trigger_soundfx
	(uint32_t time, uint32_t stereo_position) const {
	const uint32_t framenumber = time / frametime_ % nr_frames();
	const map<uint32_t, string>::const_iterator sfx_cue = sfx_cues.find(framenumber);
	if (sfx_cue != sfx_cues.end())
		g_sound_handler.play_fx(sfx_cue->second, stereo_position, 1);
}

void NonPackedAnimation::blit
	(uint32_t time, const Point& dst, const Rect& srcrc, const RGBColor* clr, Surface* target) const
{
	assert(target);

	const Image& frame = get_frame(time, clr);
	target->blit(dst, frame.surface(), srcrc);
}

const Image& NonPackedAnimation::get_frame(uint32_t time, const RGBColor* playercolor) const {
	const uint32_t framenumber = time / frametime_ % nr_frames();
	assert(framenumber < nr_frames());
	const Image* original = frames_[framenumber];

	if (!hasplrclrs_ || !playercolor)
		return *original;

	assert(frames_.size() == pcmasks_.size());
	return *ImageTransformations::player_colored(*playercolor, original, pcmasks_[framenumber]);
}

}  // namespace


/*
==============================================================================

DirAnimations IMPLEMENTAION

==============================================================================
*/

DirAnimations::DirAnimations
	(uint32_t dir1,
	 uint32_t dir2,
	 uint32_t dir3,
	 uint32_t dir4,
	 uint32_t dir5,
	 uint32_t dir6)
{
	m_animations[0] = dir1;
	m_animations[1] = dir2;
	m_animations[2] = dir3;
	m_animations[3] = dir4;
	m_animations[4] = dir5;
	m_animations[5] = dir6;
}


/*
===============
Parse an animation from the given directory and config.
sectnametempl is of the form "foowalk_??", where ?? will be replaced with
nw, ne, e, se, sw and w to get the section names for the animations.

If defaults is not zero, the additional sections are not actually necessary.
If they don't exist, the data is taken from defaults and the bitmaps
foowalk_??_nn.png are used.
===============
*/
// NOCOM(#sirver): eventually kill this method as well - it seems unnecessary when
// so much data has to be given for each walk animation anyway.
void DirAnimations::parse
	(Widelands::Map_Object_Descr & b,
	 const string & directory,
	 Profile & prof,
	 char const * const sectnametempl,
	 Section * const defaults)
{
	char dirpictempl[256];
	char sectnamebase[256];
	char * repl;

	if (strchr(sectnametempl, '%'))
		throw wexception("sectnametempl %s contains %%", sectnametempl);

	snprintf(sectnamebase, sizeof(sectnamebase), "%s", sectnametempl);
	repl = strstr(sectnamebase, "??");
	if (!repl)
		throw wexception
			("DirAnimations section name template %s does not contain %%s",
			 sectnametempl);

	strncpy(repl, "%s", 2);

	if
		(char const * const string =
		 defaults ? defaults->get_string("dirpics", 0) : 0)
	{
		snprintf(dirpictempl, sizeof(dirpictempl), "%s", string);
		repl = strstr(dirpictempl, "!!");
		if (!repl)
			throw wexception
				("DirAnimations dirpics name templates %s does not contain !!",
				 dirpictempl);

		strncpy(repl, "%s", 2);
	} else {
		snprintf(dirpictempl, sizeof(dirpictempl), "%s_??", sectnamebase);
	}

	for (int32_t dir = 0; dir < 6; ++dir) {
		static char const * const dirstrings[6] =
			{"ne", "e", "se", "sw", "w", "nw"};
		char sectname[300];

		snprintf(sectname, sizeof(sectname), sectnamebase, dirstrings[dir]);

		string const anim_name = sectname;

		Section * s = prof.get_section(sectname);
		if (!s) {
			if (!defaults)
				throw wexception
					("Section [%s] missing and no default supplied",
					 sectname);

			s = defaults;
		}

		snprintf(sectname, sizeof(sectname), dirpictempl, dirstrings[dir]);

		// Fake the section name here, so that the animation loading code is
		// using the correct glob pattern to load the images from.
		s->set_name(sectname);
		m_animations[dir] = g_gr->animations().load(directory, *s);
		b.add_animation(anim_name.c_str(), m_animations[dir]);
	}
}

/*
==============================================================================

AnimationManager IMPLEMENTATION

==============================================================================
*/

uint32_t AnimationManager::load(const string& directory, Section & s) {
	if (s.get_bool("packed", false)) {
		m_animations.push_back(new PackedAnimation(directory, s));
	} else {
		m_animations.push_back(new NonPackedAnimation(directory, s));
	}
	const uint32_t id = m_animations.size();

	return id;
}

const Animation& AnimationManager::get_animation(uint32_t id) const
{
	if (!id || id > m_animations.size())
		throw wexception("Requested unknown animation with id: %i", id);

	return *m_animations[id - 1];
}



