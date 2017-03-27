/*******************************************************************************
 *
 *
 *    WashingtonDC Dreamcast Emulator
 *    Copyright (C) 2017 snickerbockers
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 ******************************************************************************/

#ifndef PVR2_CORE_REG_HPP_
#define PVR2_CORE_REG_HPP_

// vlck divider bit for the FB_R_CTRL register
const static unsigned PVR2_VCLK_DIV_SHIFT = 23;
const static reg32_t PVR2_VCLK_DIV_MASK = 1 << PVR2_VCLK_DIV_SHIFT;

// bit in the FB_R_CTRL register that causes each scanline to be sent twice
const static unsigned PVR2_LINE_DOUBLE_SHIFT = 1;
const static reg32_t PVR2_LINE_DOUBLE_MASK = 1 << PVR2_LINE_DOUBLE_SHIFT;

// bit in the VO_CONTROL register that causes each pixel to be sent twice
const static unsigned PVR2_PIXEL_DOUBLE_SHIFT = 8;
const static reg32_t PVR2_PIXEL_DOUBLE_MASK = 1 << PVR2_PIXEL_DOUBLE_SHIFT;

int pvr2_core_reg_read(void *buf, size_t addr, size_t len);
int pvr2_core_reg_write(void const *buf, size_t addr, size_t len);

#endif