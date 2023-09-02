#include <iostream>
#include "elf.hpp"

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << std::filesystem::path(argv[0]).filename().string() << " <lib>" << std::endl;
    return 1;
  }

  std::filesystem::path lib_path(argv[1]);

  elf::elf_file lib(lib_path);
  if (lib.error())
  {
    std::cerr << "Error loading lib: " << lib.error_message() << std::endl;
    return 1;
  }
  std::cout << "Loaded lib from file" << std::endl;

  const auto &header = lib.get_header();
  std::cout << "ELF header:\n" \
 << "  Class: " << (int) header.e_ident.ei_class << "\n" \
 << "  Data: " << (int) header.e_ident.ei_data << "\n" \
 << "  Version: " << (int) header.e_ident.ei_version << "\n" \
 << "  OS/ABI: " << (int) header.e_ident.ei_osabi << "\n" \
 << "  ABI Version: " << (int) header.e_ident.ei_abiversion << "\n" \
 << "  Type: " << header.e_type << "\n" \
 << "  Machine: " << header.e_machine << "\n" \
 << "  Version: " << header.e_version << "\n" \
 << "  Entry point address: " << header.e_entry << "\n" \
 << "  Flags: " << header.e_flags << "\n" \
 << "  Size of this header: " << header.e_ehsize << "\n" \
 << "  Is 32-bit: " << lib.is_32_bit() << "\n" \
 << "  Is 64-bit: " << lib.is_64_bit() << "\n" \
 << "  Is little-endian: " << lib.is_little_endian() << "\n" \
 << "  Is big-endian: " << lib.is_big_endian() << "\n";

  const auto &program_headers = lib.get_program_headers();
  std::cout << "Program headers: Start=" << header.e_phoff << " Count=" << header.e_phnum << " Size=" << header.e_phentsize << "\n";
  for (std::size_t i = 0; i < program_headers.size(); i++)
  {
    const auto &program_header = program_headers[i];
    std::cout \
 << "  [" << i << "]\n" \
 << "    Type: " << program_header.p_type << "\n" \
 << "    Flags: " << program_header.p_flags << "\n" \
 << "    Offset: " << program_header.p_offset << "\n" \
 << "    Virtual address: " << program_header.p_vaddr << "\n" \
 << "    Physical address: " << program_header.p_paddr << "\n" \
 << "    File size: " << program_header.p_filesz << "\n" \
 << "    Memory size: " << program_header.p_memsz << "\n" \
 << "    Alignment: " << program_header.p_align << "\n";
  }

  const auto &section_headers = lib.get_section_headers();
  std::cout << "Section headers: Start=" << header.e_shoff << " Count=" << header.e_shnum << " Size=" << header.e_shentsize << "\n";
  for (std::size_t i = 0; i < section_headers.size(); i++)
  {
    const auto &section_header = section_headers[i];
    std::cout \
 << "  [" << i << "] " << section_header.sh_name_str << "\n" \
 << "    Name: " << section_header.sh_name << "\n" \
 << "    Type: " << section_header.sh_type << "\n" \
 << "    Flags: " << section_header.sh_flags << "\n" \
 << "    Address: " << section_header.sh_addr << "\n" \
 << "    Offset: " << section_header.sh_offset << "\n" \
 << "    Size: " << section_header.sh_size << "\n" \
 << "    Link: " << section_header.sh_link << "\n" \
 << "    Info: " << section_header.sh_info << "\n" \
 << "    Address alignment: " << section_header.sh_addralign << "\n" \
 << "    Entry size: " << section_header.sh_entsize << "\n";
  }

  if (!lib.parse_dynamic_segment())
  {
    std::cerr << "Failed to parse dynamic segment: " << lib.error_message() << std::endl;
    return 1;
  }

  std::cout << "Dynamic segment:\n" \
 << "  SO Name: " << lib.get_so_name() << "\n";
  for (const auto &needed_lib: lib.get_needed_libraries())
  {
    std::cout << "  Needed lib: " << needed_lib << "\n";
  }
  std::cout << "  Dynamic symbols count: " << lib.get_dynamic_symbols().size() << "\n";
  std::cout \
  << "  Relocations without addend: " << lib.get_relocations().size() << "\n" \
  << "  Relocations with addend: " << lib.get_relocations_with_addend().size() << "\n" \
  << "  PLT relocations without addend: " << lib.get_plt_relocations().size() << "\n" \
  << "  PLT relocations with addend: " << lib.get_plt_relocations_with_addend().size() << "\n";
  if (lib.get_symbol("thisisnotasymbol 1337") != lib.get_dynamic_symbols().cend())
  {
    std::cerr << "Found symbol that should not exist" << std::endl;
    return 1;
  }
  std::cout << "  FairPlaySAPSign: " << lib.get_symbol("Fc3vhtJDvr")->st_value << "\n";
}
