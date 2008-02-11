/*
 * Copyright (C) 2002-2004, 2007-2008 by the Widelands Development Team
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

#include "network_buffer.h"

#include "wexception.h"

#include <cassert>

Network_Buffer::Network_Buffer() :
m_buffer_real_len(1000),
m_buffer_len     (0), // To correct for size entry
m_buffer_pointer (0),
m_buffer         (static_cast<uint8_t *>(malloc(1000)))
{
	put_16(0); //  this will become our size
}


Network_Buffer::~Network_Buffer() {free(m_buffer);}

/*
 * finish this buffer, this basically corrects
 * the size entry
 */
void Network_Buffer::finish() {
	SDLNet_Write16(m_buffer_len - 2, m_buffer);
}

/*
 * Get functions
 */
uint8_t Network_Buffer::get_8(bool remove) {
	uint8_t const result = *static_cast<uint8_t *>(m_buffer + m_buffer_pointer);

	if (remove)
		m_buffer_pointer += 1;
	return result;
}

uint16_t Network_Buffer::get_16(bool remove) {
	uint16_t const result = SDLNet_Read16(m_buffer + m_buffer_pointer);

	if (remove)
		m_buffer_pointer += 2;
	return result;
}

uint32_t Network_Buffer::get_32(bool remove) {
	uint32_t const result = SDLNet_Read32(m_buffer + m_buffer_pointer);

	if (remove)
		m_buffer_pointer += 4;
	return result;
}

std::string Network_Buffer::get_string(bool remove) {
	std::string result;

	assert(remove); // TODO: allow peeking also for string

	uint32_t const s = get_16(1);
	for (uint32_t i = 0; i < s; ++i)
		result.append(1, get_16(1));

	return result;
}

/*
 * Put functions
 */
void Network_Buffer::put_8(uint8_t val) {
	uint32_t const s = 1;

	while (m_buffer_pointer + s >= m_buffer_real_len)
		grow_buffer();

	*static_cast<uint8_t *>(m_buffer+m_buffer_pointer) = val;

	m_buffer_pointer += s;
	m_buffer_len     += s;
}

void Network_Buffer::put_16(uint16_t val) {
	uint32_t const s = 2;

	while (m_buffer_pointer + s >= m_buffer_real_len)
		grow_buffer();

	SDLNet_Write16(val, m_buffer+m_buffer_pointer);

	m_buffer_pointer += s;
	m_buffer_len += s;
}

void Network_Buffer::put_32(uint32_t val) {
	uint32_t const s = 4;

	while (m_buffer_pointer + s >= m_buffer_real_len)
		grow_buffer();

	SDLNet_Write32(val, m_buffer+m_buffer_pointer);

	m_buffer_pointer += s;
	m_buffer_len     += s;
}

void Network_Buffer::put_string(std::string const & string) {
	put_16(string.size());

	for (uint32_t i = 0; i < string.size(); ++i) {
		put_16(string[i]);
	}
}

/*
 * Fill this m_buffer
 *
 * return -1 on error
 */
int32_t Network_Buffer::fill(TCPsocket sock) {
	//  Ok, there is data on this socket. This is assumed.
	if (SDLNet_TCP_Recv(sock, m_buffer, 2) < 2) return -1; // Some error.

	m_buffer_pointer = 0; //  we reset the data pointer

	uint32_t s = get_16();

	//  check the s of the packet
	while (m_buffer_pointer + s >= m_buffer_real_len)
		grow_buffer();

	//  Get the rest of the packet
	//  TODO This should have some timeout variable, otherwise the game can lock
	//  TODO here (when some, but not all data arrives).
	uint32_t received = 0;
	while (received != s) {
		uint32_t const retval =
			SDLNet_TCP_Recv
			(sock, m_buffer + m_buffer_pointer + received, s - received);

		if (retval <= 0)
			throw wexception
				("Network_Buffer::fill: SDLNet_TCP_Recv brought up an error!: %s",
				 SDLNet_GetError());
		received += retval;
	}

	m_buffer_len += s;
	return 0;
}


void Network_Buffer::grow_buffer() {
	m_buffer_real_len += 1000;
	m_buffer = static_cast<uint8_t *>(realloc(m_buffer, m_buffer_real_len));
}
