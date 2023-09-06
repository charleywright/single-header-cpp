/*
 * ISC License
 *
 * Copyright 2023 Charley Wright
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
 * granted, provided that the above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>
#include <cstdint>
#include <variant>
#include <cstring>
#include <algorithm>

/*
 * References:
 * https://man7.org/linux/man-pages/man5/elf.5.html
 * https://refspecs.linuxbase.org/elf/gabi4+/ch4.intro.html
 * https://refspecs.linuxbase.org/elf/gabi4+/ch4.eheader.html
 * https://refspecs.linuxbase.org/elf/gabi4+/ch4.sheader.html
 * https://refspecs.linuxbase.org/elf/elf.pdf
 * https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf
 * https://github.com/bminor/glibc/blob/master/elf/elf.h
 * https://docs.oracle.com/cd/E53394_01/pdf/E54813.pdf
 * https://blogs.oracle.com/solaris/post/gnu-hash-elf-sections
 * https://sourceware.org/legacy-ml/binutils/2006-10/msg00377.html
 * https://akkadia.org/drepper/dsohowto.pdf
 * https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-54839.html
 * https://raw.githubusercontent.com/wiki/hjl-tools/x86-psABI/intel386-psABI-1.1.pdf
 *
 * TODO:
 * - Properly handle endianness
 * - Evaluate std::variant instead of always storing 64-bit values
 */

namespace elf
{
    typedef std::uint8_t byte;

    /*
     * All of our bitness-agnostic structures have the same initial layout as
     * the 64-bit variants, allowing us to skip a copy when loading a 64-bit
     * file. This means any new fields must be added at the end of the struct.
     */

    typedef struct elf_ident
    {
        inline static constexpr byte EI_MAG0 = 0;
        inline static constexpr byte ELFMAG0 = 0x7f;
        inline static constexpr byte EI_MAG1 = 1;
        inline static constexpr byte ELFMAG1 = 'E';
        inline static constexpr byte EI_MAG2 = 2;
        inline static constexpr byte ELFMAG2 = 'L';
        inline static constexpr byte EI_MAG3 = 3;
        inline static constexpr byte ELFMAG3 = 'F';
        ::elf::byte ei_magic[4];  /* File identification */

        inline static constexpr byte ELFCLASSNONE = 0;  /* Invalid class */
        inline static constexpr byte ELFCLASS32 = 1;    /* 32-bit objects */
        inline static constexpr byte ELFCLASS64 = 2;    /* 64-bit objects */
        ::elf::byte ei_class;  /* File class */

        inline static constexpr byte ELFDATANONE = 0;   /* Invalid data encoding */
        inline static constexpr byte ELFDATA2LSB = 1;   /* 2's complement, little endian */
        inline static constexpr byte ELFDATA2MSB = 2;   /* 2's complement, big endian */
        ::elf::byte ei_data;  /* Data encoding */

        inline static constexpr byte EV_NONE = 0;       /* Invalid version */
        inline static constexpr byte EV_CURRENT = 1;    /* Current version */
        ::elf::byte ei_version;  /* File version */

        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L135-L150
        inline static constexpr byte ELFOSABI_NONE = 0;              /* UNIX System V ABI */
        inline static constexpr byte ELFOSABI_SYSV = 0;              /* Alias.  */
        inline static constexpr byte ELFOSABI_HPUX = 1;              /* HP-UX */
        inline static constexpr byte ELFOSABI_NETBSD = 2;            /* NetBSD.  */
        inline static constexpr byte ELFOSABI_GNU = 3;               /* Object uses GNU ELF extensions.  */
        inline static constexpr byte ELFOSABI_LINUX = ELFOSABI_GNU;  /* Compatibility alias.  */
        inline static constexpr byte ELFOSABI_SOLARIS = 6;           /* Sun Solaris.  */
        inline static constexpr byte ELFOSABI_AIX = 7;               /* IBM AIX.  */
        inline static constexpr byte ELFOSABI_IRIX = 8;              /* SGI Irix.  */
        inline static constexpr byte ELFOSABI_FREEBSD = 9;           /* FreeBSD.  */
        inline static constexpr byte ELFOSABI_TRU64 = 10;            /* Compaq TRU64 UNIX.  */
        inline static constexpr byte ELFOSABI_MODESTO = 11;          /* Novell Modesto.  */
        inline static constexpr byte ELFOSABI_OPENBSD = 12;          /* OpenBSD.  */
        inline static constexpr byte ELFOSABI_ARM_AEABI = 64;        /* ARM EABI */
        inline static constexpr byte ELFOSABI_ARM = 97;              /* ARM */
        inline static constexpr byte ELFOSABI_STANDALONE = 255;      /* Standalone (embedded) application */
        ::elf::byte ei_osabi;  /* OS-specific ELF extensions */

        inline static constexpr byte ELFAABIVERSION_UNSPECIFIED = 0; /* Unspecified */
        // Other values are OSABI-specific.
        ::elf::byte ei_abiversion;  /* ABI version */

        ::elf::byte ei_pad[7];
    } elf_ident;

    // Same layout as Elf64_Ehdr for optimization
    typedef struct elf_header
    {
        ::elf::elf_ident e_ident;  /* Machine-independent identification */

        inline static constexpr std::uint16_t ET_NONE = 0;         /* No file type */
        inline static constexpr std::uint16_t ET_REL = 1;          /* Relocatable file */
        inline static constexpr std::uint16_t ET_EXEC = 2;         /* Executable file */
        inline static constexpr std::uint16_t ET_DYN = 3;          /* Shared object file */
        inline static constexpr std::uint16_t ET_CORE = 4;         /* Core file */
        inline static constexpr std::uint16_t ET_LOOS = 0xfe00;    /* Operating system-specific */
        inline static constexpr std::uint16_t ET_HIOS = 0xfeff;    /* Operating system-specific */
        inline static constexpr std::uint16_t ET_LOPROC = 0xff00;  /* Processor-specific */
        inline static constexpr std::uint16_t ET_HIPROC = 0xffff;  /* Processor-specific */
        std::uint16_t e_type;  /* Object file type */

        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L169-L373
        inline static constexpr std::uint16_t EM_NONE = 0;             /* No machine */
        inline static constexpr std::uint16_t EM_M32 = 1;              /* AT&T WE 32100 */
        inline static constexpr std::uint16_t EM_SPARC = 2;            /* SUN SPARC */
        inline static constexpr std::uint16_t EM_386 = 3;              /* Intel 80386 */
        inline static constexpr std::uint16_t EM_68K = 4;              /* Motorola m68k family */
        inline static constexpr std::uint16_t EM_88K = 5;              /* Motorola m88k family */
        inline static constexpr std::uint16_t EM_IAMCU = 6;            /* Intel MCU */
        inline static constexpr std::uint16_t EM_860 = 7;              /* Intel 80860 */
        inline static constexpr std::uint16_t EM_MIPS = 8;             /* MIPS R3000 big-endian */
        inline static constexpr std::uint16_t EM_S370 = 9;             /* IBM System/370 */
        inline static constexpr std::uint16_t EM_MIPS_RS3_LE = 10;     /* MIPS R3000 little-endian */
        /* reserved 11-14 */
        inline static constexpr std::uint16_t EM_PARISC = 15;          /* HPPA */
        /* reserved 16 */
        inline static constexpr std::uint16_t EM_VPP500 = 17;          /* Fujitsu VPP500 */
        inline static constexpr std::uint16_t EM_SPARC32PLUS = 18;     /* Sun's "v8plus" */
        inline static constexpr std::uint16_t EM_960 = 19;             /* Intel 80960 */
        inline static constexpr std::uint16_t EM_PPC = 20;             /* PowerPC */
        inline static constexpr std::uint16_t EM_PPC64 = 21;           /* PowerPC 64-bit */
        inline static constexpr std::uint16_t EM_S390 = 22;            /* IBM S390 */
        inline static constexpr std::uint16_t EM_SPU = 23;             /* IBM SPU/SPC */
        /* reserved 24-35 */
        inline static constexpr std::uint16_t EM_V800 = 36;            /* NEC V800 series */
        inline static constexpr std::uint16_t EM_FR20 = 37;            /* Fujitsu FR20 */
        inline static constexpr std::uint16_t EM_RH32 = 38;            /* TRW RH-32 */
        inline static constexpr std::uint16_t EM_RCE = 39;             /* Motorola RCE */
        inline static constexpr std::uint16_t EM_ARM = 40;             /* ARM */
        inline static constexpr std::uint16_t EM_FAKE_ALPHA = 41;      /* Digital Alpha */
        inline static constexpr std::uint16_t EM_SH = 42;              /* Hitachi SH */
        inline static constexpr std::uint16_t EM_SPARCV9 = 43;         /* SPARC v9 64-bit */
        inline static constexpr std::uint16_t EM_TRICORE = 44;         /* Siemens Tricore */
        inline static constexpr std::uint16_t EM_ARC = 45;             /* Argonaut RISC Core */
        inline static constexpr std::uint16_t EM_H8_300 = 46;          /* Hitachi H8/300 */
        inline static constexpr std::uint16_t EM_H8_300H = 47;         /* Hitachi H8/300H */
        inline static constexpr std::uint16_t EM_H8S = 48;             /* Hitachi H8S */
        inline static constexpr std::uint16_t EM_H8_500 = 49;          /* Hitachi H8/500 */
        inline static constexpr std::uint16_t EM_IA_64 = 50;           /* Intel Merced */
        inline static constexpr std::uint16_t EM_MIPS_X = 51;          /* Stanford MIPS-X */
        inline static constexpr std::uint16_t EM_COLDFIRE = 52;        /* Motorola Coldfire */
        inline static constexpr std::uint16_t EM_68HC12 = 53;          /* Motorola M68HC12 */
        inline static constexpr std::uint16_t EM_MMA = 54;             /* Fujitsu MMA Multimedia Accelerator */
        inline static constexpr std::uint16_t EM_PCP = 55;             /* Siemens PCP */
        inline static constexpr std::uint16_t EM_NCPU = 56;            /* Sony nCPU embedded RISC */
        inline static constexpr std::uint16_t EM_NDR1 = 57;            /* Denso NDR1 microprocessor */
        inline static constexpr std::uint16_t EM_STARCORE = 58;        /* Motorola Start*Core processor */
        inline static constexpr std::uint16_t EM_ME16 = 59;            /* Toyota ME16 processor */
        inline static constexpr std::uint16_t EM_ST100 = 60;           /* STMicroelectronic ST100 processor */
        inline static constexpr std::uint16_t EM_TINYJ = 61;           /* Advanced Logic Corp. Tinyj emb.fam */
        inline static constexpr std::uint16_t EM_X86_64 = 62;          /* AMD x86-64 architecture */
        inline static constexpr std::uint16_t EM_PDSP = 63;            /* Sony DSP Processor */
        inline static constexpr std::uint16_t EM_PDP10 = 64;           /* Digital PDP-10 */
        inline static constexpr std::uint16_t EM_PDP11 = 65;           /* Digital PDP-11 */
        inline static constexpr std::uint16_t EM_FX66 = 66;            /* Siemens FX66 microcontroller */
        inline static constexpr std::uint16_t EM_ST9PLUS = 67;         /* STMicroelectronics ST9+ 8/16 mc */
        inline static constexpr std::uint16_t EM_ST7 = 68;             /* STmicroelectronics ST7 8 bit mc */
        inline static constexpr std::uint16_t EM_68HC16 = 69;          /* Motorola MC68HC16 microcontroller */
        inline static constexpr std::uint16_t EM_68HC11 = 70;          /* Motorola MC68HC11 microcontroller */
        inline static constexpr std::uint16_t EM_68HC08 = 71;          /* Motorola MC68HC08 microcontroller */
        inline static constexpr std::uint16_t EM_68HC05 = 72;          /* Motorola MC68HC05 microcontroller */
        inline static constexpr std::uint16_t EM_SVX = 73;             /* Silicon Graphics SVx */
        inline static constexpr std::uint16_t EM_ST19 = 74;            /* STMicroelectronics ST19 8 bit mc */
        inline static constexpr std::uint16_t EM_VAX = 75;             /* Digital VAX */
        inline static constexpr std::uint16_t EM_CRIS = 76;            /* Axis Communications 32-bit emb.proc */
        inline static constexpr std::uint16_t EM_JAVELIN = 77;         /* Infineon Technologies 32-bit emb.proc */
        inline static constexpr std::uint16_t EM_FIREPATH = 78;        /* Element 14 64-bit DSP Processor */
        inline static constexpr std::uint16_t EM_ZSP = 79;             /* LSI Logic 16-bit DSP Processor */
        inline static constexpr std::uint16_t EM_MMIX = 80;            /* Donald Knuth's educational 64-bit proc */
        inline static constexpr std::uint16_t EM_HUANY = 81;           /* Harvard University machine-independent object files */
        inline static constexpr std::uint16_t EM_PRISM = 82;           /* SiTera Prism */
        inline static constexpr std::uint16_t EM_AVR = 83;             /* Atmel AVR 8-bit microcontroller */
        inline static constexpr std::uint16_t EM_FR30 = 84;            /* Fujitsu FR30 */
        inline static constexpr std::uint16_t EM_D10V = 85;            /* Mitsubishi D10V */
        inline static constexpr std::uint16_t EM_D30V = 86;            /* Mitsubishi D30V */
        inline static constexpr std::uint16_t EM_V850 = 87;            /* NEC v850 */
        inline static constexpr std::uint16_t EM_M32R = 88;            /* Mitsubishi M32R */
        inline static constexpr std::uint16_t EM_MN10300 = 89;         /* Matsushita MN10300 */
        inline static constexpr std::uint16_t EM_MN10200 = 90;         /* Matsushita MN10200 */
        inline static constexpr std::uint16_t EM_PJ = 91;              /* picoJava */
        inline static constexpr std::uint16_t EM_OPENRISC = 92;        /* OpenRISC 32-bit embedded processor */
        inline static constexpr std::uint16_t EM_ARC_COMPACT = 93;     /* ARC International ARCompact */
        inline static constexpr std::uint16_t EM_XTENSA = 94;          /* Tensilica Xtensa Architecture */
        inline static constexpr std::uint16_t EM_VIDEOCORE = 95;       /* Alphamosaic VideoCore */
        inline static constexpr std::uint16_t EM_TMM_GPP = 96;         /* Thompson Multimedia General Purpose Proc */
        inline static constexpr std::uint16_t EM_NS32K = 97;           /* National Semi. 32000 */
        inline static constexpr std::uint16_t EM_TPC = 98;             /* Tenor Network TPC */
        inline static constexpr std::uint16_t EM_SNP1K = 99;           /* Trebia SNP 1000 */
        inline static constexpr std::uint16_t EM_ST200 = 100;          /* STMicroelectronics ST200 */
        inline static constexpr std::uint16_t EM_IP2K = 101;           /* Ubicom IP2xxx */
        inline static constexpr std::uint16_t EM_MAX = 102;            /* MAX processor */
        inline static constexpr std::uint16_t EM_CR = 103;             /* National Semi. CompactRISC */
        inline static constexpr std::uint16_t EM_F2MC16 = 104;         /* Fujitsu F2MC16 */
        inline static constexpr std::uint16_t EM_MSP430 = 105;         /* Texas Instruments msp430 */
        inline static constexpr std::uint16_t EM_BLACKFIN = 106;       /* Analog Devices Blackfin DSP */
        inline static constexpr std::uint16_t EM_SE_C33 = 107;         /* Seiko Epson S1C33 family */
        inline static constexpr std::uint16_t EM_SEP = 108;            /* Sharp embedded microprocessor */
        inline static constexpr std::uint16_t EM_ARCA = 109;           /* Arca RISC */
        inline static constexpr std::uint16_t EM_UNICORE = 110;        /* PKU-Unity & MPRC Peking Uni. mc series */
        inline static constexpr std::uint16_t EM_EXCESS = 111;         /* eXcess configurable cpu */
        inline static constexpr std::uint16_t EM_DXP = 112;            /* Icera Semi. Deep Execution Processor */
        inline static constexpr std::uint16_t EM_ALTERA_NIOS2 = 113;   /* Altera Nios II */
        inline static constexpr std::uint16_t EM_CRX = 114;            /* National Semi. CompactRISC CRX */
        inline static constexpr std::uint16_t EM_XGATE = 115;          /* Motorola XGATE */
        inline static constexpr std::uint16_t EM_C166 = 116;           /* Infineon C16x/XC16x */
        inline static constexpr std::uint16_t EM_M16C = 117;           /* Renesas M16C */
        inline static constexpr std::uint16_t EM_DSPIC30F = 118;       /* Microchip Technology dsPIC30F */
        inline static constexpr std::uint16_t EM_CE = 119;             /* Freescale Communication Engine RISC */
        inline static constexpr std::uint16_t EM_M32C = 120;           /* Renesas M32C */
        /* reserved 121-130 */
        inline static constexpr std::uint16_t EM_TSK3000 = 131;        /* Altium TSK3000 */
        inline static constexpr std::uint16_t EM_RS08 = 132;           /* Freescale RS08 */
        inline static constexpr std::uint16_t EM_SHARC = 133;          /* Analog Devices SHARC family */
        inline static constexpr std::uint16_t EM_ECOG2 = 134;          /* Cyan Technology eCOG2 */
        inline static constexpr std::uint16_t EM_SCORE7 = 135;         /* Sunplus S+core7 RISC */
        inline static constexpr std::uint16_t EM_DSP24 = 136;          /* New Japan Radio (NJR) 24-bit DSP */
        inline static constexpr std::uint16_t EM_VIDEOCORE3 = 137;     /* Broadcom VideoCore III */
        inline static constexpr std::uint16_t EM_LATTICEMICO32 = 138;  /* RISC for Lattice FPGA */
        inline static constexpr std::uint16_t EM_SE_C17 = 139;         /* Seiko Epson C17 */
        inline static constexpr std::uint16_t EM_TI_C6000 = 140;       /* Texas Instruments TMS320C6000 DSP */
        inline static constexpr std::uint16_t EM_TI_C2000 = 141;       /* Texas Instruments TMS320C2000 DSP */
        inline static constexpr std::uint16_t EM_TI_C5500 = 142;       /* Texas Instruments TMS320C55x DSP */
        inline static constexpr std::uint16_t EM_TI_ARP32 = 143;       /* Texas Instruments App. Specific RISC */
        inline static constexpr std::uint16_t EM_TI_PRU = 144;         /* Texas Instruments Prog. Realtime Unit */
        /* reserved 145-159 */
        inline static constexpr std::uint16_t EM_MMDSP_PLUS = 160;     /* STMicroelectronics 64bit VLIW DSP */
        inline static constexpr std::uint16_t EM_CYPRESS_M8C = 161;    /* Cypress M8C */
        inline static constexpr std::uint16_t EM_R32C = 162;           /* Renesas R32C */
        inline static constexpr std::uint16_t EM_TRIMEDIA = 163;       /* NXP Semi. TriMedia */
        inline static constexpr std::uint16_t EM_QDSP6 = 164;          /* QUALCOMM DSP6 */
        inline static constexpr std::uint16_t EM_8051 = 165;           /* Intel 8051 and variants */
        inline static constexpr std::uint16_t EM_STXP7X = 166;         /* STMicroelectronics STxP7x */
        inline static constexpr std::uint16_t EM_NDS32 = 167;          /* Andes Tech. compact code emb. RISC */
        inline static constexpr std::uint16_t EM_ECOG1X = 168;         /* Cyan Technology eCOG1X */
        inline static constexpr std::uint16_t EM_MAXQ30 = 169;         /* Dallas Semi. MAXQ30 mc */
        inline static constexpr std::uint16_t EM_XIMO16 = 170;         /* New Japan Radio (NJR) 16-bit DSP */
        inline static constexpr std::uint16_t EM_MANIK = 171;          /* M2000 Reconfigurable RISC */
        inline static constexpr std::uint16_t EM_CRAYNV2 = 172;        /* Cray NV2 vector architecture */
        inline static constexpr std::uint16_t EM_RX = 173;             /* Renesas RX */
        inline static constexpr std::uint16_t EM_METAG = 174;          /* Imagination Tech. META */
        inline static constexpr std::uint16_t EM_MCST_ELBRUS = 175;    /* MCST Elbrus */
        inline static constexpr std::uint16_t EM_ECOG16 = 176;         /* Cyan Technology eCOG16 */
        inline static constexpr std::uint16_t EM_CR16 = 177;           /* National Semi. CompactRISC CR16 */
        inline static constexpr std::uint16_t EM_ETPU = 178;           /* Freescale Extended Time Processing Unit */
        inline static constexpr std::uint16_t EM_SLE9X = 179;          /* Infineon Tech. SLE9X */
        inline static constexpr std::uint16_t EM_L10M = 180;           /* Intel L10M */
        inline static constexpr std::uint16_t EM_K10M = 181;           /* Intel K10M */
        /* reserved 182 */
        inline static constexpr std::uint16_t EM_AARCH64 = 183;        /* ARM AARCH64 */
        /* reserved 184 */
        inline static constexpr std::uint16_t EM_AVR32 = 185;          /* Amtel 32-bit microprocessor */
        inline static constexpr std::uint16_t EM_STM8 = 186;           /* STMicroelectronics STM8 */
        inline static constexpr std::uint16_t EM_TILE64 = 187;         /* Tilera TILE64 */
        inline static constexpr std::uint16_t EM_TILEPRO = 188;        /* Tilera TILEPro */
        inline static constexpr std::uint16_t EM_MICROBLAZE = 189;     /* Xilinx MicroBlaze */
        inline static constexpr std::uint16_t EM_CUDA = 190;           /* NVIDIA CUDA */
        inline static constexpr std::uint16_t EM_TILEGX = 191;         /* Tilera TILE-Gx */
        inline static constexpr std::uint16_t EM_CLOUDSHIELD = 192;    /* CloudShield */
        inline static constexpr std::uint16_t EM_COREA_1ST = 193;      /* KIPO-KAIST Core-A 1st gen. */
        inline static constexpr std::uint16_t EM_COREA_2ND = 194;      /* KIPO-KAIST Core-A 2nd gen. */
        inline static constexpr std::uint16_t EM_ARCV2 = 195;          /* Synopsys ARCv2 ISA.  */
        inline static constexpr std::uint16_t EM_OPEN8 = 196;          /* Open8 RISC */
        inline static constexpr std::uint16_t EM_RL78 = 197;           /* Renesas RL78 */
        inline static constexpr std::uint16_t EM_VIDEOCORE5 = 198;     /* Broadcom VideoCore V */
        inline static constexpr std::uint16_t EM_78KOR = 199;          /* Renesas 78KOR */
        inline static constexpr std::uint16_t EM_56800EX = 200;        /* Freescale 56800EX DSC */
        inline static constexpr std::uint16_t EM_BA1 = 201;            /* Beyond BA1 */
        inline static constexpr std::uint16_t EM_BA2 = 202;            /* Beyond BA2 */
        inline static constexpr std::uint16_t EM_XCORE = 203;          /* XMOS xCORE */
        inline static constexpr std::uint16_t EM_MCHP_PIC = 204;       /* Microchip 8-bit PIC(r) */
        inline static constexpr std::uint16_t EM_INTELGT = 205;        /* Intel Graphics Technology */
        /* reserved 206-209 */
        inline static constexpr std::uint16_t EM_KM32 = 210;           /* KM211 KM32 */
        inline static constexpr std::uint16_t EM_KMX32 = 211;          /* KM211 KMX32 */
        inline static constexpr std::uint16_t EM_EMX16 = 212;          /* KM211 KMX16 */
        inline static constexpr std::uint16_t EM_EMX8 = 213;           /* KM211 KMX8 */
        inline static constexpr std::uint16_t EM_KVARC = 214;          /* KM211 KVARC */
        inline static constexpr std::uint16_t EM_CDP = 215;            /* Paneve CDP */
        inline static constexpr std::uint16_t EM_COGE = 216;           /* Cognitive Smart Memory Processor */
        inline static constexpr std::uint16_t EM_COOL = 217;           /* Bluechip CoolEngine */
        inline static constexpr std::uint16_t EM_NORC = 218;           /* Nanoradio Optimized RISC */
        inline static constexpr std::uint16_t EM_CSR_KALIMBA = 219;    /* CSR Kalimba */
        inline static constexpr std::uint16_t EM_Z80 = 220;            /* Zilog Z80 */
        inline static constexpr std::uint16_t EM_VISIUM = 221;         /* Controls and Data Services VISIUMcore */
        inline static constexpr std::uint16_t EM_FT32 = 222;           /* FTDI Chip FT32 */
        inline static constexpr std::uint16_t EM_MOXIE = 223;          /* Moxie processor */
        inline static constexpr std::uint16_t EM_AMDGPU = 224;         /* AMD GPU */
        /* reserved 225-242 */
        inline static constexpr std::uint16_t EM_RISCV = 243;          /* RISC-V */
        inline static constexpr std::uint16_t EM_BPF = 247;            /* Linux BPF -- in-kernel virtual machine */
        inline static constexpr std::uint16_t EM_CSKY = 252;           /* C-SKY */
        inline static constexpr std::uint16_t EM_LOONGARCH = 258;      /* LoongArch */
        inline static constexpr std::uint16_t EM_NUM = 259;
        inline static constexpr std::uint16_t EM_ARC_A5 = EM_ARC_COMPACT;
        inline static constexpr std::uint16_t EM_ALPHA = 0x9026;
        std::uint16_t e_machine;  /* Required architecture */

        inline static constexpr byte EV_NONE = 0;       /* Invalid version */
        inline static constexpr byte EV_CURRENT = 1;    /* Current version */
        std::uint32_t e_version;  /* Object file version */

        std::uint64_t e_entry;      /* Entry point virtual address */
        std::uint64_t e_phoff;      /* Program header offset */
        std::uint64_t e_shoff;      /* Section header offset */
        std::uint32_t e_flags;      /* Processor-specific flags */
        std::uint16_t e_ehsize;     /* ELF header size */
        std::uint16_t e_phentsize;  /* Program header entry size */
        std::uint16_t e_phnum;      /* Number of program header entries */
        std::uint16_t e_shentsize;  /* Section header entry size */
        std::uint16_t e_shnum;      /* Number of section header entries */
        std::uint16_t e_shstrndx;   /* Section name string table index */
    } elf_header;

    // Same memory layout as Elf64_Shdr for optimization
    typedef struct elf_section_header
    {
        // Special indexes
        inline static constexpr std::uint16_t SHN_UNDEF = 0;           /* Undefined, missing, irrelevant */
        inline static constexpr std::uint16_t SHN_LORESERVE = 0xff00;  /* First of reserved range */
        inline static constexpr std::uint16_t SHN_LOPROC = 0xff00;     /* First processor-specific */
        inline static constexpr std::uint16_t SHN_HIPROC = 0xff1f;     /* Last processor-specific */
        inline static constexpr std::uint16_t SHN_LOOS = 0xff20;       /* First operating system-specific */
        inline static constexpr std::uint16_t SHN_HIOS = 0xff3f;       /* Last operating system-specific */
        inline static constexpr std::uint16_t SHN_ABS = 0xfff1;        /* Absolute values */
        inline static constexpr std::uint16_t SHN_COMMON = 0xfff2;     /* Common data */
        inline static constexpr std::uint16_t SHN_XINDEX = 0xffff;     /* Escape -- index stored elsewhere */
        inline static constexpr std::uint16_t SHN_HIRESERVE = 0xffff;  /* Last of reserved range */

        std::uint32_t sh_name;       /* Section name (string table index) */

        inline static constexpr std::uint32_t SHT_NULL = 0;                     /* Inactive */
        inline static constexpr std::uint32_t SHT_PROGBITS = 1;                 /* Program-defined contents */
        inline static constexpr std::uint32_t SHT_SYMTAB = 2;                   /* Symbol table */
        inline static constexpr std::uint32_t SHT_STRTAB = 3;                   /* String table */
        inline static constexpr std::uint32_t SHT_RELA = 4;                     /* Relocation entries with addends */
        inline static constexpr std::uint32_t SHT_HASH = 5;                     /* Symbol hash table */
        inline static constexpr std::uint32_t SHT_DYNAMIC = 6;                  /* Dynamic linking information */
        inline static constexpr std::uint32_t SHT_NOTE = 7;                     /* Notes */
        inline static constexpr std::uint32_t SHT_NOBITS = 8;                   /* Program space with no data e.g. bss */
        inline static constexpr std::uint32_t SHT_REL = 9;                      /* Relocation entries without addends */
        inline static constexpr std::uint32_t SHT_SHLIB = 10;                   /* Reserved */
        inline static constexpr std::uint32_t SHT_DYNSYM = 11;                  /* Dynmic symbol table */
        inline static constexpr std::uint32_t SHT_INIT_ARRAY = 14;              /* Array of constructors */
        inline static constexpr std::uint32_t SHT_FINI_ARRAY = 15;              /* Array of destructors */
        inline static constexpr std::uint32_t SHT_PREINIT_ARRAY = 16;           /* Array of pre-constructors */
        inline static constexpr std::uint32_t SHT_GROUP = 17;                   /* Section group */
        inline static constexpr std::uint32_t SHT_SYMTAB_SHNDX = 18;            /* Extended section indices */
        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L447-L466
        inline static constexpr std::uint32_t SHT_RELR = 19;                    /* RELR relative relocations */
        inline static constexpr std::uint32_t SHT_NUM = 20;                     /* Number of defined types.  */
        inline static constexpr std::uint32_t SHT_LOOS = 0x60000000;            /* First of OS specific semantics */
        inline static constexpr std::uint32_t SHT_GNU_ATTRIBUTES = 0x6ffffff5;  /* Object attributes.  */
        inline static constexpr std::uint32_t SHT_GNU_HASH = 0x6ffffff6;        /* GNU-style hash table.  */
        inline static constexpr std::uint32_t SHT_GNU_LIBLIST = 0x6ffffff7;     /* Prelink library list */
        inline static constexpr std::uint32_t SHT_CHECKSUM = 0x6ffffff8;        /* Checksum for DSO content.  */
        inline static constexpr std::uint32_t SHT_LOSUNW = 0x6ffffffa;          /* Sun-specific low bound.  */
        inline static constexpr std::uint32_t SHT_SUNW_move = 0x6ffffffa;
        inline static constexpr std::uint32_t SHT_SUNW_COMDAT = 0x6ffffffb;
        inline static constexpr std::uint32_t SHT_SUNW_syminfo = 0x6ffffffc;
        inline static constexpr std::uint32_t SHT_GNU_verdef = 0x6ffffffd;      /* Version definition section.  */
        inline static constexpr std::uint32_t SHT_GNU_verneed = 0x6ffffffe;     /* Version needs section.  */
        inline static constexpr std::uint32_t SHT_GNU_versym = 0x6fffffff;      /* Version symbol table.  */
        inline static constexpr std::uint32_t SHT_HISUNW = 0x6fffffff;          /* Sun-specific high bound.  */
        inline static constexpr std::uint32_t SHT_HIOS = 0x6fffffff;            /* Last of OS specific semantics */
        inline static constexpr std::uint32_t SHT_LOPROC = 0x70000000;          /* First of processor-specific type */
        inline static constexpr std::uint32_t SHT_HIPROC = 0x7fffffff;          /* Last of processor-specific type */
        inline static constexpr std::uint32_t SHT_LOUSER = 0x80000000;          /* First of reserved range */
        inline static constexpr std::uint32_t SHT_HIUSER = 0xffffffff;          /* Last of reserved range */
        std::uint32_t sh_type;       /* Section type */

        inline static constexpr std::uint64_t SHF_WRITE = 0x1;               /* Writable */
        inline static constexpr std::uint64_t SHF_ALLOC = 0x2;               /* Occupies memory during execution */
        inline static constexpr std::uint64_t SHF_EXECINSTR = 0x4;           /* Executable */
        inline static constexpr std::uint64_t SHF_MERGE = 0x10;              /* Might be merged */
        inline static constexpr std::uint64_t SHF_STRINGS = 0x20;            /* Contains nul-terminated strings */
        inline static constexpr std::uint64_t SHF_INFO_LINK = 0x40;          /* `sh_info` contains SHT index */
        inline static constexpr std::uint64_t SHF_LINK_ORDER = 0x80;         /* Preserve order after combining */
        inline static constexpr std::uint64_t SHF_OS_NONCONFORMING = 0x100;  /* Non-standard OS specific handling required */
        inline static constexpr std::uint64_t SHF_GROUP = 0x200;             /* Section is member of a group */
        inline static constexpr std::uint64_t SHF_TLS = 0x400;               /* Section holds thread-local data */
        inline static constexpr std::uint64_t SHF_MASKOS = 0x0ff00000;       /* All bits included are for OS-specific flags */
        inline static constexpr std::uint64_t SHF_MASKPROC = 0xf0000000;     /* All bits included are for processor-specific flags */
        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L484-L488
        inline static constexpr std::uint64_t SHF_GNU_RETAIN = (1 << 21);    /* Not to be GCed by linker.  */
        inline static constexpr std::uint64_t SHF_ORDERED = (1 << 30);       /* Special ordering requirement (Solaris).  */
        inline static constexpr std::uint64_t SHF_EXCLUDE = (1U << 31);      /* Section is excluded unless referenced or allocated (Solaris).*/
        std::uint64_t sh_flags;      /* Section flags */

        std::uint64_t sh_addr;       /* Section virtual address at execution */
        std::uint64_t sh_offset;     /* Section file offset */
        std::uint64_t sh_size;       /* Section size on disk in bytes */
        std::uint32_t sh_link;       /* Link to another section */
        std::uint32_t sh_info;       /* Additional section information */
        std::uint64_t sh_addralign;  /* Address alignment constraint */
        std::uint64_t sh_entsize;    /* Size of entries, if section has table */

        const char *sh_name_str;     /* Resolved section name */
    } elf_section_header;

    inline static constexpr std::uint32_t GRP_COMDAT = 1;             /* Mark group as COMDAT */
    inline static constexpr std::uint32_t GRP_MASKOS = 0x0ff00000;    /* All bits included are for OS-specific flags */
    inline static constexpr std::uint32_t GRP_MASKPROC = 0xf0000000;  /* All bits included are for processor-specific flags */

    // Same memory layout as Elf64_Phdr for optimization
    typedef struct elf_program_header
    {
        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L717-L738
        inline static constexpr std::uint32_t PT_NULL = 0;                   /* Program header table entry unused */
        inline static constexpr std::uint32_t PT_LOAD = 1;                   /* Loadable program segment */
        inline static constexpr std::uint32_t PT_DYNAMIC = 2;                /* Dynamic linking information */
        inline static constexpr std::uint32_t PT_INTERP = 3;                 /* Program interpreter */
        inline static constexpr std::uint32_t PT_NOTE = 4;                   /* Auxiliary information */
        inline static constexpr std::uint32_t PT_SHLIB = 5;                  /* Reserved */
        inline static constexpr std::uint32_t PT_PHDR = 6;                   /* Entry for header table itself */
        inline static constexpr std::uint32_t PT_TLS = 7;                    /* Thread-local storage segment */
        inline static constexpr std::uint32_t PT_NUM = 8;                    /* Number of defined types */
        inline static constexpr std::uint32_t PT_LOOS = 0x60000000;          /* Start of OS-specific */
        inline static constexpr std::uint32_t PT_GNU_EH_FRAME = 0x6474e550;  /* GCC .eh_frame_hdr segment */
        inline static constexpr std::uint32_t PT_GNU_STACK = 0x6474e551;     /* Indicates stack executability */
        inline static constexpr std::uint32_t PT_GNU_RELRO = 0x6474e552;     /* Read-only after relocation */
        inline static constexpr std::uint32_t PT_GNU_PROPERTY = 0x6474e553;  /* GNU property */
        inline static constexpr std::uint32_t PT_GNU_SFRAME = 0x6474e554;    /* SFrame segment.  */
        inline static constexpr std::uint32_t PT_LOSUNW = 0x6ffffffa;
        inline static constexpr std::uint32_t PT_SUNWBSS = 0x6ffffffa;       /* Sun Specific segment */
        inline static constexpr std::uint32_t PT_SUNWSTACK = 0x6ffffffb;     /* Stack segment */
        inline static constexpr std::uint32_t PT_HISUNW = 0x6fffffff;
        inline static constexpr std::uint32_t PT_HIOS = 0x6fffffff;          /* End of OS-specific */
        inline static constexpr std::uint32_t PT_LOPROC = 0x70000000;        /* Start of processor-specific */
        inline static constexpr std::uint32_t PT_HIPROC = 0x7fffffff;        /* End of processor-specific */
        std::uint32_t p_type;    /* Segment type */

        inline static constexpr std::uint32_t PF_X = 0x1;                /* Execute */
        inline static constexpr std::uint32_t PF_W = 0x2;                /* Write */
        inline static constexpr std::uint32_t PF_R = 0x4;                /* Read */
        inline static constexpr std::uint32_t PF_MASKOS = 0x0ff00000;    /* All bits included are for OS-specific flags */
        inline static constexpr std::uint32_t PF_MASKPROC = 0xf0000000;  /* All bits included are for processor-specific flags */
        std::uint32_t p_flags;   /* Segment flags */

        std::uint64_t p_offset;  /* Segment file offset */
        std::uint64_t p_vaddr;   /* Segment virtual address */
        std::uint64_t p_paddr;   /* Segment physical address */
        std::uint64_t p_filesz;  /* Segment size in file */
        std::uint64_t p_memsz;   /* Segment size in memory */
        std::uint64_t p_align;   /* Segment alignment */
    } elf_program_header;

    typedef struct elf_symbol
    {
        std::uint32_t st_name;    /* Symbol name (string table index) */
        std::uint64_t st_value;   /* Symbol value */
        std::uint64_t st_size;    /* Symbol size */
        ::elf::byte st_info;      /* Symbol type and binding */
        ::elf::byte st_other;     /* Symbol visibility */
        std::uint16_t st_shndx;   /* Section index */
        const char *st_name_str;  /* Resolved symbol name */

        inline static constexpr std::uint32_t STN_UNDEF = 0;  /* End of chain identifier */
    } elf_symbol;

    typedef struct elf_dynamic
    {
        inline static constexpr std::int64_t DT_NULL = 0;               /* Marks end of dynamic section */
        inline static constexpr std::int64_t DT_NEEDED = 1;             /* Name of needed library */
        inline static constexpr std::int64_t DT_PLTRELSZ = 2;           /* Size in bytes of all PLT relocations */
        inline static constexpr std::int64_t DT_PLTGOT = 3;             /* Processor defined value relating to PLT/GOT */
        inline static constexpr std::int64_t DT_HASH = 4;               /* Address of the symbol hash table */
        inline static constexpr std::int64_t DT_STRTAB = 5;             /* Address of the dynamic string table */
        inline static constexpr std::int64_t DT_SYMTAB = 6;             /* Address of the dynamic symbol table */
        inline static constexpr std::int64_t DT_RELA = 7;               /* Address of a relocation table with Elf*_Rela entries */
        inline static constexpr std::int64_t DT_RELASZ = 8;             /* Total size in bytes of the DT_RELA relocation table */
        inline static constexpr std::int64_t DT_RELAENT = 9;            /* Size in bytes of each DT_RELA relocation entry */
        inline static constexpr std::int64_t DT_STRSZ = 10;             /* Size in bytes of the string table */
        inline static constexpr std::int64_t DT_SYMENT = 11;            /* Size in bytes of each symbol table entry */
        inline static constexpr std::int64_t DT_INIT = 12;              /* Address of the initialization function */
        inline static constexpr std::int64_t DT_FINI = 13;              /* Address of the termination function */
        inline static constexpr std::int64_t DT_SONAME = 14;            /* Shared object name (string table index) */
        inline static constexpr std::int64_t DT_RPATH = 15;             /* Library search path (string table index) */
        inline static constexpr std::int64_t DT_SYMBOLIC = 16;          /* Indicates "symbolic" linking */
        inline static constexpr std::int64_t DT_REL = 17;               /* Address of a relocation table with Elf*_Rel entries */
        inline static constexpr std::int64_t DT_RELSZ = 18;             /* Total size in bytes of the DT_REL relocation table */
        inline static constexpr std::int64_t DT_RELENT = 19;            /* Size in bytes of each DT_REL relocation entry */
        inline static constexpr std::int64_t DT_PLTREL = 20;            /* Type of relocation used for PLT */
        inline static constexpr std::int64_t DT_DEBUG = 21;             /* Reserved for debugger */
        inline static constexpr std::int64_t DT_TEXTREL = 22;           /* Object contains text relocations (non-writable segment) */
        inline static constexpr std::int64_t DT_JMPREL = 23;            /* Address of the relocations associated with the PLT */
        inline static constexpr std::int64_t DT_BIND_NOW = 24;          /* Process all relocations before execution */
        inline static constexpr std::int64_t DT_INIT_ARRAY = 25;        /* Array of initialization functions */
        inline static constexpr std::int64_t DT_FINI_ARRAY = 26;        /* Array of termination functions */
        inline static constexpr std::int64_t DT_INIT_ARRAYSZ = 27;      /* Size of arrays in DT_INIT_ARRAY */
        inline static constexpr std::int64_t DT_FINI_ARRAYSZ = 28;      /* Size of arrays in DT_FINI_ARRAY */
        inline static constexpr std::int64_t DT_RUNPATH = 29;           /* Library search paths */
        inline static constexpr std::int64_t DT_FLAGS = 30;             /* Flags for the object being loaded */
        inline static constexpr std::int64_t DT_ENCODING = 32;          /* Values from here to DT_LOOS if even use d_ptr or odd uses d_val */
        inline static constexpr std::int64_t DT_PREINIT_ARRAY = 32;     /* Array of pre-initialization functions */
        inline static constexpr std::int64_t DT_PREINIT_ARRAYSZ = 33;   /* Size of array of pre-init functions */

        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L908-L912
        inline static constexpr std::int64_t DT_SYMTAB_SHNDX = 34;      /* Address of SYMTAB_SHNDX section */
        inline static constexpr std::int64_t DT_RELRSZ = 35;            /* Total size of RELR relative relocations */
        inline static constexpr std::int64_t DT_RELR = 36;              /* Address of RELR relative relocations */
        inline static constexpr std::int64_t DT_RELRENT = 37;           /* Size of one RELR relative relocaction */
        inline static constexpr std::int64_t DT_NUM = 38;               /* Number used */

        inline static constexpr std::int64_t DT_LOOS = 0x6000000D;      /* Start of OS-specific */
        inline static constexpr std::int64_t DT_HIOS = 0x6ffff000;      /* End of OS-specific */
        inline static constexpr std::int64_t DT_LOPROC = 0x70000000;    /* Start of processor-specific */
        inline static constexpr std::int64_t DT_HIPROC = 0x7fffffff;    /* End of processor-specific */

        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L919-L983
        /* DT_* entries which fall between DT_VALRNGHI & DT_VALRNGLO use the
         Dyn.d_un.d_val field of the Elf*_Dyn structure.  This follows Sun's
         approach.  */
        inline static constexpr std::int64_t DT_VALRNGLO = 0x6ffffd00;
        inline static constexpr std::int64_t DT_GNU_PRELINKED = 0x6ffffdf5;   /* Prelinking timestamp */
        inline static constexpr std::int64_t DT_GNU_CONFLICTSZ = 0x6ffffdf6;  /* Size of conflict section */
        inline static constexpr std::int64_t DT_GNU_LIBLISTSZ = 0x6ffffdf7;   /* Size of library list */
        inline static constexpr std::int64_t DT_CHECKSUM = 0x6ffffdf8;
        inline static constexpr std::int64_t DT_PLTPADSZ = 0x6ffffdf9;
        inline static constexpr std::int64_t DT_MOVEENT = 0x6ffffdfa;
        inline static constexpr std::int64_t DT_MOVESZ = 0x6ffffdfb;
        inline static constexpr std::int64_t DT_FEATURE_1 = 0x6ffffdfc;       /* Feature selection (DTF_*).  */
        inline static constexpr std::int64_t DT_POSFLAG_1 = 0x6ffffdfd;       /* Flags for DT_* entries, effecting the following DT_* entry.  */
        inline static constexpr std::int64_t DT_SYMINSZ = 0x6ffffdfe;         /* Size of syminfo table (in bytes) */
        inline static constexpr std::int64_t DT_SYMINENT = 0x6ffffdff;        /* Entry size of syminfo */
        inline static constexpr std::int64_t DT_VALRNGHI = 0x6ffffdff;
        inline static constexpr std::int64_t DT_VALNUM = 12;

        /* DT_* entries which fall between DT_ADDRRNGHI & DT_ADDRRNGLO use the
           Dyn.d_un.d_ptr field of the Elf*_Dyn structure.

         If any adjustment is made to the ELF object after it has been
         built these entries will need to be adjusted.  */
        inline static constexpr std::int64_t DT_ADDRRNGLO = 0x6ffffe00;
        inline static constexpr std::int64_t DT_GNU_HASH = 0x6ffffef5;      /* GNU-style hash table.  */
        inline static constexpr std::int64_t DT_TLSDESC_PLT = 0x6ffffef6;
        inline static constexpr std::int64_t DT_TLSDESC_GOT = 0x6ffffef7;
        inline static constexpr std::int64_t DT_GNU_CONFLICT = 0x6ffffef8;  /* Start of conflict section */
        inline static constexpr std::int64_t DT_GNU_LIBLIST = 0x6ffffef9;   /* Library list */
        inline static constexpr std::int64_t DT_CONFIG = 0x6ffffefa;        /* Configuration information.  */
        inline static constexpr std::int64_t DT_DEPAUDIT = 0x6ffffefb;      /* Dependency auditing.  */
        inline static constexpr std::int64_t DT_AUDIT = 0x6ffffefc;         /* Object auditing.  */
        inline static constexpr std::int64_t DT_PLTPAD = 0x6ffffefd;        /* PLT padding.  */
        inline static constexpr std::int64_t DT_MOVETAB = 0x6ffffefe;       /* Move table.  */
        inline static constexpr std::int64_t DT_SYMINFO = 0x6ffffeff;       /* Syminfo table.  */
        inline static constexpr std::int64_t DT_ADDRRNGHI = 0x6ffffeff;
        inline static constexpr std::int64_t DT_ADDRNUM = 11;

        /* The versioning entry types.  The next are defined as part of the
           GNU extension.  */
        inline static constexpr std::int64_t DT_VERSYM = 0x6ffffff0;
        inline static constexpr std::int64_t DT_RELACOUNT = 0x6ffffff9;
        inline static constexpr std::int64_t DT_RELCOUNT = 0x6ffffffa;

        /* These were chosen by Sun.  */
        inline static constexpr std::int64_t DT_FLAGS_1 = 0x6ffffffb;     /* State flags, see DF_1_* below.  */
        inline static constexpr std::int64_t DT_VERDEF = 0x6ffffffc;      /* Address of version definition table */
        inline static constexpr std::int64_t DT_VERDEFNUM = 0x6ffffffd;   /* Number of version definitions */
        inline static constexpr std::int64_t DT_VERNEED = 0x6ffffffe;     /* Address of table with needed versions */
        inline static constexpr std::int64_t DT_VERNEEDNUM = 0x6fffffff;  /* Number of needed versions */
        inline static constexpr std::int64_t DT_VERSIONTAGNUM = 16;

        /* Sun added these machine-independent extensions in the "processor-specific"
           range.  Be compatible.  */
        inline static constexpr std::int64_t DT_AUXILIARY = 0x7ffffffd;  /* Shared object to load before self */
        inline static constexpr std::int64_t DT_FILTER = 0x7fffffff;     /* Shared object to get values from */
        inline static constexpr std::int64_t DT_EXTRANUM = 3;

        std::int64_t d_tag;       /* Entry type */
        union
        {
            std::uint64_t d_val;  /* Integer value */
            std::uint64_t d_ptr;  /* Address value */
        } d_un;

        // DT_FLAGS values
        inline static constexpr std::uint64_t DF_ORIGIN = 0x1;       /* Object may use $ORIGIN */
        inline static constexpr std::uint64_t DF_SYMBOLIC = 0x2;     /* Symbol resolutions starts from this object */
        inline static constexpr std::uint64_t DF_TEXTREL = 0x4;      /* Object contains text relocations (non-writable segment) */
        inline static constexpr std::uint64_t DF_BIND_NOW = 0x8;     /* No lazy binding for this object */
        inline static constexpr std::uint64_t DF_STATIC_TLS = 0x10;  /* Module uses the static TLS model */

        // DT_FLAGS_1 values - https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L992-L1033
        inline static constexpr std::uint64_t DF_1_NOW = 0x00000001;         /* Set RTLD_NOW for this object.  */
        inline static constexpr std::uint64_t DF_1_GLOBAL = 0x00000002;      /* Set RTLD_GLOBAL for this object.  */
        inline static constexpr std::uint64_t DF_1_GROUP = 0x00000004;       /* Set RTLD_GROUP for this object.  */
        inline static constexpr std::uint64_t DF_1_NODELETE = 0x00000008;    /* Set RTLD_NODELETE for this object.*/
        inline static constexpr std::uint64_t DF_1_LOADFLTR = 0x00000010;    /* Trigger filtee loading at runtime.*/
        inline static constexpr std::uint64_t DF_1_INITFIRST = 0x00000020;   /* Set RTLD_INITFIRST for this object*/
        inline static constexpr std::uint64_t DF_1_NOOPEN = 0x00000040;      /* Set RTLD_NOOPEN for this object.  */
        inline static constexpr std::uint64_t DF_1_ORIGIN = 0x00000080;      /* $ORIGIN must be handled.  */
        inline static constexpr std::uint64_t DF_1_DIRECT = 0x00000100;      /* Direct binding enabled.  */
        inline static constexpr std::uint64_t DF_1_TRANS = 0x00000200;
        inline static constexpr std::uint64_t DF_1_INTERPOSE = 0x00000400;   /* Object is used to interpose.  */
        inline static constexpr std::uint64_t DF_1_NODEFLIB = 0x00000800;    /* Ignore default lib search path.  */
        inline static constexpr std::uint64_t DF_1_NODUMP = 0x00001000;      /* Object can't be dldump'ed.  */
        inline static constexpr std::uint64_t DF_1_CONFALT = 0x00002000;     /* Configuration alternative created.*/
        inline static constexpr std::uint64_t DF_1_ENDFILTEE = 0x00004000;   /* Filtee terminates filters search. */
        inline static constexpr std::uint64_t DF_1_DISPRELDNE = 0x00008000;  /* Disp reloc applied at build time. */
        inline static constexpr std::uint64_t DF_1_DISPRELPND = 0x00010000;  /* Disp reloc applied at run-time.  */
        inline static constexpr std::uint64_t DF_1_NODIRECT = 0x00020000;    /* Object has no-direct binding. */
        inline static constexpr std::uint64_t DF_1_IGNMULDEF = 0x00040000;
        inline static constexpr std::uint64_t DF_1_NOKSYMS = 0x00080000;
        inline static constexpr std::uint64_t DF_1_NOHDR = 0x00100000;
        inline static constexpr std::uint64_t DF_1_EDITED = 0x00200000;      /* Object is modified after built.  */
        inline static constexpr std::uint64_t DF_1_NORELOC = 0x00400000;
        inline static constexpr std::uint64_t DF_1_SYMINTPOSE = 0x00800000;  /* Object has individual interposers.  */
        inline static constexpr std::uint64_t DF_1_GLOBAUDIT = 0x01000000;   /* Global auditing required.  */
        inline static constexpr std::uint64_t DF_1_SINGLETON = 0x02000000;   /* Singleton symbols are used.  */
        inline static constexpr std::uint64_t DF_1_STUB = 0x04000000;
        inline static constexpr std::uint64_t DF_1_PIE = 0x08000000;
        inline static constexpr std::uint64_t DF_1_KMOD = 0x10000000;
        inline static constexpr std::uint64_t DF_1_WEAKFILTER = 0x20000000;
        inline static constexpr std::uint64_t DF_1_NOCOMMON = 0x40000000;

        /* Flags for the feature selection in DT_FEATURE_1.  */
        inline static constexpr std::uint64_t DTF_1_PARINIT = 0x00000001;
        inline static constexpr std::uint64_t DTF_1_CONFEXP = 0x00000002;

        /* Flags in the DT_POSFLAG_1 entry effecting only the next DT_* entry.  */
        inline static constexpr std::uint64_t DF_P1_LAZYLOAD = 0x00000001;   /* Lazyload following object.  */
        inline static constexpr std::uint64_t DF_P1_GROUPPERM = 0x00000002;  /* Symbols from next object are not generally available.  */
    } elf_dynamic;

    typedef struct elf_rel
    {
        std::uint64_t r_offset;  /* Address */
        /*
         * There is a huge amount of relocation types spanning architectures,
         * these are the values for i386 and x86_64.
         */
        // https://raw.githubusercontent.com/wiki/hjl-tools/x86-psABI/intel386-psABI-1.1.pdf
        inline static constexpr std::uint32_t R_386_NONE = 0;
        inline static constexpr std::uint32_t R_386_32 = 1;
        inline static constexpr std::uint32_t R_386_PC32 = 2;
        inline static constexpr std::uint32_t R_386_GOT32 = 3;
        inline static constexpr std::uint32_t R_386_PLT32 = 4;
        inline static constexpr std::uint32_t R_386_COPY = 5;
        inline static constexpr std::uint32_t R_386_GLOB_DAT = 6;
        inline static constexpr std::uint32_t R_386_JUMP_SLOT = 7;
        inline static constexpr std::uint32_t R_386_RELATIVE = 8;
        inline static constexpr std::uint32_t R_386_GOTOFF = 9;
        inline static constexpr std::uint32_t R_386_GOTPC = 10;
        inline static constexpr std::uint32_t R_386_TLS_TPOFF = 14;
        inline static constexpr std::uint32_t R_386_TLS_IE = 15;
        inline static constexpr std::uint32_t R_386_TLS_GOTIE = 16;
        inline static constexpr std::uint32_t R_386_TLS_LE = 17;
        inline static constexpr std::uint32_t R_386_TLS_GD = 18;
        inline static constexpr std::uint32_t R_386_TLS_LDM = 19;
        inline static constexpr std::uint32_t R_386_16 = 20;
        inline static constexpr std::uint32_t R_386_PC16 = 21;
        inline static constexpr std::uint32_t R_386_8 = 22;
        inline static constexpr std::uint32_t R_386_PC8 = 23;
        inline static constexpr std::uint32_t R_386_TLS_GD_32 = 24;
        inline static constexpr std::uint32_t R_386_TLS_GD_PUSH = 25;
        inline static constexpr std::uint32_t R_386_TLS_GD_CALL = 26;
        inline static constexpr std::uint32_t R_386_TLS_GD_POP = 27;
        inline static constexpr std::uint32_t R_386_TLS_LDM_32 = 28;
        inline static constexpr std::uint32_t R_386_TLS_LDM_PUSH = 29;
        inline static constexpr std::uint32_t R_386_TLS_LDM_CALL = 30;
        inline static constexpr std::uint32_t R_386_TLS_LDM_POP = 31;
        inline static constexpr std::uint32_t R_386_TLS_LDO_32 = 32;
        inline static constexpr std::uint32_t R_386_TLS_IE_32 = 33;
        inline static constexpr std::uint32_t R_386_TLS_LE_32 = 34;
        inline static constexpr std::uint32_t R_386_TLS_DTPMOD32 = 35;
        inline static constexpr std::uint32_t R_386_TLS_DTPOFF32 = 36;
        inline static constexpr std::uint32_t R_386_TLS_TPOFF32 = 37;
        inline static constexpr std::uint32_t R_386_SIZE32 = 38;
        inline static constexpr std::uint32_t R_386_TLS_GOTDESC = 39;
        inline static constexpr std::uint32_t R_386_TLS_DESC_CALL = 40;
        inline static constexpr std::uint32_t R_386_TLS_DESC = 41;
        inline static constexpr std::uint32_t R_386_IRELATIVE = 42;
        inline static constexpr std::uint32_t R_386_GOT32X = 43;
        // https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf
        inline static constexpr std::uint32_t R_X86_64_NONE = 0;
        inline static constexpr std::uint32_t R_X86_64_64 = 1;
        inline static constexpr std::uint32_t R_X86_64_PC32 = 2;
        inline static constexpr std::uint32_t R_X86_64_GOT32 = 3;
        inline static constexpr std::uint32_t R_X86_64_PLT32 = 4;
        inline static constexpr std::uint32_t R_X86_64_COPY = 5;
        inline static constexpr std::uint32_t R_X86_64_GLOB_DAT = 6;
        inline static constexpr std::uint32_t R_X86_64_JUMP_SLOT = 7;
        inline static constexpr std::uint32_t R_X86_64_RELATIVE = 8;
        inline static constexpr std::uint32_t R_X86_64_GOTPCREL = 9;
        inline static constexpr std::uint32_t R_X86_64_32 = 10;
        inline static constexpr std::uint32_t R_X86_64_32S = 11;
        inline static constexpr std::uint32_t R_X86_64_16 = 12;
        inline static constexpr std::uint32_t R_X86_64_PC16 = 13;
        inline static constexpr std::uint32_t R_X86_64_8 = 14;
        inline static constexpr std::uint32_t R_X86_64_PC8 = 15;
        inline static constexpr std::uint32_t R_X86_64_DTPMOD64 = 16;
        inline static constexpr std::uint32_t R_X86_64_DTPOFF64 = 17;
        inline static constexpr std::uint32_t R_X86_64_TPOFF64 = 18;
        inline static constexpr std::uint32_t R_X86_64_TLSGD = 19;
        inline static constexpr std::uint32_t R_X86_64_TLSLD = 20;
        inline static constexpr std::uint32_t R_X86_64_DTPOFF32 = 21;
        inline static constexpr std::uint32_t R_X86_64_GOTTPOFF = 22;
        inline static constexpr std::uint32_t R_X86_64_TPOFF32 = 23;
        inline static constexpr std::uint32_t R_X86_64_PC64 = 24;
        inline static constexpr std::uint32_t R_X86_64_GOTOFF64 = 25;
        inline static constexpr std::uint32_t R_X86_64_GOTPC32 = 26;
        inline static constexpr std::uint32_t R_X86_64_GOT64 = 27;
        inline static constexpr std::uint32_t R_X86_64_GOTPCREL64 = 28;
        inline static constexpr std::uint32_t R_X86_64_GOTPC64 = 29;
        inline static constexpr std::uint32_t R_X86_64_GOTPLT64 = 30;
        inline static constexpr std::uint32_t R_X86_64_PLTOFF64 = 31;
        inline static constexpr std::uint32_t R_X86_64_SIZE32 = 32;
        inline static constexpr std::uint32_t R_X86_64_SIZE64 = 33;
        inline static constexpr std::uint32_t R_X86_64_GOTPC32_TLSDESC = 34;
        inline static constexpr std::uint32_t R_X86_64_TLSDESC_CALL = 35;
        inline static constexpr std::uint32_t R_X86_64_TLSDESC = 36;
        inline static constexpr std::uint32_t R_X86_64_IRELATIVE = 37;

        std::uint32_t r_type;    /* Relocation type */
        std::uint32_t r_sym;     /* Symbol index */
    } elf_rel;

    typedef struct elf_rela
    {
        std::uint64_t r_offset;  /* Address */
        // elf_rel contains all the types
        std::uint32_t r_type;    /* Relocation type */
        std::uint32_t r_sym;     /* Symbol index */
        std::int64_t r_addend;   /* Addend */
    } elf_rela;

    /*
     * Standard ELF structures
     */
    namespace types
    {
        typedef std::uint32_t Elf32_Addr;
        typedef std::uint32_t Elf32_Off;
        typedef std::uint16_t Elf32_Half;
        typedef std::uint32_t Elf32_Word;
        typedef std::int32_t Elf32_Sword;

        typedef std::uint64_t Elf64_Addr;
        typedef std::uint64_t Elf64_Off;
        typedef std::uint16_t Elf64_Half;
        typedef std::uint32_t Elf64_Word;
        typedef std::int32_t Elf64_Sword;
        typedef std::uint64_t Elf64_Xword;
        typedef std::int64_t Elf64_Sxword;

        typedef struct Elf32_Ehdr
        {
            elf_ident e_ident;
            Elf32_Half e_type;
            Elf32_Half e_machine;
            Elf32_Word e_version;
            Elf32_Addr e_entry;
            Elf32_Off e_phoff;
            Elf32_Off e_shoff;
            Elf32_Word e_flags;
            Elf32_Half e_ehsize;
            Elf32_Half e_phentsize;
            Elf32_Half e_phnum;
            Elf32_Half e_shentsize;
            Elf32_Half e_shnum;
            Elf32_Half e_shstrndx;
        } Elf32_Ehdr;

        typedef struct Elf64_Ehdr
        {
            elf_ident e_ident;
            Elf64_Half e_type;
            Elf64_Half e_machine;
            Elf64_Word e_version;
            Elf64_Addr e_entry;
            Elf64_Off e_phoff;
            Elf64_Off e_shoff;
            Elf64_Word e_flags;
            Elf64_Half e_ehsize;
            Elf64_Half e_phentsize;
            Elf64_Half e_phnum;
            Elf64_Half e_shentsize;
            Elf64_Half e_shnum;
            Elf64_Half e_shstrndx;
        } Elf64_Ehdr;

        typedef struct Elf32_Phdr
        {
            Elf32_Word p_type;
            Elf32_Off p_offset;
            Elf32_Addr p_vaddr;
            Elf32_Addr p_paddr;
            Elf32_Word p_filesz;
            Elf32_Word p_memsz;
            Elf32_Word p_flags;
            Elf32_Word p_align;
        } Elf32_Phdr;

        typedef struct Elf64_Phdr
        {
            Elf64_Word p_type;
            Elf64_Word p_flags;
            Elf64_Off p_offset;
            Elf64_Addr p_vaddr;
            Elf64_Addr p_paddr;
            Elf64_Xword p_filesz;
            Elf64_Xword p_memsz;
            Elf64_Xword p_align;
        } Elf64_Phdr;

        typedef struct Elf32_Shdr
        {
            Elf32_Word sh_name;
            Elf32_Word sh_type;
            Elf32_Word sh_flags;
            Elf32_Addr sh_addr;
            Elf32_Off sh_offset;
            Elf32_Word sh_size;
            Elf32_Word sh_link;
            Elf32_Word sh_info;
            Elf32_Word sh_addralign;
            Elf32_Word sh_entsize;
        } Elf32_Shdr;

        typedef struct Elf64_Shdr
        {
            Elf64_Word sh_name;
            Elf64_Word sh_type;
            Elf64_Xword sh_flags;
            Elf64_Addr sh_addr;
            Elf64_Off sh_offset;
            Elf64_Xword sh_size;
            Elf64_Word sh_link;
            Elf64_Word sh_info;
            Elf64_Xword sh_addralign;
            Elf64_Xword sh_entsize;
        } Elf64_Shdr;

        typedef struct Elf32_Sym
        {
            Elf32_Word st_name;
            Elf32_Addr st_value;
            Elf32_Word st_size;
            unsigned char st_info;
            unsigned char st_other;
            Elf32_Half st_shndx;
        } Elf32_Sym;

        typedef struct Elf64_Sym
        {
            Elf64_Word st_name;
            unsigned char st_info;
            unsigned char st_other;
            Elf64_Half st_shndx;
            Elf64_Addr st_value;
            Elf64_Xword st_size;
        } Elf64_Sym;

        typedef struct Elf32_Dyn
        {
            Elf32_Sword d_tag;
            union
            {
                Elf32_Word d_val;
                Elf32_Addr d_ptr;
            } d_un;
        } Elf32_Dyn;

        typedef struct Elf64_Dyn
        {
            Elf64_Sxword d_tag;
            union
            {
                Elf64_Xword d_val;
                Elf64_Addr d_ptr;
            } d_un;
        } Elf64_Dyn;

        typedef struct Elf32_Rel
        {
            Elf32_Addr r_offset;
            Elf32_Word r_info;
        } Elf32_Rel;

        typedef struct Elf64_Rel
        {
            Elf64_Addr r_offset;
            Elf64_Xword r_info;
        } Elf64_Rel;

        typedef struct Elf32_Rela
        {
            Elf32_Addr r_offset;
            Elf32_Word r_info;
            Elf32_Sword r_addend;
        } Elf32_Rela;

        typedef struct Elf64_Rela
        {
            Elf64_Addr r_offset;
            Elf64_Xword r_info;
            Elf64_Sxword r_addend;
        } Elf64_Rela;
    }

    /*
     * Compute the ELF hash value for a symbol name
     */
    inline std::uint_fast32_t elf_hash(const char *name)
    {
      std::uint_fast32_t h = 0, g;
      auto *uname = reinterpret_cast<const unsigned char *>(name);
      while (*uname)
      {
        h = (h << 4) + *uname++;
        if (g = h & 0xf0000000; g)
          h ^= g >> 24;
        h &= ~g;
      }
      return h;
    }

    /*
     * Compute the GNU hash value for a symbol name
     *
     * References:
     *  https://blogs.oracle.com/solaris/post/gnu-hash-elf-sections
     *  https://sourceware.org/legacy-ml/binutils/2006-10/msg00377.html
     */
    inline uint_fast32_t gnu_hash(const char *s)
    {
      uint_fast32_t h = 5381;

      for (unsigned char c = *s; c != '\0'; c = *++s)
        h = h * 33 + c;

      return h & 0xffffffff;
    }

    /*
     * Read and parse an ELF file into a set of structures
     */
    class elf_file
    {
    public:
        explicit elf_file(std::filesystem::path path) : path(std::move(path))
        {
          if (!this->open_file())
          {
            return;
          }
          if (
                  !this->read_elf_header() ||
                  !this->read_program_headers() ||
                  !this->read_section_headers() ||
                  !this->read_init_functions() ||
                  !this->read_term_functions()
                  )
          {
            return;
          }
        };

        /*
         * Check if an error occurred
         */
        bool error() const
        {
          return !this->last_error.empty();
        }

        /*
         * Get a message for the last error that occurred
         */
        const std::string &error_message() const
        {
          return this->last_error;
        }

        /*
         * Clear the current error
         */
        void clear_error()
        {
          this->last_error = "";
        }

        /*
         * Get a bitness-agnostic Elf*_Ehdr structure
         */
        const ::elf::elf_header &get_header() const
        {
          return this->header;
        }

        /*
         * Get a vector of bitness-agnostic Elf*_Phdr structures
         */
        const std::vector<::elf::elf_program_header> &get_program_headers() const
        {
          return this->program_headers;
        }

        /*
         * Get a vector of bitness-agnostic Elf*_Shdr structures
         */
        const std::vector<::elf::elf_section_header> &get_section_headers() const
        {
          return this->section_headers;
        }

        /*
         * Get a vector of addresses of initialization functions. Addresses
         * assume the file is loaded at its base address. They are in order
         * of .preinit_array, .init, .init_array
         */
        const std::vector<std::uint64_t> &get_init_functions() const
        {
          return this->init_functions;
        }

        /*
         * Get a vector of addresses of termination functions. Addresses assume
         * the file is loaded at its base address. They are in order of
         * .fini_array (reversed), .fini
         */
        const std::vector<std::uint64_t> &get_fini_functions() const
        {
          return this->fini_functions;
        }

        /*
         * Get the expected base address. This is the virtual address of the
         * lowest PT_LOAD segment. An executable should be loaded at this
         * address. Shared libraries can be loaded anywhere as long as the
         * internal layout and spacing remains the same.
         */
        std::uint64_t get_base_address() const
        {
          return this->base_address;
        }

        /*
         * Check if the ELF file is little endian. Shorthand for
         * get_header().e_ident.ei_data == ::elf::elf_ident::ELFDATA2LSB
         */
        bool is_little_endian() const
        {
          return this->header.e_ident.ei_data == ::elf::elf_ident::ELFDATA2LSB;
        }

        /*
         * Check if the ELF file is big endian. Shorthand for
         * get_header().e_ident.ei_data == ::elf::elf_ident::ELFDATA2MSB
         */
        bool is_big_endian() const
        {
          return this->header.e_ident.ei_data == ::elf::elf_ident::ELFDATA2MSB;
        }

        /*
         * Check if the ELF file is 32-bit. Shorthand for
         * get_header().e_ident.ei_class == ::elf::elf_ident::ELFCLASS32
         */
        bool is_32_bit() const
        {
          return this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS32;
        }

        /*
         * Check if the ELF file is 64-bit. Shorthand for
         * get_header().e_ident.ei_class == ::elf::elf_ident::ELFCLASS64
         */
        bool is_64_bit() const
        {
          return this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS64;
        }

        /*
         * Get the underlying std::ifstream to read the binary file
         */
        std::ifstream &get_binary_file()
        {
          return this->binary_file;
        }

        /*
         * Parse the dynamic segment of the ELF file including symbols
         */
        bool parse_dynamic_segment()
        {
          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
            return false;
          }

          /*
           * Read the dynamic segment entries
           */
          const auto dynamic_header = std::find_if(this->program_headers.begin(), this->program_headers.end(),
                                                   [](const elf::elf_program_header &program_header) {
                                                       return program_header.p_type == ::elf::elf_program_header::PT_DYNAMIC;
                                                   });
          if (dynamic_header == this->program_headers.end())
          {
            return false;
          }
          this->binary_file.clear();
          this->binary_file.seekg(static_cast<std::streamoff>(dynamic_header->p_offset));
          if (this->is_64_bit())
          {
            std::size_t dynamic_entry_size = sizeof(::elf::types::Elf64_Dyn);
            std::size_t dynamic_entry_count = dynamic_header->p_filesz / dynamic_entry_size;
            if (dynamic_header->p_filesz % dynamic_entry_size != 0)
            {
              this->last_error = "Invalid dynamic segment size";
              return false;
            }
            this->dynamic_entries.resize(dynamic_entry_count);
            this->binary_file.read(reinterpret_cast<char *>(this->dynamic_entries.data()), static_cast<std::streamsize>(dynamic_header->p_filesz));
            if (this->binary_file.gcount() != dynamic_header->p_filesz)
            {
              this->last_error = "Failed to read dynamic segment";
              return false;
            }
          } else if (this->is_32_bit())
          {
            std::size_t dynamic_entry_size = sizeof(::elf::types::Elf32_Dyn);
            std::size_t dynamic_entry_count = dynamic_header->p_filesz / dynamic_entry_size;
            if (dynamic_header->p_filesz % dynamic_entry_size != 0)
            {
              this->last_error = "Invalid dynamic segment size";
              return false;
            }
            this->dynamic_entries.resize(dynamic_entry_count);
            std::vector<::elf::types::Elf32_Dyn> real_dynamic_segment(dynamic_entry_count);
            this->binary_file.read(reinterpret_cast<char *>(real_dynamic_segment.data()), static_cast<std::streamsize>(dynamic_header->p_filesz));
            if (this->binary_file.gcount() != dynamic_header->p_filesz)
            {
              this->last_error = "Failed to read dynamic segment";
              return false;
            }
            for (std::size_t i = 0; i < dynamic_entry_count; i++)
            {
              this->dynamic_entries[i].d_tag = real_dynamic_segment[i].d_tag;
              this->dynamic_entries[i].d_un.d_val = real_dynamic_segment[i].d_un.d_val;
            }
          } else
          {
            return false;
          }

          /*
           * Extract the info we need. Apply the dynamic string table offset later
           */
          std::uintptr_t dynamic_string_table_offset = 0;
          std::uint64_t dynamic_string_table_length = 0;
          std::uintptr_t symbol_table_offset = 0;
          std::uint64_t symbol_table_entry_size = 0;
          for (const auto &dynamic_entry: this->dynamic_entries)
          {
            switch (dynamic_entry.d_tag)
            {
              case ::elf::elf_dynamic::DT_STRTAB:
              {
                dynamic_string_table_offset = dynamic_entry.d_un.d_ptr - this->base_address;
                break;
              }
              case ::elf::elf_dynamic::DT_STRSZ:
              {
                dynamic_string_table_length = dynamic_entry.d_un.d_val;
                break;
              }
              case ::elf::elf_dynamic::DT_SYMTAB:
              {
                symbol_table_offset = dynamic_entry.d_un.d_ptr - this->base_address;
                break;
              }
              case ::elf::elf_dynamic::DT_SYMENT:
              {
                symbol_table_entry_size = dynamic_entry.d_un.d_val;
                break;
              }
              case ::elf::elf_dynamic::DT_SONAME:
              {
                this->so_name = reinterpret_cast<const char *>(static_cast<std::uintptr_t>(dynamic_entry.d_un.d_val));
                break;
              }
              case ::elf::elf_dynamic::DT_NEEDED:
              {
                this->needed_libraries.emplace_back(reinterpret_cast<const char *>(static_cast<std::uintptr_t>(dynamic_entry.d_un.d_val)));
                break;
              }
            }
          }
          if (dynamic_string_table_offset == 0 || dynamic_string_table_length == 0)
          {
            this->last_error = "Failed to find dynamic string table";
            return false;
          }
          if (symbol_table_offset == 0 || symbol_table_entry_size == 0)
          {
            this->last_error = "Failed to find symbol table";
            return false;
          }

          /*
           * Read the dynamic string table
           */
          this->dynamic_segment_string_table.resize(dynamic_string_table_length);
          this->binary_file.seekg(static_cast<std::streamoff>(dynamic_string_table_offset));
          this->binary_file.read(this->dynamic_segment_string_table.data(), static_cast<std::streamsize>(dynamic_string_table_length));
          if (this->binary_file.gcount() != dynamic_string_table_length)
          {
            this->last_error = "Failed to read dynamic string table";
            return false;
          }
          const auto dynamic_string_table_base = reinterpret_cast<std::uintptr_t>(this->dynamic_segment_string_table.data());

          /*
           * Apply the dynamic string table offset to the strings we need
           */
          this->so_name = dynamic_string_table_base + this->so_name;
          for (auto &needed: this->needed_libraries)
          {
            needed = dynamic_string_table_base + needed;
          }

          /*
           * Validate the symbol table header
           */
          const auto symbol_table_header = std::find_if(this->section_headers.begin(), this->section_headers.end(),
                                                        [](const elf::elf_section_header &section_header) {
                                                            return section_header.sh_type == ::elf::elf_section_header::SHT_DYNSYM;
                                                        });
          if (symbol_table_header == this->section_headers.end())
          {
            this->last_error = "Failed to find dynamic symbol table";
            return false;
          }
          if (symbol_table_offset != symbol_table_header->sh_offset)
          {
            this->last_error = "Symbol table offsets don't match";
            return false;
          }

          /*
           * Read the symbol table
           */
          std::uint64_t symbol_table_entry_count = symbol_table_header->sh_size / symbol_table_entry_size;
          if (this->is_64_bit())
          {
            if (symbol_table_entry_size != sizeof(::elf::types::Elf64_Sym))
            {
              this->last_error = "Invalid symbol table entry size";
              return false;
            }
            this->dynamic_symbols.resize(symbol_table_entry_count);
            this->binary_file.seekg(static_cast<std::streamoff>(symbol_table_offset));
            for (std::size_t i = 0; i < symbol_table_entry_count; i++)
            {
              this->binary_file.read(reinterpret_cast<char *>(&this->dynamic_symbols[i]), static_cast<std::streamsize>(symbol_table_entry_size));
              if (this->binary_file.gcount() != symbol_table_entry_size)
              {
                this->last_error = "Failed to read dynamic symbol";
                return false;
              }
              this->dynamic_symbols[i].st_name_str = this->dynamic_segment_string_table.data() + this->dynamic_symbols[i].st_name;
            }
          } else
          {
            if (symbol_table_entry_size != sizeof(::elf::types::Elf32_Sym))
            {
              this->last_error = "Invalid symbol table entry size";
              return false;
            }
            this->dynamic_symbols.resize(symbol_table_entry_count);
            std::vector<::elf::types::Elf32_Sym> real_dynamic_symbols(symbol_table_entry_count);
            const auto real_symbol_table_size = static_cast<std::streamsize>(symbol_table_entry_size * symbol_table_entry_count);
            this->binary_file.seekg(static_cast<std::streamoff>(symbol_table_offset));
            this->binary_file.read(reinterpret_cast<char *>(real_dynamic_symbols.data()), real_symbol_table_size);
            if (this->binary_file.gcount() != real_symbol_table_size)
            {
              this->last_error = "Failed to read dynamic symbols";
              return false;
            }
            for (std::size_t i = 0; i < symbol_table_entry_count; i++)
            {
              this->dynamic_symbols[i].st_name = real_dynamic_symbols[i].st_name;
              this->dynamic_symbols[i].st_info = real_dynamic_symbols[i].st_info;
              this->dynamic_symbols[i].st_other = real_dynamic_symbols[i].st_other;
              this->dynamic_symbols[i].st_shndx = real_dynamic_symbols[i].st_shndx;
              this->dynamic_symbols[i].st_value = real_dynamic_symbols[i].st_value;
              this->dynamic_symbols[i].st_size = real_dynamic_symbols[i].st_size;
              this->dynamic_symbols[i].st_name_str = this->dynamic_segment_string_table.data() + this->dynamic_symbols[i].st_name;
            }
          }

          /*
           * Parse whichever hash tables are present
           */
          if (!this->parse_hash_tables())
          {
            return false;
          }

          /*
           * Parse rel and rela sections, whichever are present
           */
          if (!this->parse_relocations())
          {
            return false;
          }

          return true;
        }

        /*
         * Get a vector of bitness-agnostic Elf*_Dyn structures
         */
        const std::vector<::elf::elf_dynamic> &get_dynamic_entries() const
        {
          return this->dynamic_entries;
        }

        /*
         * Get the dynamic string table which contains symbol names amongst other things
         */
        const std::vector<char> &get_dynamic_string_table() const
        {
          return this->dynamic_segment_string_table;
        }

        /*
         * Get the value of DT_SONAME
         */
        const char *get_so_name() const
        {
          return this->so_name;
        }

        /*
         * Get a vector of DT_NEEDED values
         */
        const std::vector<const char *> &get_needed_libraries() const
        {
          return this->needed_libraries;
        }

        /*
         * Get a vector of bitness-agnostic Elf*_Sym structures for all dynamic symbols
         */
        const std::vector<elf::elf_symbol> &get_dynamic_symbols() const
        {
          return this->dynamic_symbols;
        }

        /*
         * Get a symbol by its name using the GNU hash table and/or the ELF hash table
         */
        std::vector<::elf::elf_symbol>::const_iterator get_symbol(const char *name) const
        {
          auto symbol = this->lookup_gnu_symbol(name);
          if (symbol == this->dynamic_symbols.cend())
          {
            symbol = this->lookup_elf_symbol(name);
          }
          return symbol;
        }

        /*
         * Get dynamic symbol relocations without addend (non PLT)
         */
        const std::vector<::elf::elf_rel> &get_relocations() const
        {
          return this->dyn_rel_entries;
        }

        /*
         * Get dynamic symbol relocations with addend (non PLT)
         */
        const std::vector<::elf::elf_rela> &get_relocations_with_addend() const
        {
          return this->dyn_rela_entries;
        }

        /*
         * Get PLT relocations without addend
         */
        const std::vector<::elf::elf_rel> &get_plt_relocations() const
        {
          return this->plt_rel_entries;
        }

        /*
         * Get PLT relocations with addend
         */
        const std::vector<::elf::elf_rela> &get_plt_relocations_with_addend() const
        {
          return this->plt_rela_entries;
        }

    private:
        const std::filesystem::path path;
        std::ifstream binary_file;
        std::string last_error;

        ::elf::elf_header header{0};
        std::vector<::elf::elf_program_header> program_headers;
        std::vector<::elf::elf_section_header> section_headers;
        std::vector<char> section_header_string_table;
        std::vector<::elf::elf_dynamic> dynamic_entries;
        std::vector<char> dynamic_segment_string_table;
        const char *so_name = nullptr;
        std::vector<const char *> needed_libraries;
        std::vector<elf::elf_symbol> dynamic_symbols;
        std::vector<std::uint64_t> init_functions;
        std::vector<std::uint64_t> fini_functions;
        std::uint64_t base_address = 0;

        std::vector<std::uint32_t> hash_buckets;
        std::vector<std::uint32_t> hash_chains;
        std::vector<std::uint32_t> gnu_hash_buckets;
        std::vector<std::uint32_t> gnu_hash_values;
        std::uint32_t gnu_hash_bloom_shift = 0;
        std::uint32_t gnu_hash_omitted_symbols_count = 0;
        std::vector<std::uint64_t> gnu_hash_bloom_words;

        std::vector<::elf::elf_rel> plt_rel_entries;
        std::vector<::elf::elf_rel> dyn_rel_entries;
        std::vector<::elf::elf_rela> plt_rela_entries;
        std::vector<::elf::elf_rela> dyn_rela_entries;

    private:
        bool open_file()
        {
          if (!std::filesystem::exists(this->path))
          {
            this->last_error = "File does not exist";
            return false;
          }

          this->binary_file.open(this->path, std::ios::binary | std::ios::in);
          if (!this->binary_file.is_open())
          {
            this->last_error = "Failed to open library file";
            return false;
          }

          return true;
        }

        bool read_elf_header()
        {
          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
            return false;
          }
          this->binary_file.clear();
          this->binary_file.seekg(0);
          this->binary_file.read(reinterpret_cast<char *>(&this->header.e_ident), sizeof(this->header.e_ident));
          if (this->binary_file.gcount() != sizeof(this->header.e_ident))
          {
            this->last_error = "Failed to read ELF identification";
            return false;
          }

          if (this->is_64_bit())
          {
            this->binary_file.seekg(0);
            this->binary_file.read(reinterpret_cast<char *>(&this->header), sizeof(this->header));
            if (this->binary_file.gcount() != sizeof(this->header))
            {
              this->last_error = "Failed to read ELF header";
              return false;
            }
          } else if (this->is_32_bit())
          {
            ::elf::types::Elf32_Ehdr real_header{0};
            this->binary_file.seekg(0);
            this->binary_file.read(reinterpret_cast<char *>(&real_header), sizeof(real_header));
            if (this->binary_file.gcount() != sizeof(real_header))
            {
              this->last_error = "Failed to read ELF header";
              return false;
            }
            this->header.e_type = real_header.e_type;
            this->header.e_machine = real_header.e_machine;
            this->header.e_version = real_header.e_version;
            this->header.e_entry = real_header.e_entry;
            this->header.e_phoff = real_header.e_phoff;
            this->header.e_shoff = real_header.e_shoff;
            this->header.e_flags = real_header.e_flags;
            this->header.e_ehsize = real_header.e_ehsize;
            this->header.e_phentsize = real_header.e_phentsize;
            this->header.e_phnum = real_header.e_phnum;
            this->header.e_shentsize = real_header.e_shentsize;
            this->header.e_shnum = real_header.e_shnum;
            this->header.e_shstrndx = real_header.e_shstrndx;
          } else
          {
            this->last_error = "Invalid ELF class";
            return false;
          }

          return true;
        }

        bool read_program_headers()
        {
          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
            return false;
          }
          this->binary_file.clear();
          this->binary_file.seekg(static_cast<std::streamoff>(this->header.e_phoff));
          this->program_headers.resize(this->header.e_phnum);

          if (this->is_64_bit())
          {
            if (this->header.e_phentsize != sizeof(::elf::types::Elf64_Phdr))
            {
              this->last_error = "Invalid program header size";
              return false;
            }
            auto program_headers_size = static_cast<std::streamsize>(this->header.e_phnum * this->header.e_phentsize);
            this->binary_file.read(reinterpret_cast<char *>(this->program_headers.data()), program_headers_size);
            if (this->binary_file.gcount() != program_headers_size)
            {
              this->last_error = "Failed to read program headers";
              return false;
            }
          } else
          {
            if (this->header.e_phentsize != sizeof(::elf::types::Elf32_Phdr))
            {
              this->last_error = "Invalid program header size";
              return false;
            }
            std::vector<::elf::types::Elf32_Phdr> real_program_headers(this->header.e_phnum);
            auto real_program_headers_size = static_cast<std::streamsize>(this->header.e_phnum * this->header.e_phentsize);
            this->binary_file.read(reinterpret_cast<char *>(real_program_headers.data()), real_program_headers_size);
            if (this->binary_file.gcount() != real_program_headers_size)
            {
              this->last_error = "Failed to read program headers";
              return false;
            }
            for (std::size_t i = 0; i < this->header.e_phnum; i++)
            {
              this->program_headers[i].p_type = real_program_headers[i].p_type;
              this->program_headers[i].p_flags = real_program_headers[i].p_flags;
              this->program_headers[i].p_offset = real_program_headers[i].p_offset;
              this->program_headers[i].p_vaddr = real_program_headers[i].p_vaddr;
              this->program_headers[i].p_paddr = real_program_headers[i].p_paddr;
              this->program_headers[i].p_filesz = real_program_headers[i].p_filesz;
              this->program_headers[i].p_memsz = real_program_headers[i].p_memsz;
              this->program_headers[i].p_align = real_program_headers[i].p_align;
            }
          }

          this->base_address = 0xFFFFFFFFFFFFFFFF;
          for (const auto &program_header: this->program_headers)
          {
            if (program_header.p_type == ::elf::elf_program_header::PT_LOAD)
            {
              this->base_address = std::min(this->base_address, program_header.p_vaddr);
            }
          }

          return true;
        }

        bool read_section_headers()
        {
          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
            return false;
          }
          this->binary_file.clear();
          this->binary_file.seekg(static_cast<std::streamoff>(this->header.e_shoff));
          this->section_headers.resize(this->header.e_shnum);

          if (this->is_64_bit())
          {
            if (this->header.e_shentsize != sizeof(::elf::types::Elf64_Shdr))
            {
              this->last_error = "Invalid section header size";
              return false;
            }
            for (std::size_t i = 0; i < this->header.e_shnum; i++)
            {
              this->binary_file.read(reinterpret_cast<char *>(&this->section_headers[i]), this->header.e_shentsize);
              if (this->binary_file.gcount() != this->header.e_shentsize)
              {
                this->last_error = "Failed to read section header";
                return false;
              }
            }
          } else
          {
            if (this->header.e_shentsize != sizeof(::elf::types::Elf32_Shdr))
            {
              this->last_error = "Invalid section header size";
              return false;
            }
            std::vector<::elf::types::Elf32_Shdr> real_section_headers(this->header.e_shnum);
            const auto real_section_headers_size = static_cast<std::streamsize>(this->header.e_shnum * this->header.e_shentsize);
            this->binary_file.read(reinterpret_cast<char *>(real_section_headers.data()), real_section_headers_size);
            if (this->binary_file.gcount() != real_section_headers_size)
            {
              this->last_error = "Failed to read section headers";
              return false;
            }
            for (std::size_t i = 0; i < this->header.e_shnum; i++)
            {
              this->section_headers[i].sh_name = real_section_headers[i].sh_name;
              this->section_headers[i].sh_type = real_section_headers[i].sh_type;
              this->section_headers[i].sh_flags = real_section_headers[i].sh_flags;
              this->section_headers[i].sh_addr = real_section_headers[i].sh_addr;
              this->section_headers[i].sh_offset = real_section_headers[i].sh_offset;
              this->section_headers[i].sh_size = real_section_headers[i].sh_size;
              this->section_headers[i].sh_link = real_section_headers[i].sh_link;
              this->section_headers[i].sh_info = real_section_headers[i].sh_info;
              this->section_headers[i].sh_addralign = real_section_headers[i].sh_addralign;
              this->section_headers[i].sh_entsize = real_section_headers[i].sh_entsize;
            }
          }

          std::uint32_t section_header_string_table_idx = this->header.e_shstrndx;
          if (section_header_string_table_idx == ::elf::elf_section_header::SHN_XINDEX)
          {
            section_header_string_table_idx = this->section_headers[0].sh_link;
          }
          const auto &section_header_string_table_header = this->section_headers[section_header_string_table_idx];
          this->section_header_string_table.resize(section_header_string_table_header.sh_size);
          const auto section_header_string_table_offset = static_cast<std::streamoff>(section_header_string_table_header.sh_offset);
          const auto section_header_string_table_size = static_cast<std::streamsize>(section_header_string_table_header.sh_size);
          this->binary_file.seekg(section_header_string_table_offset);
          this->binary_file.read(this->section_header_string_table.data(), section_header_string_table_size);
          if (this->binary_file.gcount() != section_header_string_table_size)
          {
            this->last_error = "Failed to read section header string table";
            return false;
          }

          for (auto &section_header: this->section_headers)
          {
            section_header.sh_name_str = this->section_header_string_table.data() + section_header.sh_name;
          }

          return true;
        }

        bool parse_hash_tables()
        {
          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
            return false;
          }

          for (const auto &section_header: this->section_headers)
          {
            switch (section_header.sh_type)
            {
              case ::elf::elf_section_header::SHT_HASH:
              {
                typedef struct hash_table_header
                {
                    std::uint32_t nbucket;
                    std::uint32_t nchain;
                } hash_table_header;

                hash_table_header table_header;
                this->binary_file.seekg(static_cast<std::streamoff>(section_header.sh_offset));
                this->binary_file.read(reinterpret_cast<char *>(&table_header), sizeof(table_header));
                if (this->binary_file.gcount() != sizeof(table_header))
                {
                  this->last_error = "Failed to read hash table header";
                  return false;
                }

                if (table_header.nbucket == 0 || table_header.nchain == 0)
                {
                  this->last_error = "Invalid hash table header";
                  return false;
                }

                this->hash_buckets.resize(table_header.nbucket);
                this->binary_file.read(reinterpret_cast<char *>(this->hash_buckets.data()),
                                       static_cast<std::streamsize>(sizeof(std::uint32_t) * table_header.nbucket));
                if (this->binary_file.gcount() != sizeof(std::uint32_t) * table_header.nbucket)
                {
                  this->last_error = "Failed to read hash table buckets";
                  return false;
                }
                this->hash_chains.resize(table_header.nchain);
                this->binary_file.read(reinterpret_cast<char *>(this->hash_chains.data()),
                                       static_cast<std::streamsize>(sizeof(std::uint32_t) * table_header.nchain));
                if (this->binary_file.gcount() != sizeof(std::uint32_t) * table_header.nchain)
                {
                  this->last_error = "Failed to read hash table chains";
                  return false;
                }

                break;
              }
              case ::elf::elf_section_header::SHT_GNU_HASH:
              {
                typedef struct gnu_hash_table_header
                {
                    std::uint32_t nbuckets;
                    std::uint32_t omitted_symbols_count;
                    std::uint32_t bloom_size;
                    std::uint32_t bloom_shift;
                } gnu_hash_table_header;

                gnu_hash_table_header table_header;
                this->binary_file.seekg(static_cast<std::streamoff>(section_header.sh_offset));
                this->binary_file.read(reinterpret_cast<char *>(&table_header), sizeof(table_header));
                if (this->binary_file.gcount() != sizeof(table_header))
                {
                  this->last_error = "Failed to read gnu hash table header";
                  return false;
                }
                this->gnu_hash_bloom_shift = table_header.bloom_shift;
                this->gnu_hash_omitted_symbols_count = table_header.omitted_symbols_count;

                if (this->is_64_bit())
                {
                  this->gnu_hash_bloom_words.resize(table_header.bloom_size);
                  const auto bloom_words_size = static_cast<std::streamsize>(sizeof(std::uint64_t) * table_header.bloom_size);
                  this->binary_file.read(reinterpret_cast<char *>(this->gnu_hash_bloom_words.data()), bloom_words_size);
                  if (this->binary_file.gcount() != bloom_words_size)
                  {
                    this->last_error = "Failed to read gnu hash table bloom words";
                    return false;
                  }
                } else if (this->is_32_bit())
                {
                  std::vector<std::uint32_t> real_bloom_words(table_header.bloom_size);
                  const auto real_bloom_words_size = static_cast<std::streamsize>(sizeof(std::uint32_t) * table_header.bloom_size);
                  this->binary_file.read(reinterpret_cast<char *>(real_bloom_words.data()), real_bloom_words_size);
                  if (this->binary_file.gcount() != real_bloom_words_size)
                  {
                    this->last_error = "Failed to read gnu hash table bloom words";
                    return false;
                  }
                  this->gnu_hash_bloom_words.resize(table_header.bloom_size);
                  for (std::size_t i = 0; i < table_header.bloom_size; i++)
                  {
                    this->gnu_hash_bloom_words[i] = real_bloom_words[i];
                  }
                } else
                {
                  this->last_error = "Invalid ELF class";
                  return false;
                }

                this->gnu_hash_buckets.resize(table_header.nbuckets);
                const auto buckets_size = static_cast<std::streamsize>(sizeof(std::uint32_t) * table_header.nbuckets);
                this->binary_file.read(reinterpret_cast<char *>(this->gnu_hash_buckets.data()), buckets_size);
                if (this->binary_file.gcount() != buckets_size)
                {
                  this->last_error = "Failed to read gnu hash table buckets";
                  return false;
                }

                std::size_t hash_values_count = this->dynamic_symbols.size() - this->gnu_hash_omitted_symbols_count;
                const auto hash_values_size = static_cast<std::streamsize>(sizeof(std::uint32_t) * hash_values_count);
                this->gnu_hash_values.resize(hash_values_count);
                this->binary_file.read(reinterpret_cast<char *>(this->gnu_hash_values.data()), hash_values_size);
                if (this->binary_file.gcount() != hash_values_size)
                {
                  this->last_error = "Failed to read gnu hash table values";
                  return false;
                }

                break;
              }
            }
          }

          return true;
        }

        bool parse_relocations()
        {
          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
            return false;
          }

          for (const auto &section_header: this->get_section_headers())
          {
            switch (section_header.sh_type)
            {
              case ::elf::elf_section_header::SHT_REL:
              {
                this->binary_file.seekg(static_cast<std::streamoff>(section_header.sh_offset));
                if (this->is_64_bit())
                {
                  if (section_header.sh_entsize != sizeof(::elf::types::Elf64_Rel))
                  {
                    this->last_error = "Invalid relocation entry size";
                    return false;
                  }
                  std::size_t relocation_entries_count = section_header.sh_size / section_header.sh_entsize;

                  if (std::strcmp(section_header.sh_name_str, ".rel.plt") == 0)
                  {
                    this->plt_rel_entries.resize(relocation_entries_count);
                    this->binary_file.read(reinterpret_cast<char *>(this->plt_rel_entries.data()), static_cast<std::streamsize>(section_header.sh_size));
                  } else if (std::strcmp(section_header.sh_name_str, ".rel.dyn") == 0)
                  {
                    this->dyn_rel_entries.resize(relocation_entries_count);
                    this->binary_file.read(reinterpret_cast<char *>(this->dyn_rel_entries.data()), static_cast<std::streamsize>(section_header.sh_size));
                  } else
                  {
                    this->last_error = "Invalid relocation section name";
                    return false;
                  }

                  if (this->binary_file.gcount() != section_header.sh_size)
                  {
                    this->last_error = "Failed to read relocation entries";
                    return false;
                  }
                } else
                {
                  if (section_header.sh_entsize != sizeof(::elf::types::Elf32_Rel))
                  {
                    this->last_error = "Invalid relocation entry size";
                    return false;
                  }
                  std::size_t relocation_entries_count = section_header.sh_size / section_header.sh_entsize;
                  std::vector<::elf::types::Elf32_Rel> real_relocation_entries(relocation_entries_count);
                  this->binary_file.read(reinterpret_cast<char *>(real_relocation_entries.data()), static_cast<std::streamsize>(section_header.sh_size));
                  if (this->binary_file.gcount() != section_header.sh_size)
                  {
                    this->last_error = "Failed to read relocation entries";
                    return false;
                  }

                  if (std::strcmp(section_header.sh_name_str, ".rel.plt") == 0)
                  {
                    this->plt_rel_entries.resize(relocation_entries_count);
                    for (std::size_t i = 0; i < relocation_entries_count; i++)
                    {
                      this->plt_rel_entries[i].r_offset = real_relocation_entries[i].r_offset;
                      this->plt_rel_entries[i].r_type = real_relocation_entries[i].r_info & 0xff;
                      this->plt_rel_entries[i].r_sym = real_relocation_entries[i].r_info >> 8;
                    }
                  } else if (std::strcmp(section_header.sh_name_str, ".rel.dyn") == 0)
                  {
                    this->dyn_rel_entries.resize(relocation_entries_count);
                    for (std::size_t i = 0; i < relocation_entries_count; i++)
                    {
                      this->dyn_rel_entries[i].r_offset = real_relocation_entries[i].r_offset;
                      this->dyn_rel_entries[i].r_type = real_relocation_entries[i].r_info & 0xff;
                      this->dyn_rel_entries[i].r_sym = real_relocation_entries[i].r_info >> 8;
                    }
                  } else
                  {
                    this->last_error = "Invalid relocation section name";
                    return false;
                  }
                }
                break;
              }
              case ::elf::elf_section_header::SHT_RELA:
              {
                this->binary_file.seekg(static_cast<std::streamoff>(section_header.sh_offset));
                if (this->is_64_bit())
                {
                  if (section_header.sh_entsize != sizeof(::elf::types::Elf64_Rela))
                  {
                    this->last_error = "Invalid relocation entry size";
                    return false;
                  }
                  std::size_t relocation_entries_count = section_header.sh_size / section_header.sh_entsize;

                  if (std::strcmp(section_header.sh_name_str, ".rela.plt") == 0)
                  {
                    this->plt_rela_entries.resize(relocation_entries_count);
                    this->binary_file.read(reinterpret_cast<char *>(this->plt_rela_entries.data()),
                                           static_cast<std::streamsize>(section_header.sh_size));
                  } else if (std::strcmp(section_header.sh_name_str, ".rela.dyn") == 0)
                  {
                    this->dyn_rela_entries.resize(relocation_entries_count);
                    this->binary_file.read(reinterpret_cast<char *>(this->dyn_rela_entries.data()),
                                           static_cast<std::streamsize>(section_header.sh_size));
                  } else
                  {
                    this->last_error = "Invalid relocation section name";
                    return false;
                  }

                  if (this->binary_file.gcount() != section_header.sh_size)
                  {
                    this->last_error = "Failed to read relocation entries";
                    return false;
                  }
                } else
                {
                  if (section_header.sh_entsize != sizeof(::elf::types::Elf32_Rela))
                  {
                    this->last_error = "Invalid relocation entry size";
                    return false;
                  }
                  std::size_t relocation_entries_count = section_header.sh_size / section_header.sh_entsize;
                  std::vector<::elf::types::Elf32_Rela> real_relocation_entries(relocation_entries_count);
                  this->binary_file.read(reinterpret_cast<char *>(real_relocation_entries.data()), static_cast<std::streamsize>(section_header.sh_size));
                  if (this->binary_file.gcount() != section_header.sh_size)
                  {
                    this->last_error = "Failed to read relocation entries";
                    return false;
                  }

                  if (std::strcmp(section_header.sh_name_str, ".rela.plt") == 0)
                  {
                    this->plt_rela_entries.resize(relocation_entries_count);
                    for (std::size_t i = 0; i < relocation_entries_count; i++)
                    {
                      this->plt_rela_entries[i].r_offset = real_relocation_entries[i].r_offset;
                      this->plt_rela_entries[i].r_type = real_relocation_entries[i].r_info & 0xff;
                      this->plt_rela_entries[i].r_sym = real_relocation_entries[i].r_info >> 8;
                      this->plt_rela_entries[i].r_addend = real_relocation_entries[i].r_addend;
                    }
                  } else if (std::strcmp(section_header.sh_name_str, ".rela.dyn") == 0)
                  {
                    this->dyn_rela_entries.resize(relocation_entries_count);
                    for (std::size_t i = 0; i < relocation_entries_count; i++)
                    {
                      this->dyn_rela_entries[i].r_offset = real_relocation_entries[i].r_offset;
                      this->dyn_rela_entries[i].r_type = real_relocation_entries[i].r_info & 0xff;
                      this->dyn_rela_entries[i].r_sym = real_relocation_entries[i].r_info >> 8;
                      this->dyn_rela_entries[i].r_addend = real_relocation_entries[i].r_addend;
                    }
                  } else
                  {
                    this->last_error = "Invalid relocation section name";
                    return false;
                  }
                }
                break;
              }
            }
          }

          return true;
        }

        bool read_init_functions()
        {
          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
            return false;
          }

          const auto preinit_array_section_header = std::find_if(this->section_headers.cbegin(), this->section_headers.cend(),
                                                                 [](const elf::elf_section_header &hdr) {
                                                                     return std::strcmp(hdr.sh_name_str, ".preinit_array") == 0;
                                                                 });
          if (preinit_array_section_header != this->section_headers.cend())
          {
            std::size_t preinit_entry_size = this->is_64_bit() ? 8 : 4;
            if (preinit_array_section_header->sh_size % preinit_entry_size != 0)
            {
              this->last_error = "Invalid preinit array size";
              return false;
            }
            std::size_t preinit_entry_count = preinit_array_section_header->sh_size / preinit_entry_size;
            this->binary_file.seekg(static_cast<std::streamoff>(preinit_array_section_header->sh_offset));
            if (this->is_64_bit())
            {
              this->init_functions.resize(preinit_entry_count);
              this->binary_file.read(reinterpret_cast<char *>(this->init_functions.data()),
                                     static_cast<std::streamsize>(preinit_array_section_header->sh_size));
              if (this->binary_file.gcount() != preinit_array_section_header->sh_size)
              {
                this->last_error = "Failed to read preinit array";
                return false;
              }
            } else
            {
              std::vector<std::uint32_t> real_preinit_functions(preinit_entry_count);
              this->binary_file.read(reinterpret_cast<char *>(real_preinit_functions.data()),
                                     static_cast<std::streamsize>(preinit_array_section_header->sh_size));
              if (this->binary_file.gcount() != preinit_array_section_header->sh_size)
              {
                this->last_error = "Failed to read preinit array";
                return false;
              }
              this->init_functions.resize(preinit_entry_count);
              for (std::size_t i = 0; i < preinit_entry_count; i++)
              {
                this->init_functions[i] = real_preinit_functions[i];
              }
            }
          }

          const auto init_section_header = std::find_if(this->section_headers.cbegin(), this->section_headers.cend(),
                                                        [](const elf::elf_section_header &hdr) {
                                                            return std::strcmp(hdr.sh_name_str, ".init") == 0;
                                                        });
          if (init_section_header != this->section_headers.cend())
          {
            this->init_functions.emplace_back(init_section_header->sh_addr);
          }

          const auto init_array_section_header = std::find_if(this->section_headers.cbegin(), this->section_headers.cend(),
                                                              [](const elf::elf_section_header &hdr) {
                                                                  return std::strcmp(hdr.sh_name_str, ".init_array") == 0;
                                                              });
          if (init_array_section_header != this->section_headers.cend())
          {
            std::size_t init_entry_size = this->is_64_bit() ? 8 : 4;
            if (init_array_section_header->sh_size % init_entry_size != 0)
            {
              this->last_error = "Invalid preinit array size";
              return false;
            }
            std::size_t init_entry_count = init_array_section_header->sh_size / init_entry_size;
            this->binary_file.seekg(static_cast<std::streamoff>(init_array_section_header->sh_offset));
            if (this->is_64_bit())
            {
              this->init_functions.resize(this->init_functions.size() + init_entry_count);
              this->binary_file.read(reinterpret_cast<char *>(this->init_functions.data() + this->init_functions.size() - init_entry_count),
                                     static_cast<std::streamsize>(init_array_section_header->sh_size));
              if (this->binary_file.gcount() != init_array_section_header->sh_size)
              {
                this->last_error = "Failed to read preinit array";
                return false;
              }
            } else
            {
              std::vector<std::uint32_t> real_init_functions(init_entry_count);
              this->binary_file.read(reinterpret_cast<char *>(real_init_functions.data()), static_cast<std::streamsize>(init_array_section_header->sh_size));
              if (this->binary_file.gcount() != init_array_section_header->sh_size)
              {
                this->last_error = "Failed to read preinit array";
                return false;
              }
              this->init_functions.reserve(this->init_functions.size() + init_entry_count);
              std::copy(real_init_functions.cbegin(), real_init_functions.cend(), std::back_inserter(this->init_functions));
            }
          }

          return true;
        }

        bool read_term_functions()
        {
          const auto fini_array_section_header = std::find_if(this->section_headers.cbegin(), this->section_headers.cend(),
                                                              [](const elf::elf_section_header &hdr) {
                                                                  return std::strcmp(hdr.sh_name_str, ".fini_array") == 0;
                                                              });
          if (fini_array_section_header != this->section_headers.cend())
          {
            std::size_t fini_entry_size = this->is_64_bit() ? 8 : 4;
            if (fini_array_section_header->sh_size % fini_entry_size != 0)
            {
              this->last_error = "Invalid fini array size";
              return false;
            }
            std::size_t fini_entry_count = fini_array_section_header->sh_size / fini_entry_size;
            this->binary_file.seekg(static_cast<std::streamoff>(fini_array_section_header->sh_offset));
            if (this->is_64_bit())
            {
              this->fini_functions.resize(fini_entry_count);
              this->binary_file.read(reinterpret_cast<char *>(this->fini_functions.data()),
                                     static_cast<std::streamsize>(fini_array_section_header->sh_size));
              if (this->binary_file.gcount() != fini_array_section_header->sh_size)
              {
                this->last_error = "Failed to read fini array";
                return false;
              }
              std::reverse(this->fini_functions.begin(), this->fini_functions.end());
            } else
            {
              std::vector<std::uint32_t> real_fini_functions(fini_entry_count);
              this->binary_file.read(reinterpret_cast<char *>(real_fini_functions.data()),
                                     static_cast<std::streamsize>(fini_array_section_header->sh_size));
              if (this->binary_file.gcount() != fini_array_section_header->sh_size)
              {
                this->last_error = "Failed to read fini array";
                return false;
              }
              this->fini_functions.reserve(fini_entry_count);
              std::copy(real_fini_functions.crbegin(), real_fini_functions.crend(), std::back_inserter(this->fini_functions));
            }
          }

          const auto fini_section_header = std::find_if(this->section_headers.cbegin(), this->section_headers.cend(),
                                                        [](const elf::elf_section_header &hdr) {
                                                            return std::strcmp(hdr.sh_name_str, ".fini") == 0;
                                                        });
          if (fini_section_header != this->section_headers.cend())
          {
            this->fini_functions.emplace_back(fini_section_header->sh_addr);
          }

          return true;
        }

        std::vector<::elf::elf_symbol>::const_iterator lookup_elf_symbol(const char *name) const
        {
          if (this->hash_buckets.empty())
          {
            return this->dynamic_symbols.cend();
          }
          const std::uint32_t hash = elf_hash(name);
          std::uint32_t index = this->hash_buckets[hash % this->hash_buckets.size()];
          while (true)
          {
            if (index == ::elf::elf_symbol::STN_UNDEF)
            {
              return this->dynamic_symbols.cend();
            }
            auto symbol_it = this->dynamic_symbols.cbegin();
            std::advance(symbol_it, index);
            if (std::strcmp(symbol_it->st_name_str, name) == 0)
            {
              return symbol_it;
            }
            index = this->hash_chains.at(index);
          }
        }

        std::vector<::elf::elf_symbol>::const_iterator lookup_gnu_symbol(const char *name) const
        {
          if (this->gnu_hash_buckets.empty())
          {
            return this->dynamic_symbols.cend();
          }
          std::uint32_t hash1 = gnu_hash(name);
          std::uint32_t hash2 = hash1 >> this->gnu_hash_bloom_shift;
          const std::uint64_t bloom_word_size = sizeof(std::uint64_t) * 8;
          const std::uint64_t bitmask = (static_cast<std::uint64_t>(1) << (hash1 % bloom_word_size)) |
                                        (static_cast<std::uint64_t>(1) << (hash2 % bloom_word_size));
          const std::uint64_t bloom_size_bitmask = this->gnu_hash_bloom_words.size() - 1;

          if ((this->gnu_hash_bloom_words.at((hash1 / bloom_word_size) & bloom_size_bitmask) & bitmask) != bitmask)
          {
            return this->dynamic_symbols.cend();
          }

          std::size_t start_idx = this->gnu_hash_buckets.at(hash1 % this->gnu_hash_buckets.size());
          if (start_idx == ::elf::elf_symbol::STN_UNDEF)
          {
            return this->dynamic_symbols.cend();
          }

          auto current_symbol = this->dynamic_symbols.cbegin();
          std::advance(current_symbol, start_idx);
          auto current_value = this->gnu_hash_values.cbegin();
          std::advance(current_value, start_idx - this->gnu_hash_omitted_symbols_count);

          hash1 &= ~1;
          for (; current_symbol != this->dynamic_symbols.cend() && current_value != this->gnu_hash_values.cend(); current_symbol++, current_value++)
          {
            hash2 = *current_value;

            if (hash1 != (hash2 & ~1))
            {
              continue;
            }

            if (std::strcmp(current_symbol->st_name_str, name) == 0)
            {
              return current_symbol;
            }

            if (hash2 & 1)
            {
              return this->dynamic_symbols.cend();
            }
          }

          return this->dynamic_symbols.cend();
        }
    };
}
