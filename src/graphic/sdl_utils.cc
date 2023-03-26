/*
 * Copyright (C) 2006-2023 by the Widelands Development Team
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "graphic/sdl_utils.h"

#include <SDL_surface.h>

SDL_Surface* empty_sdl_surface(int16_t w, int16_t h) {
	SDL_Surface* const surface =
#if 0
#if !defined(SDL_BYTEORDER) || SDL_BYTEORDER == SDL_LIL_ENDIAN
	   SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#else
	   SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#endif
#else
	   SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0b1111111111, 0b11111111110000000000, 0b111111111100000000000000000000, 0b11000000000000000000000000000000);
#endif
	return surface;
}
