# ref: https://github.com/feliam/mkShellcode
import sys
import struct
from StringIO import StringIO

sys.path.append('mkShellcode/pyelftools/build/lib.linux-x86_64-2.7')
from elftools.elf.elffile import ELFFile,RelocationSection,SymbolTableSection
from elftools.elf.enums import ENUM_RELOC_TYPE_i386, ENUM_RELOC_TYPE_x64

def find_relocations_for_section(elf, section_name):
    """ Given a section, find the relocation section for it in the ELF
        file. Return a RelocationSection object, or None if none was
        found.
    """
    rels = elf.get_section_by_name(b'.rel' + section_name)
    if rels is None:
        rels = elf.get_section_by_name(b'.rela' + section_name)
    return rels

def main():
    filename = sys.argv[1]
    elf = ELFFile(file(filename))
    print('[II] Object %s is a %s_%s elf' % (filename, elf.get_machine_arch(), elf.elfclass))
    assert elf.elfclass == 64 and elf.get_machine_arch() == 'x64'

    print "[II] Elf has %d sections."% elf.num_sections()

    selected_sections = [".text",".data"]
    for section in elf.iter_sections():
      if section.name.startswith(".rodata"):
        selected_sections.append(section.name)
    selected_sections.append('.bss')

    offsets = {}
    shellcode = StringIO('')
    for section_name in selected_sections:
        offsets[section_name] = shellcode.len
        try:
            s = elf.get_section_by_name(section_name)
            if s['sh_type'] == 'SHT_NOBITS':
                data = chr(0) * s['sh_size']
            else:
                data = elf.get_section_by_name(section_name).data()
            print "[II] Section %s is %d bytes offset %d"%(section_name,len(data),offsets[section_name])
        except:
            data = ''
            print '[WW] No %s section'%section_name
        shellcode.write(data)
        # padding to 16
        shellcode.write(chr(0) * (16-shellcode.len % 16))

    print "[II] Total packed data size %d" % shellcode.len

    relocs = []
    for section_name in selected_sections:
        reloc_section = find_relocations_for_section(elf, section_name)
        if reloc_section is None:
            continue
        symtab = elf.get_section(reloc_section['sh_link'])
        for reloc in reloc_section.iter_relocations():
            print reloc
            #assert elf.get_machine_arch() == 'x64' and not reloc.is_RELA()
            assert elf.get_machine_arch() == 'x64' and reloc.is_RELA()
            reloc_base = offsets[section_name]
            reloc_offset = reloc['r_offset']
            reloc_type = reloc['r_info_type']
            target_symbol = symtab.get_symbol(reloc['r_info_sym'])
            target_name = elf.get_section(target_symbol['st_shndx']).name
            target_base = offsets[target_name]
            target_offset = target_symbol['st_value']

            shellcode.seek(reloc_base+reloc_offset)
            value = struct.unpack("<l",shellcode.read(4))[0]     #+ reloc['r_addend']
            print "RELOC:",section_name, '0x%x' % reloc_base, '0x%x' % reloc_offset, "=>", target_name, '0x%x' % target_base,'0x%x' % target_offset, value, '(%s)' % target_symbol.name
            if reloc_type == ENUM_RELOC_TYPE_x64['R_X86_64_32']:
                value = target_base + target_offset + value + reloc['r_addend']
                relocs.append(reloc_base+reloc_offset)
                print "[II] Offset ",reloc_base+reloc_offset, "added to reloc list"
            elif reloc_type == ENUM_RELOC_TYPE_x64['R_X86_64_PC32']:
                value = (target_base + target_offset) -  (reloc_base + reloc_offset) + value + reloc['r_addend']

            elif reloc_type == ENUM_RELOC_TYPE_x64['R_X86_64_32S']:
                value = target_base + target_offset + value+ reloc['r_addend']
                relocs.append(reloc_base+reloc_offset)
            else:
                assert reloc_type == ENUM_RELOC_TYPE_x64['R_X86_64_NONE']
            shellcode.seek(reloc_base + reloc_offset)
            shellcode.write(struct.pack("<L",value&0xffffffff))
        shellcode.seek(shellcode.len)

    def to_c_array(s):
        if len(s) % 4 != 0:
            s += chr(0) * (4 - len(s) % 4)
        bs = map(ord, s)
        result = ''
        for i in range(0, len(bs), 8):
            result += ' ' + ''.join(' 0x%02x,' % b for b in bs[i:i+8]) + '\n'
        return result

    def to_c_array2(arr):
        result = ''
        for i in range(0, len(arr), 10):
            result += ' ' + ''.join(' %d,' % x for x in arr[i:i+10]) + '\n'
        return result

    with file('bot_opt.cc', 'w') as fp:
        bss_size = elf.get_section_by_name('.bss')['sh_size']
        assert shellcode.getvalue()[-bss_size:] == chr(0) * bss_size

        pagesize = 4096

        fp.write('''#include <sys/mman.h>
#include "bot_opt.h"

static unsigned char code[%d] __attribute__((aligned(4096))) = {
%s};
static int patch[] = {
%s};
''' % (
        (shellcode.len + pagesize-1) / pagesize * pagesize,
        to_c_array(shellcode.getvalue()[:-bss_size]),
        to_c_array2(relocs),
        ))
    
            #fp.write('reloc %d\n' % rel)
        fp.write('''
void load_code() {
  if ((uintptr_t)code > 0xffffffffull)
    return;
  mprotect(code, sizeof(code), PROT_READ|PROT_WRITE|PROT_EXEC);
  for (unsigned int i = 0; i < sizeof(patch)/sizeof(patch[0]); i++) {
    *(uint32_t*)(void*)(code + patch[i]) += (uintptr_t)code;
  }
''')

    #with file('bot_opt.bin', 'wb') as fp:
    #    fp.write(shellcode.getvalue())

    
        # export symbols
        for entry in (
                'root_search_move',
                'init_bot',
                'max_lookahead',
                ):
            symbol = None
            for s in elf.get_section_by_name('.symtab').iter_symbols():
                if s.name == entry:
                    symbol = s
            assert symbol
            section = elf.get_section(symbol['st_shndx']).name
            base = offsets[section]
            offset = symbol['st_value']
            
            start = base + offset
            print section, entry, start
            if section == '.text':
                fp.write('  %s_func = (%s_func_t)((char*)code + %d);\n' % (
                    entry, entry, start))
            else:
                fp.write('  %s_ptr = (%s_ptr_t)((char*)code + %d);\n' % (
                    entry, entry, start))
            #fp.write('%s %s %d\n' % (section, entry, start))


        fp.write('''}
''')

if __name__ == '__main__':
    main()
