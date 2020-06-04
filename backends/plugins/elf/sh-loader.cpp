/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/scummsys.h"

#if defined(DYNAMIC_MODULES) && defined(USE_ELF_LOADER) && defined(SH_TARGET)

#include "backends/plugins/elf/elf-loader.h"
#include "backends/plugins/elf/sh-loader.h"

#include "common/debug.h"

bool SHDLObject::relocate(Elf32_Off offset, Elf32_Word size, byte *relSegment) {
	Elf32_Rela *rel = NULL;

	if (!(rel = (Elf32_Rela *)malloc(size))) {
		warning("elfloader: Out of memory.");
		return false;
	}

	if (!_file->seek(offset, SEEK_SET) || _file->read(rel, size) != size) {
		warning("elfloader: Relocation table load failed.");
		free(rel);
		return false;
	}

	uint32 cnt = size / sizeof(*rel);

	debug(2, "elfloader: Loaded relocation table. %d entries. base address=%p", cnt, relSegment);

	uint32 *src;

	for (uint32 i = 0; i < cnt; i++) {
		// Get the symbol this relocation entry is referring to
		Elf32_Sym *sym = _symtab + (REL_INDEX(rel[i].r_info));

		// Get the target instruction in the code
		src = (uint32 *)((char *)relSegment + rel[i].r_offset - _segmentVMA);
		switch (REL_TYPE(rel[i].r_info)) {
		case R_SH_DIR32:
			if (sym->st_shndx != SHN_ABS) {
				*src += relSegment;
			}
			break;
		default:
			warning("elfloader: Unknown relocation type %d", REL_TYPE(rel[i].r_info));
			free(rel);
			return false;
		}
	}

	free(rel);
	return true;
}

bool SHDLObject::relocateRels(Elf32_Ehdr *ehdr, Elf32_Shdr *shdr) {
	for (uint32 i = 0; i < ehdr->e_shnum; i++) {
		Elf32_Shdr *curShdr = &(shdr[i]);

		if ((curShdr->sh_type == SHT_REL) &&
				curShdr->sh_entsize == sizeof(Elf32_Rel) &&
				int32(curShdr->sh_link) == _symtab_sect &&
				curShdr->sh_info < ehdr->e_shnum &&
				(shdr[curShdr->sh_info].sh_flags & SHF_ALLOC)) {
			warning("elfloader: REL entries not supported!");
			return false;
		}

		if ((curShdr->sh_type == SHT_RELA) &&
				curShdr->sh_entsize == sizeof(Elf32_Rela) &&
				int32(curShdr->sh_link) == _symtab_sect &&
				curShdr->sh_info < ehdr->e_shnum &&
				(shdr[curShdr->sh_info].sh_flags & SHF_ALLOC)) {
			if (!relocate(curShdr->sh_offset, curShdr->sh_size, _segment))
				return false;
		}
	}

	return true;
}

#endif /* defined(DYNAMIC_MODULES) && defined(USE_ELF_LOADER) && defined(SH_TARGET) */
