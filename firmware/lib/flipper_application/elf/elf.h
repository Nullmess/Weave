#ifndef _ELF_H
#define _ELF_H 1

#include <stdint.h>

typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;

typedef uint64_t Elf32_Xword;
typedef int64_t Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

/* Indices de section sur 16 bits */
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;

#define EI_NIDENT (16)

typedef struct {
    unsigned char e_ident[EI_NIDENT]; /* Magic et informations d’identification */
    Elf32_Half e_type; /* Type objet */
    Elf32_Half e_machine; /* Architecture */
    Elf32_Word e_version; /* Version objet */
    Elf32_Addr e_entry; /* Adresse virtuelle d’entrée */
    Elf32_Off e_phoff; /* Offset table program headers dans fichier */
    Elf32_Off e_shoff; /* Offset table section headers dans fichier */
    Elf32_Word e_flags; /* Flags propres au processeur */
    Elf32_Half e_ehsize; /* Taille ELF header en bytes */
    Elf32_Half e_phentsize; /* Taille entrée program header */
    Elf32_Half e_phnum; /* Nombre entrées program header */
    Elf32_Half e_shentsize; /* Taille entrée section header */
    Elf32_Half e_shnum; /* Nombre entrées section header */
    Elf32_Half e_shstrndx; /* Index table string des sections */
} Elf32_Ehdr;

typedef struct {
    unsigned char e_ident[EI_NIDENT]; /* Magic et informations d’identification */
    Elf64_Half e_type; /* Type objet */
    Elf64_Half e_machine; /* Architecture */
    Elf64_Word e_version; /* Version objet */
    Elf64_Addr e_entry; /* Adresse virtuelle d’entrée */
    Elf64_Off e_phoff; /* Offset table program headers dans fichier */
    Elf64_Off e_shoff; /* Offset table section headers dans fichier */
    Elf64_Word e_flags; /* Flags propres au processeur */
    Elf64_Half e_ehsize; /* Taille ELF header en bytes */
    Elf64_Half e_phentsize; /* Taille entrée program header */
    Elf64_Half e_phnum; /* Nombre entrées program header */
    Elf64_Half e_shentsize; /* Taille entrée section header */
    Elf64_Half e_shnum; /* Nombre entrées section header */
    Elf64_Half e_shstrndx; /* Index table string des sections */
} Elf64_Ehdr;

/* EI_* : index dans e_ident ; sous-macros : valeurs possibles */

#define EI_MAG0 0
#define ELFMAG0 0x7f

#define EI_MAG1 1
#define ELFMAG1 'E'

#define EI_MAG2 2
#define ELFMAG2 'L'

#define EI_MAG3 3
#define ELFMAG3 'F'

/* Magic complet pour comparaison comme word */
#define ELFMAG "\177ELF"
#define SELFMAG 4

#define EI_CLASS 4
#define ELFCLASSNONE 0 /* Classe invalide */
#define ELFCLASS32 1 /* Objets 32 bits */
#define ELFCLASS64 2 /* Objets 64 bits */
#define ELFCLASSNUM 3

#define EI_DATA 5
#define ELFDATANONE 0 /* Encodage données invalide */
#define ELFDATA2LSB 1 /* Complément à deux, little endian */
#define ELFDATA2MSB 2 /* Complément à deux, big endian */
#define ELFDATANUM 3

#define EI_VERSION 6
/* Valeur obligatoirement EV_CURRENT */

#define EI_OSABI 7
#define ELFOSABI_NONE 0 /* UNIX System V ABI */
#define ELFOSABI_SYSV 0 /* Alias. */
#define ELFOSABI_HPUX 1 /* HP-UX */
#define ELFOSABI_NETBSD 2 /* NetBSD. */
#define ELFOSABI_LINUX 3 /* Linux. */
#define ELFOSABI_SOLARIS 6 /* Sun Solaris. */
#define ELFOSABI_AIX 7 /* IBM AIX. */
#define ELFOSABI_IRIX 8 /* SGI Irix. */
#define ELFOSABI_FREEBSD 9 /* FreeBSD. */
#define ELFOSABI_TRU64 10 /* Compaq TRU64 UNIX. */
#define ELFOSABI_MODESTO 11 /* Novell Modesto. */
#define ELFOSABI_OPENBSD 12 /* OpenBSD. */
#define ELFOSABI_ARM 97 /* ARM */
#define ELFOSABI_STANDALONE 255 /* Application embarquée standalone */

#define EI_ABIVERSION 8 /* Version ABI */

#define EI_PAD 9 /* Index premier byte de padding */

#define ET_NONE 0 /* Aucun type fichier */
#define ET_REL 1 /* Fichier relogeable */
#define ET_EXEC 2 /* Fichier exécutable */
#define ET_DYN 3 /* Objet partagé */
#define ET_CORE 4 /* Fichier core */
#define ET_NUM 5 /* Nombre types définis */
#define ET_LOOS 0xfe00 /* Début plage propre à l’OS */
#define ET_HIOS 0xfeff /* Fin plage propre à l’OS */
#define ET_LOPROC 0xff00 /* Début plage propre au processeur */
#define ET_HIPROC 0xffff /* Fin plage propre au processeur */

#define EM_NONE 0 /* Aucune architecture */
#define EM_M32 1 /* AT&T WE 32100 */
#define EM_SPARC 2 /* SUN SPARC */
#define EM_386 3 /* Intel 80386 */
#define EM_68K 4 /* Motorola m68k */
#define EM_88K 5 /* Motorola m88k */
#define EM_860 7 /* Intel 80860 */
#define EM_MIPS 8 /* MIPS R3000 big-endian */
#define EM_S370 9 /* IBM System/370 */
#define EM_MIPS_RS3_LE 10 /* MIPS R3000 little-endian */

#define EM_PARISC 15 /* HPPA */
#define EM_VPP500 17 /* Fujitsu VPP500 */
#define EM_SPARC32PLUS 18 /* Sun's "v8plus" */
#define EM_960 19 /* Intel 80960 */
#define EM_PPC 20 /* PowerPC */
#define EM_PPC64 21 /* PowerPC 64-bit */
#define EM_S390 22 /* IBM S390 */

#define EM_V800 36 /* NEC V800 */
#define EM_FR20 37 /* Fujitsu FR20 */
#define EM_RH32 38 /* TRW RH-32 */
#define EM_RCE 39 /* Motorola RCE */
#define EM_ARM 40 /* ARM */
#define EM_FAKE_ALPHA 41 /* Digital Alpha */
#define EM_SH 42 /* Hitachi SH */
#define EM_SPARCV9 43 /* SPARC v9 64-bit */
#define EM_TRICORE 44 /* Siemens Tricore */
#define EM_ARC 45 /* Argonaut RISC Core */
#define EM_H8_300 46 /* Hitachi H8/300 */
#define EM_H8_300H 47 /* Hitachi H8/300H */
#define EM_H8S 48 /* Hitachi H8S */
#define EM_H8_500 49 /* Hitachi H8/500 */
#define EM_IA_64 50 /* Intel Merced */
#define EM_MIPS_X 51 /* Stanford MIPS-X */
#define EM_COLDFIRE 52 /* Motorola Coldfire */
#define EM_68HC12 53 /* Motorola M68HC12 */
#define EM_MMA 54 /* Fujitsu MMA Multimedia Accelerator */
#define EM_PCP 55 /* Siemens PCP */
#define EM_NCPU 56 /* Sony nCPU RISC embarqué */
#define EM_NDR1 57 /* Denso NDR1 microprocesseur */
#define EM_STARCORE 58 /* Motorola Start*Core processeur */
#define EM_ME16 59 /* Toyota ME16 processeur */
#define EM_ST100 60 /* STMicroelectronic ST100 processeur */
#define EM_TINYJ 61 /* Advanced Logic Corp. Tinyj emb.fam */
#define EM_X86_64 62 /* AMD x86-64 */
#define EM_PDSP 63 /* Sony DSP processeur */

#define EM_FX66 66 /* Siemens FX66 microcontrôleur */
#define EM_ST9PLUS 67 /* STMicroelectronics ST9+ 8/16 mc */
#define EM_ST7 68 /* STmicroelectronics ST7 8 bit mc */
#define EM_68HC16 69 /* Motorola MC68HC16 microcontrôleur */
#define EM_68HC11 70 /* Motorola MC68HC11 microcontrôleur */
#define EM_68HC08 71 /* Motorola MC68HC08 microcontrôleur */
#define EM_68HC05 72 /* Motorola MC68HC05 microcontrôleur */
#define EM_SVX 73 /* Silicon Graphics SVx */
#define EM_ST19 74 /* STMicroelectronics ST19 8 bit mc */
#define EM_VAX 75 /* Digital VAX */
#define EM_CRIS 76 /* Axis Communications 32-bit processeur embarqué */
#define EM_JAVELIN 77 /* Infineon Technologies 32-bit processeur embarqué */
#define EM_FIREPATH 78 /* Element 14 64-bit DSP processeur */
#define EM_ZSP 79 /* LSI Logic 16-bit DSP processeur */
#define EM_MMIX 80 /* Donald Knuth's éducatif 64-bit processeur */
#define EM_HUANY 81 /* Harvard University objets indépendants de la machine */
#define EM_PRISM 82 /* SiTera Prism */
#define EM_AVR 83 /* Atmel AVR 8-bit microcontrôleur */
#define EM_FR30 84 /* Fujitsu FR30 */
#define EM_D10V 85 /* Mitsubishi D10V */
#define EM_D30V 86 /* Mitsubishi D30V */
#define EM_V850 87 /* NEC v850 */
#define EM_M32R 88 /* Mitsubishi M32R */
#define EM_MN10300 89 /* Matsushita MN10300 */
#define EM_MN10200 90 /* Matsushita MN10200 */
#define EM_PJ 91 /* picoJava */
#define EM_OPENRISC 92 /* OpenRISC 32-bit processeur embarqué */
#define EM_ARC_A5 93 /* ARC Cores Tangent-A5 */
#define EM_XTENSA 94 /* Tensilica Xtensa */
#define EM_NUM 95

#define EV_NONE 0 /* Version ELF invalide */
#define EV_CURRENT 1 /* Version courante */
#define EV_NUM 2

typedef struct {
    Elf32_Word sh_name; /* Index nom section dans table string */
    Elf32_Word sh_type; /* Type section */
    Elf32_Word sh_flags; /* Flags section */
    Elf32_Addr sh_addr; /* Adresse virtuelle section pendant exécution */
    Elf32_Off sh_offset; /* Offset section dans fichier */
    Elf32_Word sh_size; /* Taille section en bytes */
    Elf32_Word sh_link; /* Lien vers une autre section */
    Elf32_Word sh_info; /* Informations complémentaires section */
    Elf32_Word sh_addralign; /* Alignement section */
    Elf32_Word sh_entsize; /* Taille entrée si section contenant une table */
} Elf32_Shdr;

typedef struct {
    Elf64_Word sh_name; /* Index nom section dans table string */
    Elf64_Word sh_type; /* Type section */
    Elf64_Xword sh_flags; /* Flags section */
    Elf64_Addr sh_addr; /* Adresse virtuelle section pendant exécution */
    Elf64_Off sh_offset; /* Offset section dans fichier */
    Elf64_Xword sh_size; /* Taille section en bytes */
    Elf64_Word sh_link; /* Lien vers une autre section */
    Elf64_Word sh_info; /* Informations complémentaires section */
    Elf64_Xword sh_addralign; /* Alignement section */
    Elf64_Xword sh_entsize; /* Taille entrée si section contenant une table */
} Elf64_Shdr;

#define SHN_UNDEF 0 /* Section indéfinie */
#define SHN_LORESERVE 0xff00 /* Début indices réservés */
#define SHN_LOPROC 0xff00 /* Début spécifique au processeur */
#define SHN_BEFORE \
    0xff00 /* Section avant toutes les autres, Solaris */
#define SHN_AFTER \
    0xff01 /* Section après toutes les autres, Solaris */
#define SHN_HIPROC 0xff1f /* Fin spécifique au processeur */
#define SHN_LOOS 0xff20 /* Début plage propre à l’OS */
#define SHN_HIOS 0xff3f /* Fin plage propre à l’OS */
#define SHN_ABS 0xfff1 /* Symbole associé absolu */
#define SHN_COMMON 0xfff2 /* Symbole associé common */
#define SHN_XINDEX 0xffff /* Index dans table supplémentaire */
#define SHN_HIRESERVE 0xffff /* Fin indices réservés */

#define SHT_NULL 0 /* Entrée section header inutilisée */
#define SHT_PROGBITS 1 /* Données programme */
#define SHT_SYMTAB 2 /* Table symboles */
#define SHT_STRTAB 3 /* Table string */
#define SHT_RELA 4 /* Relocations avec addends */
#define SHT_HASH 5 /* Table hash des symboles */
#define SHT_DYNAMIC 6 /* Informations de dynamic linking */
#define SHT_NOTE 7 /* Notes */
#define SHT_NOBITS 8 /* Espace programme sans données, BSS */
#define SHT_REL 9 /* Relocations sans addends */
#define SHT_SHLIB 10 /* Réservé */
#define SHT_DYNSYM 11 /* Table symboles du dynamic linker */
#define SHT_INIT_ARRAY 14 /* Table constructors */
#define SHT_FINI_ARRAY 15 /* Table destructors */
#define SHT_PREINIT_ARRAY 16 /* Table pre-constructors */
#define SHT_GROUP 17 /* Groupe sections */
#define SHT_SYMTAB_SHNDX 18 /* Indices sections étendus */
#define SHT_NUM 19 /* Nombre types définis */
#define SHT_LOOS 0x60000000 /* Début plage propre à l’OS */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5 /* Attributs objet */
#define SHT_GNU_HASH 0x6ffffff6 /* Table hash GNU */
#define SHT_GNU_LIBLIST 0x6ffffff7 /* Liste libraries prelink */
#define SHT_CHECKSUM 0x6ffffff8 /* Checksum contenu DSO */
#define SHT_LOSUNW 0x6ffffffa /* Borne basse propre à Sun */
#define SHT_SUNW_move 0x6ffffffa
#define SHT_SUNW_COMDAT 0x6ffffffb
#define SHT_SUNW_syminfo 0x6ffffffc
#define SHT_GNU_verdef 0x6ffffffd /* Section définitions de versions */
#define SHT_GNU_verneed 0x6ffffffe /* Section versions requises */
#define SHT_GNU_versym 0x6fffffff /* Table symboles de versions */
#define SHT_HISUNW 0x6fffffff /* Borne haute propre à Sun */
#define SHT_HIOS 0x6fffffff /* Fin plage types propres à l’OS */
#define SHT_LOPROC 0x70000000 /* Début spécifique au processeur */
#define SHT_HIPROC 0x7fffffff /* Fin spécifique au processeur */
#define SHT_LOUSER 0x80000000 /* Début plage propre à l’application */
#define SHT_HIUSER 0x8fffffff /* Fin plage propre à l’application */

#define SHF_WRITE (1 << 0) /* Écriture permise */
#define SHF_ALLOC (1 << 1) /* Mémoire occupée pendant exécution */
#define SHF_EXECINSTR (1 << 2) /* Exécutable */
#define SHF_MERGE (1 << 4) /* Fusion possible */
#define SHF_STRINGS (1 << 5) /* Strings terminées par NUL */
#define SHF_INFO_LINK (1 << 6) /* Sh_info contient un index SHT */
#define SHF_LINK_ORDER (1 << 7) /* Ordre conservé après combinaison */
#define SHF_OS_NONCONFORMING \
    (1 << 8) /* Traitement spécifique non standard par l’OS */
#define SHF_GROUP (1 << 9) /* Section membre d’un groupe */
#define SHF_TLS (1 << 10) /* Données thread-local */
#define SHF_MASKOS 0x0ff00000 /* Propre à l’OS */
#define SHF_MASKPROC 0xf0000000 /* Propre au processeur */
#define SHF_ORDERED \
    (1 << 30) /* Ordre spécifique requis, Solaris */
#define SHF_EXCLUDE \
    (1 << 31) /* Section exclue sauf si référencée/allouée, Solaris */

#define GRP_COMDAT 0x1 /* Groupe marqué COMDAT */

typedef struct {
    Elf32_Word st_name; /* Index nom symbole dans table string */
    Elf32_Addr st_value; /* Valeur symbole */
    Elf32_Word st_size; /* Taille symbole */
    unsigned char st_info; /* Type et binding symbole */
    unsigned char st_other; /* Visibilité symbole */
    Elf32_Section st_shndx; /* Index section */
} Elf32_Sym;

typedef struct {
    Elf64_Word st_name; /* Index nom symbole dans table string */
    unsigned char st_info; /* Type et binding symbole */
    unsigned char st_other; /* Visibilité symbole */
    Elf64_Section st_shndx; /* Index section */
    Elf64_Addr st_value; /* Valeur symbole */
    Elf64_Xword st_size; /* Taille symbole */
} Elf64_Sym;

/* Syminfo optionnel : informations complémentaires par symbole dynamique */

typedef struct {
    Elf32_Half si_boundto; /* Direct binding : symbole lié à */
    Elf32_Half si_flags; /* Flags par symbole */
} Elf32_Syminfo;

typedef struct {
    Elf64_Half si_boundto; /* Direct binding : symbole lié à */
    Elf64_Half si_flags; /* Flags par symbole */
} Elf64_Syminfo;

#define SYMINFO_BT_SELF 0xffff /* Symbole lié à lui-même */
#define SYMINFO_BT_PARENT 0xfffe /* Symbole lié au parent */
#define SYMINFO_BT_LOWRESERVE 0xff00 /* Début entrées réservées */

#define SYMINFO_FLG_DIRECT 0x0001 /* Symbole directement lié */
#define SYMINFO_FLG_PASSTHRU 0x0002 /* Symbole passthrough pour translator */
#define SYMINFO_FLG_COPY 0x0004 /* Symbole copy-reloc */
#define SYMINFO_FLG_LAZYLOAD \
    0x0008 /* Symbole lié à objet chargé paresseusement */

#define SYMINFO_NONE 0
#define SYMINFO_CURRENT 1
#define SYMINFO_NUM 2

#define ELF32_ST_BIND(val) (((unsigned char)(val)) >> 4)
#define ELF32_ST_TYPE(val) ((val)&0xf)
#define ELF32_ST_INFO(bind, type) (((bind) << 4) + ((type)&0xf))

/* Elf32_Sym et Elf64_Sym : st_info sur un byte */
#define ELF64_ST_BIND(val) ELF32_ST_BIND(val)
#define ELF64_ST_TYPE(val) ELF32_ST_TYPE(val)
#define ELF64_ST_INFO(bind, type) ELF32_ST_INFO((bind), (type))

#define STB_LOCAL 0 /* Symbole local */
#define STB_GLOBAL 1 /* Symbole global */
#define STB_WEAK 2 /* Symbole weak */
#define STB_NUM 3 /* Nombre types définis */
#define STB_LOOS 10 /* Début plage propre à l’OS */
#define STB_GNU_UNIQUE 10 /* Symbole unique */
#define STB_HIOS 12 /* Fin plage propre à l’OS */
#define STB_LOPROC 13 /* Début spécifique au processeur */
#define STB_HIPROC 15 /* Fin spécifique au processeur */

#define STT_NOTYPE 0 /* Type symbole non spécifié */
#define STT_OBJECT 1 /* Symbole objet données */
#define STT_FUNC 2 /* Symbole objet code */
#define STT_SECTION 3 /* Symbole associé à une section */
#define STT_FILE 4 /* Nom symbole égal au nom fichier */
#define STT_COMMON 5 /* Symbole données common */
#define STT_TLS 6 /* Symbole données thread-local */
#define STT_NUM 7 /* Nombre types définis */
#define STT_LOOS 10 /* Début plage propre à l’OS */
#define STT_GNU_IFUNC 10 /* Symbole objet code indirect */
#define STT_HIOS 12 /* Fin plage propre à l’OS */
#define STT_LOPROC 13 /* Début spécifique au processeur */
#define STT_HIPROC 15 /* Fin spécifique au processeur */

/* Index terminal du bucket : fin de chaîne hash */

#define STN_UNDEF 0 /* Fin de chaîne */

#define ELF32_ST_VISIBILITY(o) ((o)&0x03)

/* Définitions identiques pour ELF64 */
#define ELF64_ST_VISIBILITY(o) ELF32_ST_VISIBILITY(o)

/* Visibilité symbole encodée dans st_other */
#define STV_DEFAULT 0 /* Règles de visibilité par défaut */
#define STV_INTERNAL 1 /* Classe hidden propre au processeur */
#define STV_HIDDEN 2 /* Symbole indisponible aux autres modules */
#define STV_PROTECTED 3 /* Visible externe, non préemptible dans le module */

/* Relocation sans addend dans section SHT_REL */

typedef struct {
    Elf32_Addr r_offset; /* Adresse */
    Elf32_Word r_info; /* Type relocation et index symbole */
} Elf32_Rel;

/* Format utilisé sur SPARC v9, MIPS et Alpha */

typedef struct {
    Elf64_Addr r_offset; /* Adresse */
    Elf64_Xword r_info; /* Type relocation et index symbole */
} Elf64_Rel;

/* Relocation avec addend dans section SHT_RELA */

typedef struct {
    Elf32_Addr r_offset; /* Adresse */
    Elf32_Word r_info; /* Type relocation et index symbole */
    Elf32_Sword r_addend; /* Addend */
} Elf32_Rela;

typedef struct {
    Elf64_Addr r_offset; /* Adresse */
    Elf64_Xword r_info; /* Type relocation et index symbole */
    Elf64_Sxword r_addend; /* Addend */
} Elf64_Rela;

#define ELF32_R_SYM(val) ((val) >> 8)
#define ELF32_R_TYPE(val) ((val)&0xff)
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type)&0xff))

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i)&0xffffffff)
#define ELF64_R_INFO(sym, type) ((((Elf64_Xword)(sym)) << 32) + (type))

typedef struct {
    Elf32_Word p_type; /* Type segment */
    Elf32_Off p_offset; /* Offset segment dans fichier */
    Elf32_Addr p_vaddr; /* Adresse virtuelle segment */
    Elf32_Addr p_paddr; /* Adresse physique segment */
    Elf32_Word p_filesz; /* Taille segment dans fichier */
    Elf32_Word p_memsz; /* Taille segment en mémoire */
    Elf32_Word p_flags; /* Flags segment */
    Elf32_Word p_align; /* Alignement segment */
} Elf32_Phdr;

typedef struct {
    Elf64_Word p_type; /* Type segment */
    Elf64_Word p_flags; /* Flags segment */
    Elf64_Off p_offset; /* Offset segment dans fichier */
    Elf64_Addr p_vaddr; /* Adresse virtuelle segment */
    Elf64_Addr p_paddr; /* Adresse physique segment */
    Elf64_Xword p_filesz; /* Taille segment dans fichier */
    Elf64_Xword p_memsz; /* Taille segment en mémoire */
    Elf64_Xword p_align; /* Alignement segment */
} Elf64_Phdr;

#define PT_NULL 0 /* Entrée program header inutilisée */
#define PT_LOAD 1 /* Segment programme chargeable */
#define PT_DYNAMIC 2 /* Informations de dynamic linking */
#define PT_INTERP 3 /* Interpreter programme */
#define PT_NOTE 4 /* Informations auxiliaires */
#define PT_SHLIB 5 /* Réservé */
#define PT_PHDR 6 /* Entrée pour la table header elle-même */
#define PT_TLS 7 /* Segment stockage thread-local */
#define PT_NUM 8 /* Nombre types définis */
#define PT_LOOS 0x60000000 /* Début plage propre à l’OS */
#define PT_GNU_EH_FRAME 0x6474e550 /* Segment GCC .eh_frame_hdr */
#define PT_GNU_STACK 0x6474e551 /* Indique si stack exécutable */
#define PT_GNU_RELRO 0x6474e552 /* Lecture seule après relocation */
#define PT_LOSUNW 0x6ffffffa
#define PT_SUNWBSS 0x6ffffffa /* Segment propre à Sun */
#define PT_SUNWSTACK 0x6ffffffb /* Segment stack */
#define PT_HISUNW 0x6fffffff
#define PT_HIOS 0x6fffffff /* Fin plage propre à l’OS */
#define PT_LOPROC 0x70000000 /* Début spécifique au processeur */
#define PT_HIPROC 0x7fffffff /* Fin spécifique au processeur */

#define PF_X (1 << 0) /* Segment exécutable */
#define PF_W (1 << 1) /* Segment modifiable */
#define PF_R (1 << 2) /* Segment lisible */
#define PF_MASKOS 0x0ff00000 /* Propre à l’OS */
#define PF_MASKPROC 0xf0000000 /* Propre au processeur */

#define NT_PRSTATUS 1 /* Copie de prstatus */
#define NT_FPREGSET 2 /* Copie de fpregset */
#define NT_PRPSINFO 3 /* Copie de prpsinfo */
#define NT_PRXREG 4 /* Copie de prxregset */
#define NT_TASKSTRUCT 4 /* Copie de task structure */
#define NT_PLATFORM 5 /* String issue de sysinfo(SI_PLATFORM) */
#define NT_AUXV 6 /* Copie de auxv */
#define NT_GWINDOWS 7 /* Copie de gwindows */
#define NT_ASRS 8 /* Copie de asrset */
#define NT_PSTATUS 10 /* Copie de pstatus */
#define NT_PSINFO 13 /* Copie de psinfo */
#define NT_PRCRED 14 /* Copie de prcred */
#define NT_UTSNAME 15 /* Copie de utsname */
#define NT_LWPSTATUS 16 /* Copie de lwpstatus */
#define NT_LWPSINFO 17 /* Copie de lwpinfo */
#define NT_PRFPXREG 20 /* Copie de fprxregset */
#define NT_PRXFPREG 0x46e62b7f /* Copie de user_fxsr_struct */
#define NT_PPC_VMX 0x100 /* Registres PowerPC Altivec/VMX */
#define NT_PPC_SPE 0x101 /* Registres PowerPC SPE/EVR */
#define NT_PPC_VSX 0x102 /* Registres PowerPC VSX */
#define NT_386_TLS 0x200 /* Slots TLS i386, struct user_desc */
#define NT_386_IOPERM 0x201 /* Bitmap permissions I/O x86 ; 1 refusé */

#define NT_VERSION 1 /* Contient une string de version */

typedef struct {
    Elf32_Sword d_tag; /* Type entrée dynamique */
    union {
        Elf32_Word d_val; /* Valeur entière */
        Elf32_Addr d_ptr; /* Valeur adresse */
    } d_un;
} Elf32_Dyn;

typedef struct {
    Elf64_Sxword d_tag; /* Type entrée dynamique */
    union {
        Elf64_Xword d_val; /* Valeur entière */
        Elf64_Addr d_ptr; /* Valeur adresse */
    } d_un;
} Elf64_Dyn;

#define DT_NULL 0 /* Marque fin de section dynamique */
#define DT_NEEDED 1 /* Nom library requise */
#define DT_PLTRELSZ 2 /* Taille relocations PLT en bytes */
#define DT_PLTGOT 3 /* Valeur définie par le processeur */
#define DT_HASH 4 /* Adresse table hash symboles */
#define DT_STRTAB 5 /* Adresse table string */
#define DT_SYMTAB 6 /* Adresse table symboles */
#define DT_RELA 7 /* Adresse relocations Rela */
#define DT_RELASZ 8 /* Taille totale relocations Rela */
#define DT_RELAENT 9 /* Taille d’une relocation Rela */
#define DT_STRSZ 10 /* Taille table string */
#define DT_SYMENT 11 /* Taille entrée table symboles */
#define DT_INIT 12 /* Adresse fonction init */
#define DT_FINI 13 /* Adresse fonction terminaison */
#define DT_SONAME 14 /* Nom objet partagé */
#define DT_RPATH 15 /* Chemin recherche library obsolète */
#define DT_SYMBOLIC 16 /* Début recherche symbole ici */
#define DT_REL 17 /* Adresse relocations Rel */
#define DT_RELSZ 18 /* Taille totale relocations Rel */
#define DT_RELENT 19 /* Taille d’une relocation Rel */
#define DT_PLTREL 20 /* Type relocation dans PLT */
#define DT_DEBUG 21 /* Usage debug non spécifié */
#define DT_TEXTREL 22 /* Relocation susceptible de modifier .text */
#define DT_JMPREL 23 /* Adresse relocations PLT */
#define DT_BIND_NOW 24 /* Traitement des relocations de l’objet */
#define DT_INIT_ARRAY 25 /* Table adresses fonctions init */
#define DT_FINI_ARRAY 26 /* Table adresses fonctions fini */
#define DT_INIT_ARRAYSZ 27 /* Taille DT_INIT_ARRAY en bytes */
#define DT_FINI_ARRAYSZ 28 /* Taille DT_FINI_ARRAY en bytes */
#define DT_RUNPATH 29 /* Chemin recherche library */
#define DT_FLAGS 30 /* Flags de l’objet en chargement */
#define DT_ENCODING 32 /* Début plage encodée */
#define DT_PREINIT_ARRAY 32 /* Table adresses fonctions preinit */
#define DT_PREINIT_ARRAYSZ 33 /* Taille DT_PREINIT_ARRAY en bytes */
#define DT_NUM 34 /* Nombre utilisé */
#define DT_LOOS 0x6000000d /* Début plage propre à l’OS */
#define DT_HIOS 0x6ffff000 /* Fin plage propre à l’OS */
#define DT_LOPROC 0x70000000 /* Début spécifique au processeur */
#define DT_HIPROC 0x7fffffff /* Fin spécifique au processeur */
#define DT_PROCNUM DT_MIPS_NUM /* Maximum utilisé par un processeur */

/* DT_VALRNGLO..HI utilisent Dyn.d_un.d_val, selon Sun */
#define DT_VALRNGLO 0x6ffffd00
#define DT_GNU_PRELINKED 0x6ffffdf5 /* Timestamp prelink */
#define DT_GNU_CONFLICTSZ 0x6ffffdf6 /* Taille section conflits */
#define DT_GNU_LIBLISTSZ 0x6ffffdf7 /* Taille liste libraries */
#define DT_CHECKSUM 0x6ffffdf8
#define DT_PLTPADSZ 0x6ffffdf9
#define DT_MOVEENT 0x6ffffdfa
#define DT_MOVESZ 0x6ffffdfb
#define DT_FEATURE_1 0x6ffffdfc /* Sélection features DTF_* */
#define DT_POSFLAG_1 \
    0x6ffffdfd /* Flags appliqués à l’entrée DT_* suivante */
#define DT_SYMINSZ 0x6ffffdfe /* Taille table syminfo en bytes */
#define DT_SYMINENT 0x6ffffdff /* Taille entrée syminfo */
#define DT_VALRNGHI 0x6ffffdff
#define DT_VALTAGIDX(tag) (DT_VALRNGHI - (tag)) /* Ordre inverse */
#define DT_VALNUM 12

/* DT_ADDRRNGLO..HI utilisent d_ptr, à ajuster si objet modifié */
#define DT_ADDRRNGLO 0x6ffffe00
#define DT_GNU_HASH 0x6ffffef5 /* Table hash GNU */
#define DT_TLSDESC_PLT 0x6ffffef6
#define DT_TLSDESC_GOT 0x6ffffef7
#define DT_GNU_CONFLICT 0x6ffffef8 /* Début section conflits */
#define DT_GNU_LIBLIST 0x6ffffef9 /* Liste libraries */
#define DT_CONFIG 0x6ffffefa /* Informations configuration */
#define DT_DEPAUDIT 0x6ffffefb /* Audit dépendances */
#define DT_AUDIT 0x6ffffefc /* Audit objet */
#define DT_PLTPAD 0x6ffffefd /* Padding PLT */
#define DT_MOVETAB 0x6ffffefe /* Table move */
#define DT_SYMINFO 0x6ffffeff /* Table syminfo */
#define DT_ADDRRNGHI 0x6ffffeff
#define DT_ADDRTAGIDX(tag) (DT_ADDRRNGHI - (tag)) /* Ordre inverse */
#define DT_ADDRNUM 11

/* Types d’entrées de version : extension GNU */
#define DT_VERSYM 0x6ffffff0

#define DT_RELACOUNT 0x6ffffff9
#define DT_RELCOUNT 0x6ffffffa

/* Valeurs choisies par Sun */
#define DT_FLAGS_1 0x6ffffffb /* Flags d’état DF_1_* */
#define DT_VERDEF \
    0x6ffffffc /* Adresse table définitions version */
#define DT_VERDEFNUM 0x6ffffffd /* Nombre définitions version */
#define DT_VERNEED \
    0x6ffffffe /* Adresse table versions requises */
#define DT_VERNEEDNUM 0x6fffffff /* Nombre versions requises */
#define DT_VERSIONTAGIDX(tag) (DT_VERNEEDNUM - (tag)) /* Ordre inverse */
#define DT_VERSIONTAGNUM 16

/* Extensions Sun indépendantes du processeur dans plage processor-specific */
#define DT_AUXILIARY 0x7ffffffd /* Objet partagé à charger avant celui-ci */
#define DT_FILTER 0x7fffffff /* Objet partagé source des valeurs */
#define DT_EXTRATAGIDX(tag) ((Elf32_Word) - ((Elf32_Sword)(tag) << 1 >> 1) - 1)
#define DT_EXTRANUM 3

#define DF_ORIGIN 0x00000001 /* DF_ORIGIN utilisable par l’objet */
#define DF_SYMBOLIC 0x00000002 /* Début résolution symboles ici */
#define DF_TEXTREL 0x00000004 /* Objet avec relocations text */
#define DF_BIND_NOW 0x00000008 /* Lazy binding interdit */
#define DF_STATIC_TLS 0x00000010 /* TLS statique pour ce module */

#define DF_1_NOW 0x00000001 /* Activation RTLD_NOW */
#define DF_1_GLOBAL 0x00000002 /* Activation RTLD_GLOBAL */
#define DF_1_GROUP 0x00000004 /* Activation RTLD_GROUP */
#define DF_1_NODELETE 0x00000008 /* Activation RTLD_NODELETE */
#define DF_1_LOADFLTR 0x00000010 /* Chargement filtee au runtime */
#define DF_1_INITFIRST 0x00000020 /* Activation RTLD_INITFIRST */
#define DF_1_NOOPEN 0x00000040 /* Activation RTLD_NOOPEN */
#define DF_1_ORIGIN 0x00000080 /* Traitement $ORIGIN obligatoire */
#define DF_1_DIRECT 0x00000100 /* Direct binding activé */
#define DF_1_TRANS 0x00000200
#define DF_1_INTERPOSE 0x00000400 /* Objet utilisé comme interposition */
#define DF_1_NODEFLIB 0x00000800 /* Chemin recherche library par défaut ignoré */
#define DF_1_NODUMP 0x00001000 /* Dldump interdit sur cet objet */
#define DF_1_CONFALT 0x00002000 /* Alternative de configuration créée */
#define DF_1_ENDFILTEE 0x00004000 /* Filtee termine recherche des filters */
#define DF_1_DISPRELDNE 0x00008000 /* Relocation displacement appliquée au build */
#define DF_1_DISPRELPND 0x00010000 /* Relocation displacement appliquée au runtime */

#define DTF_1_PARINIT 0x00000001
#define DTF_1_CONFEXP 0x00000002

/* Flags DT_POSFLAG_1 appliqués uniquement au DT_* suivant */
#define DF_P1_LAZYLOAD 0x00000001 /* Chargement paresseux de l’objet suivant */
#define DF_P1_GROUPPERM \
    0x00000002 /* Symboles de l’objet suivant non disponibles globalement */

typedef struct {
    Elf32_Half vd_version;
    Elf32_Half vd_flags;
    Elf32_Half vd_ndx;
    Elf32_Half vd_cnt;
    Elf32_Word vd_hash;
    Elf32_Word vd_aux;
    Elf32_Word vd_next;
} Elf32_Verdef;

typedef struct {
    Elf64_Half vd_version;
    Elf64_Half vd_flags;
    Elf64_Half vd_ndx;
    Elf64_Half vd_cnt;
    Elf64_Word vd_hash;
    Elf64_Word vd_aux;
    Elf64_Word vd_next;
} Elf64_Verdef;

#define VER_DEF_NONE 0
#define VER_DEF_CURRENT 1 /* Version courante */
#define VER_DEF_NUM 2

#define VER_FLG_BASE 0x1 /* Définition version du fichier lui-même */
#define VER_FLG_WEAK 0x2

#define VER_NDX_LOCAL 0 /* Symbole local */
#define VER_NDX_GLOBAL 1 /* Symbole global */
#define VER_NDX_LORESERVE 0xff00 /* Début entrées réservées */
#define VER_NDX_ELIMINATE 0xff01 /* Symbole à supprimer */

typedef struct {
    Elf32_Word vda_name;
    Elf32_Word vda_next;
} Elf32_Verdaux;

typedef struct {
    Elf64_Word vda_name;
    Elf64_Word vda_next;
} Elf64_Verdaux;

typedef struct {
    Elf32_Half vn_version;
    Elf32_Half vn_cnt;
    Elf32_Word vn_file;
    Elf32_Word vn_aux;
    Elf32_Word vn_next;
} Elf32_Verneed;

typedef struct {
    Elf64_Half vn_version;
    Elf64_Half vn_cnt;
    Elf64_Word vn_file;
    Elf64_Word vn_aux;
    Elf64_Word vn_next;
} Elf64_Verneed;

#define VER_NEED_NONE 0
#define VER_NEED_CURRENT 1 /* Version courante */
#define VER_NEED_NUM 2

typedef struct {
    Elf32_Word vna_hash;
    Elf32_Half vna_flags;
    Elf32_Half vna_other;
    Elf32_Word vna_name;
    Elf32_Word vna_next;
} Elf32_Vernaux;

typedef struct {
    Elf64_Word vna_hash;
    Elf64_Half vna_flags;
    Elf64_Half vna_other;
    Elf64_Word vna_name;
    Elf64_Word vna_next;
} Elf64_Vernaux;

#define VER_FLG_WEAK 0x2

/* Vecteur auxiliaire pour interpreter, renommé pour éviter conflits auxv_t */
/* Tailles convenues entre exec server et interpreter, non spécifiées ici */

typedef struct {
    uint32_t a_type; /* Type entrée */
    union {
        uint32_t a_val; /* Valeur entière */
        /* Pointers exclus : incompatibilité entre définitions 32/64 bits */
    } a_un;
} Elf32_auxv_t;

typedef struct {
    uint64_t a_type; /* Type entrée */
    union {
        uint64_t a_val; /* Valeur entière */
        /* Pointers exclus : incompatibilité entre définitions 32/64 bits */
    } a_un;
} Elf64_auxv_t;

#define AT_NULL 0 /* Fin vecteur */
#define AT_IGNORE 1 /* Entrée à ignorer */
#define AT_EXECFD 2 /* File descriptor programme */
#define AT_PHDR 3 /* Program headers du programme */
#define AT_PHENT 4 /* Taille entrée program header */
#define AT_PHNUM 5 /* Nombre program headers */
#define AT_PAGESZ 6 /* Taille page système */
#define AT_BASE 7 /* Adresse de base interpreter */
#define AT_FLAGS 8 /* Flags */
#define AT_ENTRY 9 /* Point d’entrée programme */
#define AT_NOTELF 10 /* Programme non ELF */
#define AT_UID 11 /* UID réel */
#define AT_EUID 12 /* UID effectif */
#define AT_GID 13 /* GID réel */
#define AT_EGID 14 /* GID effectif */
#define AT_CLKTCK 17 /* Fréquence de times() */

#define AT_PLATFORM 15 /* String identifiant plateforme */
#define AT_HWCAP \
    16 /* Capabilities processeur propres à la machine */

/* Informations sur initialisation FPU par le kernel */
#define AT_FPUCW 18 /* Control word FPU utilisé */

#define AT_DCACHEBSIZE 19 /* Taille bloc cache données */
#define AT_ICACHEBSIZE 20 /* Taille bloc cache instructions */
#define AT_UCACHEBSIZE 21 /* Taille bloc cache unifié */

/* PPC : valeur ignorée de contrôle AUXV, obligatoirement >16 */
#define AT_IGNOREPPC 22 /* Entrée à ignorer */

#define AT_SECURE 23 /* Booléen exec de type setuid */

#define AT_BASE_PLATFORM 24 /* String identifiant plateforme réelle */

#define AT_RANDOM 25 /* Adresse de 16 bytes aléatoires */

#define AT_EXECFN 31 /* Nom fichier exécutable */

/* Pointer page système globale pour syscalls */
#define AT_SYSINFO 32
#define AT_SYSINFO_EHDR 33

/* Cache : associativité bits0..3, log2 ligne bits4..7 ; reste taille */
#define AT_L1I_CACHESHAPE 34
#define AT_L1D_CACHESHAPE 35
#define AT_L2_CACHESHAPE 36
#define AT_L3_CACHESHAPE 37

typedef struct {
    Elf32_Word n_namesz;
    Elf32_Word n_descsz;
    Elf32_Word n_type;
} Elf32_Nhdr;

typedef struct {
    Elf64_Word n_namesz;
    Elf64_Word n_descsz;
    Elf64_Word n_type;
} Elf64_Nhdr;

/* Nom des notes Solaris */
#define ELF_NOTE_SOLARIS "SUNW Solaris"

/* Nom des notes GNU */
#define ELF_NOTE_GNU "GNU"

/* Descripteur WORD : taille page souhaitée pour le binaire */
#define ELF_NOTE_PAGESIZE_HINT 1

/* Descripteur ABI : word0 OS, word1 version majeure */
/* Word2 version mineure, word3 version submineure */
#define NT_GNU_ABI_TAG 1
#define ELF_NOTE_ABI NT_GNU_ABI_TAG /* Ancien nom */

/* OS connus : word0 d’une note NT_GNU_ABI_TAG */
#define ELF_NOTE_OS_LINUX 0
#define ELF_NOTE_OS_GNU 1
#define ELF_NOTE_OS_SOLARIS2 2
#define ELF_NOTE_OS_FREEBSD 3

/* Hwcap : word0 nombre entrées, word1 masque activations */
/* Entrées : byte numéro bit puis nom terminé par NUL */
#define NT_GNU_HWCAP 2

/* Build ID ld --build-id : nombre quelconque de bytes non nul */
#define NT_GNU_BUILD_ID 3

/* Version string produite par GNU gold */
#define NT_GNU_GOLD_VERSION 4

typedef struct {
    Elf32_Xword m_value;
    Elf32_Word m_info;
    Elf32_Word m_poffset;
    Elf32_Half m_repeat;
    Elf32_Half m_stride;
} Elf32_Move;

typedef struct {
    Elf64_Xword m_value;
    Elf64_Xword m_info;
    Elf64_Xword m_poffset;
    Elf64_Half m_repeat;
    Elf64_Half m_stride;
} Elf64_Move;

#define ELF32_M_SYM(info) ((info) >> 8)
#define ELF32_M_SIZE(info) ((unsigned char)(info))
#define ELF32_M_INFO(sym, size) (((sym) << 8) + (unsigned char)(size))

#define ELF64_M_SYM(info) ELF32_M_SYM(info)
#define ELF64_M_SIZE(info) ELF32_M_SIZE(info)
#define ELF64_M_INFO(sym, size) ELF32_M_INFO(sym, size)

/* Flags e_flags propres au processeur */
#define EF_ARM_RELEXEC 0x01
#define EF_ARM_HASENTRY 0x02
#define EF_ARM_INTERWORK 0x04
#define EF_ARM_APCS_26 0x08
#define EF_ARM_APCS_FLOAT 0x10
#define EF_ARM_PIC 0x20
#define EF_ARM_ALIGN8 0x40
#define EF_ARM_NEW_ABI 0x80
#define EF_ARM_OLD_ABI 0x100
#define EF_ARM_SOFT_FLOAT 0x200
#define EF_ARM_VFP_FLOAT 0x400
#define EF_ARM_MAVERICK_FLOAT 0x800

/* NOTE: conflit avec les valeurs définies précédemment */
#define EF_ARM_SYMSARESORTED 0x04
#define EF_ARM_DYNSYMSUSESEGIDX 0x08
#define EF_ARM_MAPSYMSFIRST 0x10
#define EF_ARM_EABIMASK 0XFF000000

#define EF_ARM_BE8 0x00800000
#define EF_ARM_LE8 0x00400000

#define EF_ARM_EABI_VERSION(flags) ((flags)&EF_ARM_EABIMASK)
#define EF_ARM_EABI_UNKNOWN 0x00000000
#define EF_ARM_EABI_VER1 0x01000000
#define EF_ARM_EABI_VER2 0x02000000
#define EF_ARM_EABI_VER3 0x03000000
#define EF_ARM_EABI_VER4 0x04000000
#define EF_ARM_EABI_VER5 0x05000000

/* Types symboles supplémentaires pour Thumb */
#define STT_ARM_TFUNC STT_LOPROC /* Fonction Thumb */
#define STT_ARM_16BIT STT_HIPROC /* Label Thumb */

#define SHF_ARM_ENTRYSECT 0x10000000 /* Section contenant un point d’entrée */
#define SHF_ARM_COMDEF \
    0x80000000 /* Section définissable plusieurs fois parmi les inputs du link */

#define PF_ARM_SB \
    0x10000000 /* Segment contenant l’adresse ciblée par la base statique */
#define PF_ARM_PI 0x20000000 /* Segment indépendant de sa position */
#define PF_ARM_ABS 0x40000000 /* Segment absolu */

#define PT_ARM_EXIDX (PT_LOPROC + 1) /* Segment ARM unwind */

#define SHT_ARM_EXIDX (SHT_LOPROC + 1) /* Section ARM unwind */
#define SHT_ARM_PREEMPTMAP (SHT_LOPROC + 2) /* Détails préemption */
#define SHT_ARM_ATTRIBUTES (SHT_LOPROC + 3) /* Section attributs ARM */

#define R_ARM_NONE 0 /* Aucune relocation */
#define R_ARM_PC24 1 /* Branche PC-relative sur 26 bits */
#define R_ARM_ABS32 2 /* Direct 32 bits */
#define R_ARM_REL32 3 /* PC-relative sur 32 bits */
#define R_ARM_PC13 4
#define R_ARM_ABS16 5 /* Direct 16 bits */
#define R_ARM_ABS12 6 /* Direct 12 bits */
#define R_ARM_THM_ABS5 7
#define R_ARM_ABS8 8 /* Direct 8 bits */
#define R_ARM_SBREL32 9
#define R_ARM_THM_PC22 10
#define R_ARM_THM_PC8 11
#define R_ARM_AMP_VCALL9 12
#define R_ARM_SWI24 13
#define R_ARM_THM_SWI8 14
#define R_ARM_XPC25 15
#define R_ARM_THM_XPC22 16
#define R_ARM_TLS_DTPMOD32 17 /* ID module contenant le symbole */
#define R_ARM_TLS_DTPOFF32 18 /* Offset dans bloc TLS */
#define R_ARM_TLS_TPOFF32 19 /* Offset dans bloc TLS statique */
#define R_ARM_COPY 20 /* Copie symbole au runtime */
#define R_ARM_GLOB_DAT 21 /* Création entrée GOT */
#define R_ARM_JUMP_SLOT 22 /* Création entrée PLT */
#define R_ARM_RELATIVE 23 /* Ajustement selon base programme */
#define R_ARM_GOTOFF 24 /* Offset GOT sur 32 bits */
#define R_ARM_GOTPC 25 /* Offset GOT PC-relative sur 32 bits */
#define R_ARM_GOT32 26 /* Entrée GOT sur 32 bits */
#define R_ARM_PLT32 27 /* Adresse PLT sur 32 bits */
#define R_ARM_THM_JUMP24 30 /* Thumb32 ((S + A) | T) - P */
#define R_ARM_ALU_PCREL_7_0 32
#define R_ARM_ALU_PCREL_15_8 33
#define R_ARM_ALU_PCREL_23_15 34
#define R_ARM_LDR_SBREL_11_0 35
#define R_ARM_ALU_SBREL_19_12 36
#define R_ARM_ALU_SBREL_27_20 37
#define R_ARM_THM_MOVW_ABS_NC 47 /* Direct 16 bits, Thumb32 MOVW */
#define R_ARM_THM_MOVT_ABS 48 /* Direct 16 bits supérieurs */
#define R_ARM_GNU_VTENTRY 100
#define R_ARM_GNU_VTINHERIT 101
#define R_ARM_THM_PC11 102 /* Branche Thumb inconditionnelle */
#define R_ARM_THM_PC9 103 /* Branche Thumb conditionnelle */
#define R_ARM_TLS_GD32 \
    104 /* PC-relative 32 bits pour données TLS global-dynamic */
#define R_ARM_TLS_LDM32 \
    105 /* PC-relative 32 bits pour données TLS local-dynamic */
#define R_ARM_TLS_LDO32 \
    106 /* Offset relatif au bloc TLS sur 32 bits */
#define R_ARM_TLS_IE32 \
    107 /* PC-relative 32 bits pour entrée GOT de l’offset TLS statique */
#define R_ARM_TLS_LE32 \
    108 /* Offset relatif au bloc TLS statique sur 32 bits */
#define R_ARM_RXPC25 249
#define R_ARM_RSBREL32 250
#define R_ARM_THM_RPC22 251
#define R_ARM_RREL32 252
#define R_ARM_RABS22 253
#define R_ARM_RPC24 254
#define R_ARM_RBASE 255
/* Entrée obligatoirement dernière */
#define R_ARM_NUM 256

#endif
