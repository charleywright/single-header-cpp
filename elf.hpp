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
#include <byteswap.h>
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
 *
 * TODO:
 * - Properly handle endianness
 */

namespace elf
{
    typedef std::uint8_t byte;

    typedef struct elf_ident
    {
        static constexpr byte EI_MAG0 = 0;
        static constexpr byte ELFMAG0 = 0x7f;
        static constexpr byte EI_MAG1 = 1;
        static constexpr byte ELFMAG1 = 'E';
        static constexpr byte EI_MAG2 = 2;
        static constexpr byte ELFMAG2 = 'L';
        static constexpr byte EI_MAG3 = 3;
        static constexpr byte ELFMAG3 = 'F';
        ::elf::byte ei_magic[4];  /* File identification */

        static constexpr byte ELFCLASSNONE = 0;  /* Invalid class */
        static constexpr byte ELFCLASS32 = 1;    /* 32-bit objects */
        static constexpr byte ELFCLASS64 = 2;    /* 64-bit objects */
        ::elf::byte ei_class;  /* File class */

        static constexpr byte ELFDATANONE = 0;   /* Invalid data encoding */
        static constexpr byte ELFDATA2LSB = 1;   /* 2's complement, little endian */
        static constexpr byte ELFDATA2MSB = 2;   /* 2's complement, big endian */
        ::elf::byte ei_data;  /* Data encoding */

        static constexpr byte EV_NONE = 0;       /* Invalid version */
        static constexpr byte EV_CURRENT = 1;    /* Current version */
        ::elf::byte ei_version;  /* File version */

        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L135-L150
        static constexpr byte ELFOSABI_NONE = 0;              /* UNIX System V ABI */
        static constexpr byte ELFOSABI_SYSV = 0;              /* Alias.  */
        static constexpr byte ELFOSABI_HPUX = 1;              /* HP-UX */
        static constexpr byte ELFOSABI_NETBSD = 2;            /* NetBSD.  */
        static constexpr byte ELFOSABI_GNU = 3;               /* Object uses GNU ELF extensions.  */
        static constexpr byte ELFOSABI_LINUX = ELFOSABI_GNU;  /* Compatibility alias.  */
        static constexpr byte ELFOSABI_SOLARIS = 6;           /* Sun Solaris.  */
        static constexpr byte ELFOSABI_AIX = 7;               /* IBM AIX.  */
        static constexpr byte ELFOSABI_IRIX = 8;              /* SGI Irix.  */
        static constexpr byte ELFOSABI_FREEBSD = 9;           /* FreeBSD.  */
        static constexpr byte ELFOSABI_TRU64 = 10;            /* Compaq TRU64 UNIX.  */
        static constexpr byte ELFOSABI_MODESTO = 11;          /* Novell Modesto.  */
        static constexpr byte ELFOSABI_OPENBSD = 12;          /* OpenBSD.  */
        static constexpr byte ELFOSABI_ARM_AEABI = 64;        /* ARM EABI */
        static constexpr byte ELFOSABI_ARM = 97;              /* ARM */
        static constexpr byte ELFOSABI_STANDALONE = 255;      /* Standalone (embedded) application */
        ::elf::byte ei_osabi;  /* OS-specific ELF extensions */

        static constexpr byte ELFAABIVERSION_UNSPECIFIED = 0; /* Unspecified */
        // Other values are OSABI-specific.
        ::elf::byte ei_abiversion;  /* ABI version */

        ::elf::byte ei_pad[7];
    } elf_ident;

    // Same layout as Elf64_Ehdr for optimization
    typedef struct elf_header
    {
        ::elf::elf_ident e_ident;  /* Machine-independent identification */

        static constexpr std::uint16_t ET_NONE = 0;         /* No file type */
        static constexpr std::uint16_t ET_REL = 1;          /* Relocatable file */
        static constexpr std::uint16_t ET_EXEC = 2;         /* Executable file */
        static constexpr std::uint16_t ET_DYN = 3;          /* Shared object file */
        static constexpr std::uint16_t ET_CORE = 4;         /* Core file */
        static constexpr std::uint16_t ET_LOOS = 0xfe00;    /* Operating system-specific */
        static constexpr std::uint16_t ET_HIOS = 0xfeff;    /* Operating system-specific */
        static constexpr std::uint16_t ET_LOPROC = 0xff00;  /* Processor-specific */
        static constexpr std::uint16_t ET_HIPROC = 0xffff;  /* Processor-specific */
        std::uint16_t e_type;  /* Object file type */

        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L169-L373
        static constexpr std::uint16_t EM_NONE = 0;             /* No machine */
        static constexpr std::uint16_t EM_M32 = 1;              /* AT&T WE 32100 */
        static constexpr std::uint16_t EM_SPARC = 2;            /* SUN SPARC */
        static constexpr std::uint16_t EM_386 = 3;              /* Intel 80386 */
        static constexpr std::uint16_t EM_68K = 4;              /* Motorola m68k family */
        static constexpr std::uint16_t EM_88K = 5;              /* Motorola m88k family */
        static constexpr std::uint16_t EM_IAMCU = 6;            /* Intel MCU */
        static constexpr std::uint16_t EM_860 = 7;              /* Intel 80860 */
        static constexpr std::uint16_t EM_MIPS = 8;             /* MIPS R3000 big-endian */
        static constexpr std::uint16_t EM_S370 = 9;             /* IBM System/370 */
        static constexpr std::uint16_t EM_MIPS_RS3_LE = 10;     /* MIPS R3000 little-endian */
        /* reserved 11-14 */
        static constexpr std::uint16_t EM_PARISC = 15;          /* HPPA */
        /* reserved 16 */
        static constexpr std::uint16_t EM_VPP500 = 17;          /* Fujitsu VPP500 */
        static constexpr std::uint16_t EM_SPARC32PLUS = 18;     /* Sun's "v8plus" */
        static constexpr std::uint16_t EM_960 = 19;             /* Intel 80960 */
        static constexpr std::uint16_t EM_PPC = 20;             /* PowerPC */
        static constexpr std::uint16_t EM_PPC64 = 21;           /* PowerPC 64-bit */
        static constexpr std::uint16_t EM_S390 = 22;            /* IBM S390 */
        static constexpr std::uint16_t EM_SPU = 23;             /* IBM SPU/SPC */
        /* reserved 24-35 */
        static constexpr std::uint16_t EM_V800 = 36;            /* NEC V800 series */
        static constexpr std::uint16_t EM_FR20 = 37;            /* Fujitsu FR20 */
        static constexpr std::uint16_t EM_RH32 = 38;            /* TRW RH-32 */
        static constexpr std::uint16_t EM_RCE = 39;             /* Motorola RCE */
        static constexpr std::uint16_t EM_ARM = 40;             /* ARM */
        static constexpr std::uint16_t EM_FAKE_ALPHA = 41;      /* Digital Alpha */
        static constexpr std::uint16_t EM_SH = 42;              /* Hitachi SH */
        static constexpr std::uint16_t EM_SPARCV9 = 43;         /* SPARC v9 64-bit */
        static constexpr std::uint16_t EM_TRICORE = 44;         /* Siemens Tricore */
        static constexpr std::uint16_t EM_ARC = 45;             /* Argonaut RISC Core */
        static constexpr std::uint16_t EM_H8_300 = 46;          /* Hitachi H8/300 */
        static constexpr std::uint16_t EM_H8_300H = 47;         /* Hitachi H8/300H */
        static constexpr std::uint16_t EM_H8S = 48;             /* Hitachi H8S */
        static constexpr std::uint16_t EM_H8_500 = 49;          /* Hitachi H8/500 */
        static constexpr std::uint16_t EM_IA_64 = 50;           /* Intel Merced */
        static constexpr std::uint16_t EM_MIPS_X = 51;          /* Stanford MIPS-X */
        static constexpr std::uint16_t EM_COLDFIRE = 52;        /* Motorola Coldfire */
        static constexpr std::uint16_t EM_68HC12 = 53;          /* Motorola M68HC12 */
        static constexpr std::uint16_t EM_MMA = 54;             /* Fujitsu MMA Multimedia Accelerator */
        static constexpr std::uint16_t EM_PCP = 55;             /* Siemens PCP */
        static constexpr std::uint16_t EM_NCPU = 56;            /* Sony nCPU embedded RISC */
        static constexpr std::uint16_t EM_NDR1 = 57;            /* Denso NDR1 microprocessor */
        static constexpr std::uint16_t EM_STARCORE = 58;        /* Motorola Start*Core processor */
        static constexpr std::uint16_t EM_ME16 = 59;            /* Toyota ME16 processor */
        static constexpr std::uint16_t EM_ST100 = 60;           /* STMicroelectronic ST100 processor */
        static constexpr std::uint16_t EM_TINYJ = 61;           /* Advanced Logic Corp. Tinyj emb.fam */
        static constexpr std::uint16_t EM_X86_64 = 62;          /* AMD x86-64 architecture */
        static constexpr std::uint16_t EM_PDSP = 63;            /* Sony DSP Processor */
        static constexpr std::uint16_t EM_PDP10 = 64;           /* Digital PDP-10 */
        static constexpr std::uint16_t EM_PDP11 = 65;           /* Digital PDP-11 */
        static constexpr std::uint16_t EM_FX66 = 66;            /* Siemens FX66 microcontroller */
        static constexpr std::uint16_t EM_ST9PLUS = 67;         /* STMicroelectronics ST9+ 8/16 mc */
        static constexpr std::uint16_t EM_ST7 = 68;             /* STmicroelectronics ST7 8 bit mc */
        static constexpr std::uint16_t EM_68HC16 = 69;          /* Motorola MC68HC16 microcontroller */
        static constexpr std::uint16_t EM_68HC11 = 70;          /* Motorola MC68HC11 microcontroller */
        static constexpr std::uint16_t EM_68HC08 = 71;          /* Motorola MC68HC08 microcontroller */
        static constexpr std::uint16_t EM_68HC05 = 72;          /* Motorola MC68HC05 microcontroller */
        static constexpr std::uint16_t EM_SVX = 73;             /* Silicon Graphics SVx */
        static constexpr std::uint16_t EM_ST19 = 74;            /* STMicroelectronics ST19 8 bit mc */
        static constexpr std::uint16_t EM_VAX = 75;             /* Digital VAX */
        static constexpr std::uint16_t EM_CRIS = 76;            /* Axis Communications 32-bit emb.proc */
        static constexpr std::uint16_t EM_JAVELIN = 77;         /* Infineon Technologies 32-bit emb.proc */
        static constexpr std::uint16_t EM_FIREPATH = 78;        /* Element 14 64-bit DSP Processor */
        static constexpr std::uint16_t EM_ZSP = 79;             /* LSI Logic 16-bit DSP Processor */
        static constexpr std::uint16_t EM_MMIX = 80;            /* Donald Knuth's educational 64-bit proc */
        static constexpr std::uint16_t EM_HUANY = 81;           /* Harvard University machine-independent object files */
        static constexpr std::uint16_t EM_PRISM = 82;           /* SiTera Prism */
        static constexpr std::uint16_t EM_AVR = 83;             /* Atmel AVR 8-bit microcontroller */
        static constexpr std::uint16_t EM_FR30 = 84;            /* Fujitsu FR30 */
        static constexpr std::uint16_t EM_D10V = 85;            /* Mitsubishi D10V */
        static constexpr std::uint16_t EM_D30V = 86;            /* Mitsubishi D30V */
        static constexpr std::uint16_t EM_V850 = 87;            /* NEC v850 */
        static constexpr std::uint16_t EM_M32R = 88;            /* Mitsubishi M32R */
        static constexpr std::uint16_t EM_MN10300 = 89;         /* Matsushita MN10300 */
        static constexpr std::uint16_t EM_MN10200 = 90;         /* Matsushita MN10200 */
        static constexpr std::uint16_t EM_PJ = 91;              /* picoJava */
        static constexpr std::uint16_t EM_OPENRISC = 92;        /* OpenRISC 32-bit embedded processor */
        static constexpr std::uint16_t EM_ARC_COMPACT = 93;     /* ARC International ARCompact */
        static constexpr std::uint16_t EM_XTENSA = 94;          /* Tensilica Xtensa Architecture */
        static constexpr std::uint16_t EM_VIDEOCORE = 95;       /* Alphamosaic VideoCore */
        static constexpr std::uint16_t EM_TMM_GPP = 96;         /* Thompson Multimedia General Purpose Proc */
        static constexpr std::uint16_t EM_NS32K = 97;           /* National Semi. 32000 */
        static constexpr std::uint16_t EM_TPC = 98;             /* Tenor Network TPC */
        static constexpr std::uint16_t EM_SNP1K = 99;           /* Trebia SNP 1000 */
        static constexpr std::uint16_t EM_ST200 = 100;          /* STMicroelectronics ST200 */
        static constexpr std::uint16_t EM_IP2K = 101;           /* Ubicom IP2xxx */
        static constexpr std::uint16_t EM_MAX = 102;            /* MAX processor */
        static constexpr std::uint16_t EM_CR = 103;             /* National Semi. CompactRISC */
        static constexpr std::uint16_t EM_F2MC16 = 104;         /* Fujitsu F2MC16 */
        static constexpr std::uint16_t EM_MSP430 = 105;         /* Texas Instruments msp430 */
        static constexpr std::uint16_t EM_BLACKFIN = 106;       /* Analog Devices Blackfin DSP */
        static constexpr std::uint16_t EM_SE_C33 = 107;         /* Seiko Epson S1C33 family */
        static constexpr std::uint16_t EM_SEP = 108;            /* Sharp embedded microprocessor */
        static constexpr std::uint16_t EM_ARCA = 109;           /* Arca RISC */
        static constexpr std::uint16_t EM_UNICORE = 110;        /* PKU-Unity & MPRC Peking Uni. mc series */
        static constexpr std::uint16_t EM_EXCESS = 111;         /* eXcess configurable cpu */
        static constexpr std::uint16_t EM_DXP = 112;            /* Icera Semi. Deep Execution Processor */
        static constexpr std::uint16_t EM_ALTERA_NIOS2 = 113;   /* Altera Nios II */
        static constexpr std::uint16_t EM_CRX = 114;            /* National Semi. CompactRISC CRX */
        static constexpr std::uint16_t EM_XGATE = 115;          /* Motorola XGATE */
        static constexpr std::uint16_t EM_C166 = 116;           /* Infineon C16x/XC16x */
        static constexpr std::uint16_t EM_M16C = 117;           /* Renesas M16C */
        static constexpr std::uint16_t EM_DSPIC30F = 118;       /* Microchip Technology dsPIC30F */
        static constexpr std::uint16_t EM_CE = 119;             /* Freescale Communication Engine RISC */
        static constexpr std::uint16_t EM_M32C = 120;           /* Renesas M32C */
        /* reserved 121-130 */
        static constexpr std::uint16_t EM_TSK3000 = 131;        /* Altium TSK3000 */
        static constexpr std::uint16_t EM_RS08 = 132;           /* Freescale RS08 */
        static constexpr std::uint16_t EM_SHARC = 133;          /* Analog Devices SHARC family */
        static constexpr std::uint16_t EM_ECOG2 = 134;          /* Cyan Technology eCOG2 */
        static constexpr std::uint16_t EM_SCORE7 = 135;         /* Sunplus S+core7 RISC */
        static constexpr std::uint16_t EM_DSP24 = 136;          /* New Japan Radio (NJR) 24-bit DSP */
        static constexpr std::uint16_t EM_VIDEOCORE3 = 137;     /* Broadcom VideoCore III */
        static constexpr std::uint16_t EM_LATTICEMICO32 = 138;  /* RISC for Lattice FPGA */
        static constexpr std::uint16_t EM_SE_C17 = 139;         /* Seiko Epson C17 */
        static constexpr std::uint16_t EM_TI_C6000 = 140;       /* Texas Instruments TMS320C6000 DSP */
        static constexpr std::uint16_t EM_TI_C2000 = 141;       /* Texas Instruments TMS320C2000 DSP */
        static constexpr std::uint16_t EM_TI_C5500 = 142;       /* Texas Instruments TMS320C55x DSP */
        static constexpr std::uint16_t EM_TI_ARP32 = 143;       /* Texas Instruments App. Specific RISC */
        static constexpr std::uint16_t EM_TI_PRU = 144;         /* Texas Instruments Prog. Realtime Unit */
        /* reserved 145-159 */
        static constexpr std::uint16_t EM_MMDSP_PLUS = 160;     /* STMicroelectronics 64bit VLIW DSP */
        static constexpr std::uint16_t EM_CYPRESS_M8C = 161;    /* Cypress M8C */
        static constexpr std::uint16_t EM_R32C = 162;           /* Renesas R32C */
        static constexpr std::uint16_t EM_TRIMEDIA = 163;       /* NXP Semi. TriMedia */
        static constexpr std::uint16_t EM_QDSP6 = 164;          /* QUALCOMM DSP6 */
        static constexpr std::uint16_t EM_8051 = 165;           /* Intel 8051 and variants */
        static constexpr std::uint16_t EM_STXP7X = 166;         /* STMicroelectronics STxP7x */
        static constexpr std::uint16_t EM_NDS32 = 167;          /* Andes Tech. compact code emb. RISC */
        static constexpr std::uint16_t EM_ECOG1X = 168;         /* Cyan Technology eCOG1X */
        static constexpr std::uint16_t EM_MAXQ30 = 169;         /* Dallas Semi. MAXQ30 mc */
        static constexpr std::uint16_t EM_XIMO16 = 170;         /* New Japan Radio (NJR) 16-bit DSP */
        static constexpr std::uint16_t EM_MANIK = 171;          /* M2000 Reconfigurable RISC */
        static constexpr std::uint16_t EM_CRAYNV2 = 172;        /* Cray NV2 vector architecture */
        static constexpr std::uint16_t EM_RX = 173;             /* Renesas RX */
        static constexpr std::uint16_t EM_METAG = 174;          /* Imagination Tech. META */
        static constexpr std::uint16_t EM_MCST_ELBRUS = 175;    /* MCST Elbrus */
        static constexpr std::uint16_t EM_ECOG16 = 176;         /* Cyan Technology eCOG16 */
        static constexpr std::uint16_t EM_CR16 = 177;           /* National Semi. CompactRISC CR16 */
        static constexpr std::uint16_t EM_ETPU = 178;           /* Freescale Extended Time Processing Unit */
        static constexpr std::uint16_t EM_SLE9X = 179;          /* Infineon Tech. SLE9X */
        static constexpr std::uint16_t EM_L10M = 180;           /* Intel L10M */
        static constexpr std::uint16_t EM_K10M = 181;           /* Intel K10M */
        /* reserved 182 */
        static constexpr std::uint16_t EM_AARCH64 = 183;        /* ARM AARCH64 */
        /* reserved 184 */
        static constexpr std::uint16_t EM_AVR32 = 185;          /* Amtel 32-bit microprocessor */
        static constexpr std::uint16_t EM_STM8 = 186;           /* STMicroelectronics STM8 */
        static constexpr std::uint16_t EM_TILE64 = 187;         /* Tilera TILE64 */
        static constexpr std::uint16_t EM_TILEPRO = 188;        /* Tilera TILEPro */
        static constexpr std::uint16_t EM_MICROBLAZE = 189;     /* Xilinx MicroBlaze */
        static constexpr std::uint16_t EM_CUDA = 190;           /* NVIDIA CUDA */
        static constexpr std::uint16_t EM_TILEGX = 191;         /* Tilera TILE-Gx */
        static constexpr std::uint16_t EM_CLOUDSHIELD = 192;    /* CloudShield */
        static constexpr std::uint16_t EM_COREA_1ST = 193;      /* KIPO-KAIST Core-A 1st gen. */
        static constexpr std::uint16_t EM_COREA_2ND = 194;      /* KIPO-KAIST Core-A 2nd gen. */
        static constexpr std::uint16_t EM_ARCV2 = 195;          /* Synopsys ARCv2 ISA.  */
        static constexpr std::uint16_t EM_OPEN8 = 196;          /* Open8 RISC */
        static constexpr std::uint16_t EM_RL78 = 197;           /* Renesas RL78 */
        static constexpr std::uint16_t EM_VIDEOCORE5 = 198;     /* Broadcom VideoCore V */
        static constexpr std::uint16_t EM_78KOR = 199;          /* Renesas 78KOR */
        static constexpr std::uint16_t EM_56800EX = 200;        /* Freescale 56800EX DSC */
        static constexpr std::uint16_t EM_BA1 = 201;            /* Beyond BA1 */
        static constexpr std::uint16_t EM_BA2 = 202;            /* Beyond BA2 */
        static constexpr std::uint16_t EM_XCORE = 203;          /* XMOS xCORE */
        static constexpr std::uint16_t EM_MCHP_PIC = 204;       /* Microchip 8-bit PIC(r) */
        static constexpr std::uint16_t EM_INTELGT = 205;        /* Intel Graphics Technology */
        /* reserved 206-209 */
        static constexpr std::uint16_t EM_KM32 = 210;           /* KM211 KM32 */
        static constexpr std::uint16_t EM_KMX32 = 211;          /* KM211 KMX32 */
        static constexpr std::uint16_t EM_EMX16 = 212;          /* KM211 KMX16 */
        static constexpr std::uint16_t EM_EMX8 = 213;           /* KM211 KMX8 */
        static constexpr std::uint16_t EM_KVARC = 214;          /* KM211 KVARC */
        static constexpr std::uint16_t EM_CDP = 215;            /* Paneve CDP */
        static constexpr std::uint16_t EM_COGE = 216;           /* Cognitive Smart Memory Processor */
        static constexpr std::uint16_t EM_COOL = 217;           /* Bluechip CoolEngine */
        static constexpr std::uint16_t EM_NORC = 218;           /* Nanoradio Optimized RISC */
        static constexpr std::uint16_t EM_CSR_KALIMBA = 219;    /* CSR Kalimba */
        static constexpr std::uint16_t EM_Z80 = 220;            /* Zilog Z80 */
        static constexpr std::uint16_t EM_VISIUM = 221;         /* Controls and Data Services VISIUMcore */
        static constexpr std::uint16_t EM_FT32 = 222;           /* FTDI Chip FT32 */
        static constexpr std::uint16_t EM_MOXIE = 223;          /* Moxie processor */
        static constexpr std::uint16_t EM_AMDGPU = 224;         /* AMD GPU */
        /* reserved 225-242 */
        static constexpr std::uint16_t EM_RISCV = 243;          /* RISC-V */
        static constexpr std::uint16_t EM_BPF = 247;            /* Linux BPF -- in-kernel virtual machine */
        static constexpr std::uint16_t EM_CSKY = 252;           /* C-SKY */
        static constexpr std::uint16_t EM_LOONGARCH = 258;      /* LoongArch */
        static constexpr std::uint16_t EM_NUM = 259;
        static constexpr std::uint16_t EM_ARC_A5 = EM_ARC_COMPACT;
        static constexpr std::uint16_t EM_ALPHA = 0x9026;
        std::uint16_t e_machine;  /* Required architecture */

        static constexpr byte EV_NONE = 0;       /* Invalid version */
        static constexpr byte EV_CURRENT = 1;    /* Current version */
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
        static constexpr std::uint16_t SHN_UNDEF = 0;           /* Undefined, missing, irrelevant */
        static constexpr std::uint16_t SHN_LORESERVE = 0xff00;  /* First of reserved range */
        static constexpr std::uint16_t SHN_LOPROC = 0xff00;     /* First processor-specific */
        static constexpr std::uint16_t SHN_HIPROC = 0xff1f;     /* Last processor-specific */
        static constexpr std::uint16_t SHN_LOOS = 0xff20;       /* First operating system-specific */
        static constexpr std::uint16_t SHN_HIOS = 0xff3f;       /* Last operating system-specific */
        static constexpr std::uint16_t SHN_ABS = 0xfff1;        /* Absolute values */
        static constexpr std::uint16_t SHN_COMMON = 0xfff2;     /* Common data */
        static constexpr std::uint16_t SHN_XINDEX = 0xffff;     /* Escape -- index stored elsewhere */
        static constexpr std::uint16_t SHN_HIRESERVE = 0xffff;  /* Last of reserved range */

        std::uint32_t sh_name;       /* Section name (string table index) */

        static constexpr std::uint32_t SHT_NULL = 0;                     /* Inactive */
        static constexpr std::uint32_t SHT_PROGBITS = 1;                 /* Program-defined contents */
        static constexpr std::uint32_t SHT_SYMTAB = 2;                   /* Symbol table */
        static constexpr std::uint32_t SHT_STRTAB = 3;                   /* String table */
        static constexpr std::uint32_t SHT_RELA = 4;                     /* Relocation entries with addends */
        static constexpr std::uint32_t SHT_HASH = 5;                     /* Symbol hash table */
        static constexpr std::uint32_t SHT_DYNAMIC = 6;                  /* Dynamic linking information */
        static constexpr std::uint32_t SHT_NOTE = 7;                     /* Notes */
        static constexpr std::uint32_t SHT_NOBITS = 8;                   /* Program space with no data e.g. bss */
        static constexpr std::uint32_t SHT_REL = 9;                      /* Relocation entries without addends */
        static constexpr std::uint32_t SHT_SHLIB = 10;                   /* Reserved */
        static constexpr std::uint32_t SHT_DYNSYM = 11;                  /* Dynmic symbol table */
        static constexpr std::uint32_t SHT_INIT_ARRAY = 14;              /* Array of constructors */
        static constexpr std::uint32_t SHT_FINI_ARRAY = 15;              /* Array of destructors */
        static constexpr std::uint32_t SHT_PREINIT_ARRAY = 16;           /* Array of pre-constructors */
        static constexpr std::uint32_t SHT_GROUP = 17;                   /* Section group */
        static constexpr std::uint32_t SHT_SYMTAB_SHNDX = 18;            /* Extended section indices */
        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L447-L466
        static constexpr std::uint32_t SHT_RELR = 19;                    /* RELR relative relocations */
        static constexpr std::uint32_t SHT_NUM = 20;                     /* Number of defined types.  */
        static constexpr std::uint32_t SHT_LOOS = 0x60000000;            /* First of OS specific semantics */
        static constexpr std::uint32_t SHT_GNU_ATTRIBUTES = 0x6ffffff5;  /* Object attributes.  */
        static constexpr std::uint32_t SHT_GNU_HASH = 0x6ffffff6;        /* GNU-style hash table.  */
        static constexpr std::uint32_t SHT_GNU_LIBLIST = 0x6ffffff7;     /* Prelink library list */
        static constexpr std::uint32_t SHT_CHECKSUM = 0x6ffffff8;        /* Checksum for DSO content.  */
        static constexpr std::uint32_t SHT_LOSUNW = 0x6ffffffa;          /* Sun-specific low bound.  */
        static constexpr std::uint32_t SHT_SUNW_move = 0x6ffffffa;
        static constexpr std::uint32_t SHT_SUNW_COMDAT = 0x6ffffffb;
        static constexpr std::uint32_t SHT_SUNW_syminfo = 0x6ffffffc;
        static constexpr std::uint32_t SHT_GNU_verdef = 0x6ffffffd;      /* Version definition section.  */
        static constexpr std::uint32_t SHT_GNU_verneed = 0x6ffffffe;     /* Version needs section.  */
        static constexpr std::uint32_t SHT_GNU_versym = 0x6fffffff;      /* Version symbol table.  */
        static constexpr std::uint32_t SHT_HISUNW = 0x6fffffff;          /* Sun-specific high bound.  */
        static constexpr std::uint32_t SHT_HIOS = 0x6fffffff;            /* Last of OS specific semantics */
        static constexpr std::uint32_t SHT_LOPROC = 0x70000000;          /* First of processor-specific type */
        static constexpr std::uint32_t SHT_HIPROC = 0x7fffffff;          /* Last of processor-specific type */
        static constexpr std::uint32_t SHT_LOUSER = 0x80000000;          /* First of reserved range */
        static constexpr std::uint32_t SHT_HIUSER = 0xffffffff;          /* Last of reserved range */
        std::uint32_t sh_type;       /* Section type */

        static constexpr std::uint64_t SHF_WRITE = 0x1;               /* Writable */
        static constexpr std::uint64_t SHF_ALLOC = 0x2;               /* Occupies memory during execution */
        static constexpr std::uint64_t SHF_EXECINSTR = 0x4;           /* Executable */
        static constexpr std::uint64_t SHF_MERGE = 0x10;              /* Might be merged */
        static constexpr std::uint64_t SHF_STRINGS = 0x20;            /* Contains nul-terminated strings */
        static constexpr std::uint64_t SHF_INFO_LINK = 0x40;          /* `sh_info` contains SHT index */
        static constexpr std::uint64_t SHF_LINK_ORDER = 0x80;         /* Preserve order after combining */
        static constexpr std::uint64_t SHF_OS_NONCONFORMING = 0x100;  /* Non-standard OS specific handling required */
        static constexpr std::uint64_t SHF_GROUP = 0x200;             /* Section is member of a group */
        static constexpr std::uint64_t SHF_TLS = 0x400;               /* Section holds thread-local data */
        static constexpr std::uint64_t SHF_MASKOS = 0x0ff00000;       /* All bits included are for OS-specific flags */
        static constexpr std::uint64_t SHF_MASKPROC = 0xf0000000;     /* All bits included are for processor-specific flags */
        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L484-L488
        static constexpr std::uint64_t SHF_GNU_RETAIN = (1 << 21);    /* Not to be GCed by linker.  */
        static constexpr std::uint64_t SHF_ORDERED = (1 << 30);       /* Special ordering requirement (Solaris).  */
        static constexpr std::uint64_t SHF_EXCLUDE = (1U << 31);      /* Section is excluded unless referenced or allocated (Solaris).*/
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

    static constexpr std::uint32_t GRP_COMDAT = 1;             /* Mark group as COMDAT */
    static constexpr std::uint32_t GRP_MASKOS = 0x0ff00000;    /* All bits included are for OS-specific flags */
    static constexpr std::uint32_t GRP_MASKPROC = 0xf0000000;  /* All bits included are for processor-specific flags */

    // Same memory layout as Elf64_Phdr for optimization
    typedef struct elf_program_header
    {
        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L717-L738
        static constexpr std::uint32_t PT_NULL = 0;                   /* Program header table entry unused */
        static constexpr std::uint32_t PT_LOAD = 1;                   /* Loadable program segment */
        static constexpr std::uint32_t PT_DYNAMIC = 2;                /* Dynamic linking information */
        static constexpr std::uint32_t PT_INTERP = 3;                 /* Program interpreter */
        static constexpr std::uint32_t PT_NOTE = 4;                   /* Auxiliary information */
        static constexpr std::uint32_t PT_SHLIB = 5;                  /* Reserved */
        static constexpr std::uint32_t PT_PHDR = 6;                   /* Entry for header table itself */
        static constexpr std::uint32_t PT_TLS = 7;                    /* Thread-local storage segment */
        static constexpr std::uint32_t PT_NUM = 8;                    /* Number of defined types */
        static constexpr std::uint32_t PT_LOOS = 0x60000000;          /* Start of OS-specific */
        static constexpr std::uint32_t PT_GNU_EH_FRAME = 0x6474e550;  /* GCC .eh_frame_hdr segment */
        static constexpr std::uint32_t PT_GNU_STACK = 0x6474e551;     /* Indicates stack executability */
        static constexpr std::uint32_t PT_GNU_RELRO = 0x6474e552;     /* Read-only after relocation */
        static constexpr std::uint32_t PT_GNU_PROPERTY = 0x6474e553;  /* GNU property */
        static constexpr std::uint32_t PT_GNU_SFRAME = 0x6474e554;    /* SFrame segment.  */
        static constexpr std::uint32_t PT_LOSUNW = 0x6ffffffa;
        static constexpr std::uint32_t PT_SUNWBSS = 0x6ffffffa;       /* Sun Specific segment */
        static constexpr std::uint32_t PT_SUNWSTACK = 0x6ffffffb;     /* Stack segment */
        static constexpr std::uint32_t PT_HISUNW = 0x6fffffff;
        static constexpr std::uint32_t PT_HIOS = 0x6fffffff;          /* End of OS-specific */
        static constexpr std::uint32_t PT_LOPROC = 0x70000000;        /* Start of processor-specific */
        static constexpr std::uint32_t PT_HIPROC = 0x7fffffff;        /* End of processor-specific */
        std::uint32_t p_type;    /* Segment type */

        static constexpr std::uint32_t PF_X = 0x1;                /* Execute */
        static constexpr std::uint32_t PF_W = 0x2;                /* Write */
        static constexpr std::uint32_t PF_R = 0x4;                /* Read */
        static constexpr std::uint32_t PF_MASKOS = 0x0ff00000;    /* All bits included are for OS-specific flags */
        static constexpr std::uint32_t PF_MASKPROC = 0xf0000000;  /* All bits included are for processor-specific flags */
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
    } elf_symbol;

    typedef struct elf_dynamic
    {
        static constexpr std::int64_t DT_NULL = 0;              /* Marks end of dynamic section */
        static constexpr std::int64_t DT_NEEDED = 1;            /* Name of needed library */
        static constexpr std::int64_t DT_PLTRELSZ = 2;          /* Size in bytes of all PLT relocations */
        static constexpr std::int64_t DT_PLTGOT = 3;            /* Processor defined value relating to PLT/GOT */
        static constexpr std::int64_t DT_HASH = 4;              /* Address of the symbol hash table */
        static constexpr std::int64_t DT_STRTAB = 5;            /* Address of the dynamic string table */
        static constexpr std::int64_t DT_SYMTAB = 6;            /* Address of the dynamic symbol table */
        static constexpr std::int64_t DT_RELA = 7;              /* Address of a relocation table with Elf*_Rela entries */
        static constexpr std::int64_t DT_RELASZ = 8;            /* Total size in bytes of the DT_RELA relocation table */
        static constexpr std::int64_t DT_RELAENT = 9;           /* Size in bytes of each DT_RELA relocation entry */
        static constexpr std::int64_t DT_STRSZ = 10;            /* Size in bytes of the string table */
        static constexpr std::int64_t DT_SYMENT = 11;           /* Size in bytes of each symbol table entry */
        static constexpr std::int64_t DT_INIT = 12;             /* Address of the initialization function */
        static constexpr std::int64_t DT_FINI = 13;             /* Address of the termination function */
        static constexpr std::int64_t DT_SONAME = 14;           /* Shared object name (string table index) */
        static constexpr std::int64_t DT_RPATH = 15;            /* Library search path (string table index) */
        static constexpr std::int64_t DT_SYMBOLIC = 16;         /* Indicates "symbolic" linking */
        static constexpr std::int64_t DT_REL = 17;              /* Address of a relocation table with Elf*_Rel entries */
        static constexpr std::int64_t DT_RELSZ = 18;            /* Total size in bytes of the DT_REL relocation table */
        static constexpr std::int64_t DT_RELENT = 19;           /* Size in bytes of each DT_REL relocation entry */
        static constexpr std::int64_t DT_PLTREL = 20;           /* Type of relocation used for PLT */
        static constexpr std::int64_t DT_DEBUG = 21;            /* Reserved for debugger */
        static constexpr std::int64_t DT_TEXTREL = 22;          /* Object contains text relocations (non-writable segment) */
        static constexpr std::int64_t DT_JMPREL = 23;           /* Address of the relocations associated with the PLT */
        static constexpr std::int64_t DT_BIND_NOW = 24;         /* Process all relocations before execution */
        static constexpr std::int64_t DT_INIT_ARRAY = 25;       /* Array of initialization functions */
        static constexpr std::int64_t DT_FINI_ARRAY = 26;       /* Array of termination functions */
        static constexpr std::int64_t DT_INIT_ARRAYSZ = 27;     /* Size of arrays in DT_INIT_ARRAY */
        static constexpr std::int64_t DT_FINI_ARRAYSZ = 28;     /* Size of arrays in DT_FINI_ARRAY */
        static constexpr std::int64_t DT_RUNPATH = 29;          /* Library search paths */
        static constexpr std::int64_t DT_FLAGS = 30;            /* Flags for the object being loaded */
        static constexpr std::int64_t DT_ENCODING = 32;         /* Values from here to DT_LOOS if even use d_ptr or odd uses d_val */
        static constexpr std::int64_t DT_PREINIT_ARRAY = 32;    /* Array of pre-initialization functions */
        static constexpr std::int64_t DT_PREINIT_ARRAYSZ = 33;  /* Size of array of pre-init functions */

        // https://github.com/bminor/glibc/blob/42c960a4f1052a71d928a1c554f5d445b00e61f7/elf/elf.h#L908-L912
        static constexpr std::int64_t DT_SYMTAB_SHNDX = 34;     /* Address of SYMTAB_SHNDX section */
        static constexpr std::int64_t DT_RELRSZ = 35;           /* Total size of RELR relative relocations */
        static constexpr std::int64_t DT_RELR = 36;             /* Address of RELR relative relocations */
        static constexpr std::int64_t DT_RELRENT = 37;          /* Size of one RELR relative relocaction */
        static constexpr std::int64_t DT_NUM = 38;              /* Number used */

        static constexpr std::int64_t DT_LOOS = 0x6000000D;     /* Start of OS-specific */
        static constexpr std::int64_t DT_HIOS = 0x6ffff000;     /* End of OS-specific */
        static constexpr std::int64_t DT_LOPROC = 0x70000000;   /* Start of processor-specific */
        static constexpr std::int64_t DT_HIPROC = 0x7fffffff;   /* End of processor-specific */
        std::int64_t d_tag;       /* Entry type */
        union
        {
            std::uint64_t d_val;  /* Integer value */
            std::uint64_t d_ptr;  /* Address value */
        } d_un;

        // DT_FLAGS values
        static constexpr std::uint64_t DF_ORIGIN = 0x1;       /* Object may use $ORIGIN */
        static constexpr std::uint64_t DF_SYMBOLIC = 0x2;     /* Symbol resolutions starts from this object */
        static constexpr std::uint64_t DF_TEXTREL = 0x4;      /* Object contains text relocations (non-writable segment) */
        static constexpr std::uint64_t DF_BIND_NOW = 0x8;     /* No lazy binding for this object */
        static constexpr std::uint64_t DF_STATIC_TLS = 0x10;  /* Module uses the static TLS model */
    } elf_dynamic;

    // Non-architecture agnostic types
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

        typedef struct
        {
            Elf32_Word st_name;
            Elf32_Addr st_value;
            Elf32_Word st_size;
            unsigned char st_info;
            unsigned char st_other;
            Elf32_Half st_shndx;
        } Elf32_Sym;

        typedef struct
        {
            Elf64_Word st_name;
            unsigned char st_info;
            unsigned char st_other;
            Elf64_Half st_shndx;
            Elf64_Addr st_value;
            Elf64_Xword st_size;
        } Elf64_Sym;

        typedef struct
        {
            Elf32_Sword d_tag;
            union
            {
                Elf32_Word d_val;
                Elf32_Addr d_ptr;
            } d_un;
        } Elf32_Dyn;

        typedef struct
        {
            Elf64_Sxword d_tag;
            union
            {
                Elf64_Xword d_val;
                Elf64_Addr d_ptr;
            } d_un;
        } Elf64_Dyn;
    }

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
                  !this->read_section_headers()
                  )
          {
            return;
          }
        };

        bool error() const
        {
          return !this->last_error.empty();
        }

        const std::string &error_message() const
        {
          return this->last_error;
        }

        void clear_error()
        {
          this->last_error = "";
        }

        const ::elf::elf_header &get_header() const
        {
          return this->header;
        }

        const std::vector<::elf::elf_program_header> &get_program_headers() const
        {
          return this->program_headers;
        }

        const std::vector<::elf::elf_section_header> &get_section_headers() const
        {
          return this->section_headers;
        }

        bool is_little_endian() const
        {
          return this->header.e_ident.ei_data == ::elf::elf_ident::ELFDATA2LSB;
        }

        bool is_big_endian() const
        {
          return this->header.e_ident.ei_data == ::elf::elf_ident::ELFDATA2MSB;
        }

        bool is_32_bit() const
        {
          return this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS32;
        }

        bool is_64_bit() const
        {
          return this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS64;
        }

        std::ifstream &get_binary_file()
        {
          return this->binary_file;
        }

        bool parse_dynamic_segment()
        {
          const auto dynamic_header = std::find_if(this->program_headers.begin(), this->program_headers.end(),
                                                   [](const elf::elf_program_header &program_header) {
                                                       return program_header.p_type == ::elf::elf_program_header::PT_DYNAMIC;
                                                   });
          if (dynamic_header == this->program_headers.end())
          {
            return false;
          }

          if (!this->binary_file.is_open())
          {
            this->last_error = "Binary file is not open";
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
                dynamic_string_table_offset = dynamic_entry.d_un.d_ptr;
                break;
              }
              case ::elf::elf_dynamic::DT_STRSZ:
              {
                dynamic_string_table_length = dynamic_entry.d_un.d_val;
                break;
              }
              case ::elf::elf_dynamic::DT_SYMTAB:
              {
                symbol_table_offset = dynamic_entry.d_un.d_ptr;
                break;
              }
              case ::elf::elf_dynamic::DT_SYMENT:
              {
                symbol_table_entry_size = dynamic_entry.d_un.d_val;
                break;
              }
              case ::elf::elf_dynamic::DT_SONAME:
              {
                this->so_name = reinterpret_cast<const char *>(reinterpret_cast<std::uintptr_t>(dynamic_entry.d_un.d_val));
                break;
              }
              case ::elf::elf_dynamic::DT_NEEDED:
              {
                this->needed_libraries.emplace_back(reinterpret_cast<const char *>(reinterpret_cast<std::uintptr_t>(dynamic_entry.d_un.d_val)));
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

          this->dynamic_segment_string_table.resize(dynamic_string_table_length);
          this->binary_file.seekg(static_cast<std::streamoff>(dynamic_string_table_offset));
          this->binary_file.read(this->dynamic_segment_string_table.data(), static_cast<std::streamsize>(dynamic_string_table_length));
          if (this->binary_file.gcount() != dynamic_string_table_length)
          {
            this->last_error = "Failed to read dynamic string table";
            return false;
          }
          const auto dynamic_string_table_base = reinterpret_cast<std::uintptr_t>(this->dynamic_segment_string_table.data());

          this->so_name = dynamic_string_table_base + this->so_name;
          for(auto &needed : this->needed_libraries)
          {
            needed = dynamic_string_table_base + needed;
          }

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

          return true;
        }

        const std::vector<::elf::elf_dynamic> &get_dynamic_entries() const
        {
          return this->dynamic_entries;
        }

        const std::vector<char> &get_dynamic_string_table() const
        {
          return this->dynamic_segment_string_table;
        }

        const char *get_so_name() const
        {
          return this->so_name;
        }

        const std::vector<const char *> &get_needed_libraries() const
        {
          return this->needed_libraries;
        }

        const std::vector<elf::elf_symbol> &get_dynamic_symbols() const
        {
          return this->dynamic_symbols;
        }

    private:
        const std::filesystem::path path;
        std::ifstream binary_file;
        std::string last_error;

        ::elf::elf_header header;
        std::vector<::elf::elf_program_header> program_headers;
        std::vector<::elf::elf_section_header> section_headers;
        std::vector<char> section_header_string_table;
        std::vector<::elf::elf_dynamic> dynamic_entries;
        std::vector<char> dynamic_segment_string_table;
        const char *so_name = nullptr;
        std::vector<const char *> needed_libraries;
        std::vector<elf::elf_symbol> dynamic_symbols;

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

          if (this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS64)
          {
            this->binary_file.seekg(0);
            this->binary_file.read(reinterpret_cast<char *>(&this->header), sizeof(this->header));
            if (this->binary_file.gcount() != sizeof(this->header))
            {
              this->last_error = "Failed to read ELF header";
              return false;
            }
          } else if (this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS32)
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

          if (this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS64)
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

          if (this->header.e_ident.ei_class == ::elf::elf_ident::ELFCLASS64)
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
    };

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

    // https://blogs.oracle.com/solaris/post/gnu-hash-elf-sections
    // https://sourceware.org/legacy-ml/binutils/2006-10/msg00377.html
    inline uint_fast32_t gnu_hash(const char *s)
    {
      uint_fast32_t h = 5381;

      for (unsigned char c = *s; c != '\0'; c = *++s)
        h = h * 33 + c;

      return h & 0xffffffff;
    }
}
