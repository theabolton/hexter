/* hexter DSSI software synthesizer text-mode UI
 *
 * Copyright (C) 2004 Sean Bolton.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

void readline_callback(char *line);
int  test_note_callback(int a, int b);
void update_from_program_select(unsigned long bank, unsigned long program);
void update_patches(const char *key, const char *value);
void update_monophonic(const char *value);
void update_polyphony(const char *value);
void update_global_polyphony(const char *value);

