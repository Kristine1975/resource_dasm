Mario Teaches Typing
====================

MuV
---

- Always 44 bytes
- 0x12: 
- 0x1C: Handle to Ply resource of same ID (set upon loading)
- 0x20: Handle to Img resource of same ID (set upon loading)
- 0x28: Handle to Pak resource of same ID (set upon loading)




WRLE in PICT
============

videocodec ffmsrle
  info "Microsoft RLE"
  status working
  format 0x1
  format 0x2
  fourcc WRLE
  driver ffmpeg
  dll "msrle"
  out BGR8


WRLE (ISO) / mrle (RIFF) "Apple BMP"

https://wiki.multimedia.cx/index.php?title=Microsoft_RLE
https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/msrle.c



Missing Types
=============

From System 7.6.1:
- cbnd = Comm Toolbox Resource Bundle, TMPL
- fbnd = Comm Toolbox Resource Bundle (cbnd alias)
- tbnd = Comm Toolbox Resource Bundle (cbnd alias)
- caps = Connection Tool Capabilities List, TMPL
- faps = File Transfer Tool Capabilities List (caps alias)
- taps = Terminal Tool Capabilities List (caps alias)
- dflg = ddev Flags, TMPL
- alis = alias, cf. Hex Fiend template
- DSAT = Deep Shit Alert Table => see BootAlerts.a, 'Ogram's DSAT TMPLs'
- eppc = EPP Configuration, TMPL
- gmcd = “Guard Mechanism for Compression/Decompression”, TMPL
- gmra = ???
- gpch = ???
- gtbl = ???
- hdlg = dialog item balloon help, TMPL
- hfdr = Finder balloon help, TMPL
- hmnu = menu balloon help, TMPL
- hrct = Help Balloon Rectangle List, TMPL
- hovr = Help Balloon Override List, TMPL
- iopc = Input/Output Processor Mgr, AppleTalk. Code for IOP hardware
    "The IOP hardware is called a "Peripheral Interface Controller" or "PIC",
		which can cause some confusion with other names already in the vocabulary
		of Macintosh developers ("PICT" for example), so from the software side,
		we will call them "Input / Output Processors" or "IOP" to avoid (or create)
		confusion."
X- itl0 = number/time/short date format
- itl1 = long date format
- itl2 = string compare hooks
- itl4 = tokenization info
- itlm = sort mapping
- KCAP = keyboard physical layout
- KCHR = key char ASCII mapping
- KMAP = hardware keyboard mapping
- KSWP = keyboard script swap table
- lmem = Toolbox low memory switch table. List of entries (no count), where each entry is (see ProcessMgr/Data.a):
    2 bytes length (upper two bits have special meaning and are always 0)
    4 bytes base pointer
  ends with 0x0000
- lpch = Linked patch, cf. LinkedPatchLoader.a/LinkedPatchMacros.a
    has header etc, see comments in LinkedPatchLoader.a
- MACA = MacWrite creator type ???
- MDRW = ???
- MPNT = MacPaint creator type ??? points?
- mppc - MPP Configuration, TMPL
- onot = ??? STR#?
- pg&e = ??? 32k, all start with "BORG" => PG&E = power management ASIC on DBLite (Powerbook?) and other machines
- picb = picture button
- pixs = pixel data
- pmap = pixmap header
- prob = ???
- rgb  = color => color table-like
- thng = component information => Components.r.
  - Should decode the referenced resource as code
  - Should decode referenced name resource
- timd = ???
- TWIT = "puppet string conversion table" ???
- vdpm = ???
- vm   = ???
- wedg = ??? wedge?


From System 9.0.4:
- gvrs = version info => vers alias?
- kind = custom Finder kind strings => Finder.r
- ldes = list control descriptor => ControlDefinitions.r
- xsig = array of OSTypes


From https://whitefiles.org/mac/pgs/t02.htm:
- fmnu = Finder menu
- fmn2 = Finder menu

Quicktime Components 12-14f:
pnot = preview
  // Contains the modification time (in standard Macintosh seconds since midnight,
  // January 1, 1904) of the file for which the preview was created.
  DATE	Modification date (timestamp)
  // The low bit of the version is a flag for preview components that only reference
  // their data. If the bit is set, it indicates that the resource identified in the
  // preview resource is not owned by the preview component, but is part of the file
  HWRD	Version
  // Contains the type of a resource used as a preview cache for the given file.
  TNAM	Preview resource type (always 0?)
    CASE	Usual=PICT (0)
  // Contains the identification number of a resource used as a preview cache.
  RSID	Resource ID
  WORD  NumResItems
    DATE
    TNAM
    TNAM
    WORD
    WORD
    LONG

3DMF
====

- QuickDraw 3D Object Metafile
- 3D Graphics Programming with QuickDraw 3D 1.5.4#3D Metafile 1.5 Reference



pixs
====

-	rowbytes (high bit set)
-	rect
-	pixel data
-	needs pmap to interpret



pmap
====

Pixmap struct

    base address	  $"00 00 00 00"
    rowbytes		    $"80 06"
    bounds			    $"00 00 00 00 00 0B 00 0B"
    version			    $"00 00"
    pack type		    $"00 00"
    pack size		    $"00 00 00 00"
    fixed hres		  $"00 48 00 00" (72 ppi)
    fixed vres		  $"00 48 00 00"
    pixel type		  $"00 00" (chunky)
    bits per pixel	$"00 04" (16 colors)
    comp count		  $"00 01"
    bits per comp	  $"00 04"
    plane bytes		  $"00 00 00 00"
    color table		  $"00 00 00 00"
    reserved		    $"00 00 00 00"



pxm#
====

See HexFiend template



After Dark
==========

- +ATA:
- +DRL:
- +ERO:
- +ISZ:
- +IZE:
- +ODE:
- +STD:
- +STI:
- +SyP:
- Actv: empty, only name important (module "Fish!")
- ADca:
- ADex: (sprites?)
- ADpr: (preferences)
- Adgm: CODE?
- ADrk: Version string (PSTR).
- aInf: Animation info? (same ID as MiCT/PiCT/SiCT resource)
  - module "Flying Toasters Pro"
  - module "Clocks"
  - several others
  - 30 bytes = one frame only (whole picture?)
- BALS:
- BOIL: TMPL exists in "After Dark 3.0 Faceplate"
- bord: (8 bytes? index + RGB color?)
- bVal: TMPL; Resource name is name of setting
- Cals: TMPL
- CCOD:
- Chnl: TMPL (4n bytes, 2xWORD?)
- ckid: Projector resource (not AD-specific)
- Cost:
- CPLD: WORD (???) + PSTR (Version) + PSTR (Creator)
- CPUt:
- DEFS:
- DLay:
- dlsz: (module "Logo")
- dll#: List (WORD) of PSTR (module "Rebound")
- Finf: font info (12 bytes + PSTR font name)
- Fish: fish animations (module "Fish!")
- HCCS:
- InaP: 
- INFO: TMPL exists in module "Rotating Cube.PICS"
- MiCT: Alias for PICT, masks for PiCT and SiCT
- MMev: 
- MONt: (monitor table?)
- MPST: Version (PSTR)
- Mrph: Image rects? (module "DrawMorph")
- Mrpt: Images? (module "DrawMorph")
- mVal: TMPL; Resource name is name of setting.
- NCMD:
- PiCT: Alias for PICT; black&white version of SiCT
- pLst: list of WORD with picture resource IDs? (module "Logo")
- PORT: (4 bytes?)
- Quiz: Question + possible answers (WORD string count + several CSTR) (module "You Bet Your Head")
- RatN: List (WORD) of CSTR (module "Rat Race")
- Rdat: (50 bytes?) (module "Flying Toasters Pro", "Fish Pro")
- RMAP:
- rsVl:
- RGBv: RGB color (3xWORD). Resource name is name of setting.
- RLEP: Magic at beginning: RLID. Optional additional resources with same ID (module "Flying Toilets"):
  - OFmm: required memory?
  - OFst: offsets?
  - OFtb: frames?
  - module "Flying Toasters Pro"
  - module "Daredevil Dan"
  - module "Bugs"
  - 32 byte header
    -  0: "RLID"
    -  4: 0x00000000?
    -  8: 0x00000000?
    - 12: 0x00000020?
    - 16: 0x00000002?
    - 20: 0x00000000?
    - 24:
    - 28: 0x00000000?
  - CSTM chunks
    -  0: "CSTM"
    -  4: ???
    -  6: image number zero-based
    -  8: last chunk? (32 bit bool)
    - 12: total chunk size
  - CTAB chunks
    -  0: "CTAB"
    -  4: always 0?
    -  6: Number of colors. Each color is 4 bytes.
    -  7: ???
    -  8: last chunk? (32 bit bool)
    - 12: total chunk size
  - at end list of IHDR chunks, each 28 (0x1C) bytes
    - one for each CSTM chunk
    -  0: "IHDR"
    -  4: ???
    -  6: image number zero-based
    - 12: total chunk size (0x1C)
    - last 4 bytes are 2 16 bit values
- RTBC:
- sEEd: WORD random seed
- SEPh:
- SiCT: Alias for PICT; color version of PiCT
- SINs: TMPL
- SIQ™️: TMPL exists in "After Dark" extension
- SPCA:
- sReq: TMPL
- sUnt: TMPL; DWRD + PSTR (popup menu?). Resource name is name of popup menu.
- sVal: TMPL; Resource name is name of setting.
- SYMT: (same IDs/names as CODE resources)
- Sys7: CODE?
- SysP:
- sysz: TMPL; HLNG (amount to increase system heap by (in bytes))
- THUM: TMPL (6 bytes?)
- TiCT: Alias for PICT
- tVal: TMPL
- WRDS: List of CSTR? (module "Flying Toasters Pro", some kind of song?)
- xVal: TMPL
- µVal: TMPL; Resource name is error message? WORD
        - 0x000B:
        - 0x0080:
- _STD:
- _STI:



TODO
====

- CCT™️: System Extension description for Extensions Manager (non-sized, non-terminated string)
- fAni: System 7 Finder, TMPL exists
- MacApp VIEW template
- DITL's info is type-specific, not always a string
- Any TemplateEntry::non-list without name is omitted
- Res-Name => Filename: escape dot? only when first character?
- Res-Name => Filename: trim trailing spaces? (might be there by accident)
- --no-overwrite switch?
- --text-encoding switch? requires conversion tables!
- Tool: rsrcutil <command> ...
  - list <file> [type] [id]
    - Each line: <type>:<id> <name>
    - Or JSON
  - count <file> [type]
  - dump <file> [type] [id]
    --separator=xxx (when dumping several resources)
  - compare <file 1> <file2> [type] [id]
    --ignore-contents (to ignore the resources' actual contents and only check for existence)
  - Options:
    --data-fork
- 68k disassembler: recognize QDExtensions selector
- icns: support additional variants:
  - tile
  - over
  - drop
  - open
  - odrp
- these:
    warning: failed to decode resource DRVR:53: status label is before code start
    ... System 7.6.1.out/System 7.6.1_DRVR_53_.ATADisk.bin
- these (require QuickDraw implementation):
    warning: rendering of PICT:11 failed (unimplemented opcode 0030 before offset DA) [frame rect]
    ... System 7.6.1.out/System 7.6.1_PICT_11.pict
    warning: rendering of PICT:-32510 failed (unimplemented opcode 002B before offset 1E) [dh/dv text]
    ... System 7.6.1.out/System 7.6.1_PICT_-32510.pict
    warning: rendering of PICT:-16620 failed (unimplemented opcode 0031 before offset 27) [paint rect]
    ... System 7.6.1.out/System 7.6.1_PICT_-16620.pict
    warning: rendering of PICT:-16610 failed (unimplemented opcode 0031 before offset 27) [paint rect]
    ... System 7.6.1.out/System 7.6.1_PICT_-16610.pict
    warning: rendering of PICT:-16600 failed (unimplemented opcode 0031 before offset 27) [paint rect]
    ... System 7.6.1.out/System 7.6.1_PICT_-16600.pict
    warning: rendering of PICT:-16525 failed (unimplemented opcode 002E before offset 3C) [glyph state]
    ... System 7.6.1.out/System 7.6.1_PICT_-16525.pict
    warning: rendering of PICT:-8224 failed (unimplemented opcode 0071 before offset 18)  [paint poly]
    ... System 7.6.1.out/System 7.6.1_PICT_-8224.pict
    warning: rendering of PICT:-6046 failed (unimplemented opcode 002B before offset 24) [dh/dv text]
    ... System 7.6.1.out/System 7.6.1_PICT_-6046.pict
    warning: rendering of PICT:-5790 failed (unimplemented opcode 0022 before offset 18)  [short line]
    ... System 7.6.1.out/System 7.6.1_PICT_-5790.pict
    warning: rendering of PICT:-5789 failed (unimplemented opcode 0030 before offset 21)  [frame rect]
    ... System 7.6.1.out/System 7.6.1_PICT_-5790.pict
