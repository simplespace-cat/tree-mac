Project Path: tree-mac

Source Tree:

```txt
tree-mac
├── CHANGES
├── INSTALL
├── LICENSE
├── Makefile
├── README
├── TODO
├── color.c
├── doc
│   ├── global_info
│   ├── tree.1
│   └── xml.dtd
├── file.c
├── filter.c
├── hash.c
├── html.c
├── info.c
├── json.c
├── list.c
├── strverscmp.c
├── tree.c
├── tree.h
├── tree.lsm
├── unix.c
└── xml.c

```

`tree-mac/color.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

/*
 * Hacked in DIR_COLORS support for linux. ------------------------------
 */
/*
 *  If someone asked me, I'd extend dircolors, to provide more generic
 * color support so that more programs could take advantage of it.  This
 * is really just hacked in support.  The dircolors program should:
 * 1) Put the valid terms in a environment var, like:
 *    COLOR_TERMS=linux:console:xterm:vt100...
 * 2) Put the COLOR and OPTIONS directives in a env var too.
 * 3) Have an option to dircolors to silently ignore directives that it
 *    doesn't understand (directives that other programs would
 *    understand).
 * 4) Perhaps even make those unknown directives environment variables.
 *
 * The environment is the place for cryptic crap no one looks at, but
 * programs.  No one is going to care if it takes 30 variables to do
 * something.
 */
enum {
  ERROR = -1, CMD_COLOR = 0, CMD_OPTIONS, CMD_TERM, CMD_EIGHTBIT, COL_RESET,
  COL_NORMAL, COL_FILE, COL_DIR, COL_LINK, COL_FIFO, COL_DOOR, COL_BLK, COL_CHR,
  COL_ORPHAN, COL_SOCK, COL_SETUID, COL_SETGID, COL_STICKY_OTHER_WRITABLE,
  COL_OTHER_WRITABLE, COL_STICKY, COL_EXEC, COL_MISSING,
  COL_LEFTCODE, COL_RIGHTCODE, COL_ENDCODE, COL_BOLD, COL_ITALIC,
/* Keep this one last, sets the size of the color_code array: */
  DOT_EXTENSION
};

enum {
  MCOL_INODE, MCOL_PERMS, MCOL_USER, MCOL_GROUP, MCOL_SIZE, MCOL_DATE,
  MCOL_INDENTLINES
};

bool colorize = false, ansilines = false, linktargetcolor = false;

char *color_code[DOT_EXTENSION+1] = {NULL};

/*
char *vgacolor[] = {
  "black", "red", "green", "yellow", "blue", "fuchsia", "aqua", "white",
  NULL, NULL,
  "transparent", "red", "green", "yellow", "blue", "fuchsia", "aqua", "black"
};
struct colortable {
  char *term_flg, *CSS_name, *font_fg, *font_bg;
} colortable[11];
*/

struct extensions *ext = NULL;
const struct linedraw *linedraw;

char **split(char *str, const char *delim, size_t *nwrds);
int cmd(char *s);

extern FILE *outfile;
extern bool Hflag, force_color, nocolor;
extern const char *charset;

void parse_dir_colors(void)
{
  char **arg, **c, *colors, *s;
  int i, col, cc;
  size_t n;
  struct extensions *e;

  if (Hflag) return;

  s = getenv("NO_COLOR");
  if (s && s[0]) nocolor = true;

  if (getenv("TERM") == NULL) {
    colorize = false;
    return;
  }

  cc = getenv("CLICOLOR") != NULL;
  if (getenv("CLICOLOR_FORCE") != NULL && !nocolor) force_color=true;
  s = getenv("TREE_COLORS");
  if (s == NULL) s = getenv("LS_COLORS");
  if ((s == NULL || strlen(s) == 0) && (force_color || cc)) s = ":no=00:rs=0:fi=00:di=01;34:ln=01;36:pi=40;33:so=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:ex=01;32:*.bat=01;32:*.BAT=01;32:*.btm=01;32:*.BTM=01;32:*.cmd=01;32:*.CMD=01;32:*.com=01;32:*.COM=01;32:*.dll=01;32:*.DLL=01;32:*.exe=01;32:*.EXE=01;32:*.arj=01;31:*.bz2=01;31:*.deb=01;31:*.gz=01;31:*.lzh=01;31:*.rpm=01;31:*.tar=01;31:*.taz=01;31:*.tb2=01;31:*.tbz2=01;31:*.tbz=01;31:*.tgz=01;31:*.tz2=01;31:*.z=01;31:*.Z=01;31:*.zip=01;31:*.ZIP=01;31:*.zoo=01;31:*.asf=01;35:*.ASF=01;35:*.avi=01;35:*.AVI=01;35:*.bmp=01;35:*.BMP=01;35:*.flac=01;35:*.FLAC=01;35:*.gif=01;35:*.GIF=01;35:*.jpg=01;35:*.JPG=01;35:*.jpeg=01;35:*.JPEG=01;35:*.m2a=01;35:*.M2a=01;35:*.m2v=01;35:*.M2V=01;35:*.mov=01;35:*.MOV=01;35:*.mp3=01;35:*.MP3=01;35:*.mpeg=01;35:*.MPEG=01;35:*.mpg=01;35:*.MPG=01;35:*.ogg=01;35:*.OGG=01;35:*.ppm=01;35:*.rm=01;35:*.RM=01;35:*.tga=01;35:*.TGA=01;35:*.tif=01;35:*.TIF=01;35:*.wav=01;35:*.WAV=01;35:*.wmv=01;35:*.WMV=01;35:*.xbm=01;35:*.xpm=01;35:";

  if (s == NULL || (!force_color && (nocolor || !isatty(1)))) {
    colorize = false;
    return;
  }

  colorize = true;

  for(i=0; i < DOT_EXTENSION; i++) color_code[i] = NULL;

  colors = scopy(s);

  arg = split(colors,":",&n);

  for(i=0;arg[i];i++) {
    c = split(arg[i],"=",&n);

    switch(col = cmd(c[0])) {
      case ERROR:
	break;
      case DOT_EXTENSION:
	if (c[1]) {
	  e = xmalloc(sizeof(struct extensions));
	  e->ext = scopy(c[0]+1);
	  e->term_flg = scopy(c[1]);
	  e->nxt = ext;
	  ext = e;
	}
	break;
      case COL_LINK:
	if (c[1] && (strcasecmp("target",c[1]) == 0)) {
	  linktargetcolor = true;
	  color_code[COL_LINK] = "01;36"; /* Should never actually be used */
	  break;
	}
	/* Falls through */
      default:
	if (c[1]) color_code[col] = scopy(c[1]);
	break;
    }

    free(c);
  }
  free(arg);

  /**
   * Make sure at least reset (not normal) is defined.  We're going to assume
   * ANSI/vt100 support:
   */
  if (!color_code[COL_LEFTCODE]) color_code[COL_LEFTCODE] = scopy("\033[");
  if (!color_code[COL_RIGHTCODE]) color_code[COL_RIGHTCODE] = scopy("m");
  if (!color_code[COL_RESET]) color_code[COL_RESET] = scopy("0");
  if (!color_code[COL_BOLD]) {
    color_code[COL_BOLD] = xmalloc(strlen(color_code[COL_LEFTCODE])+strlen(color_code[COL_RIGHTCODE])+2);
    sprintf(color_code[COL_BOLD],"%s1%s",color_code[COL_LEFTCODE],color_code[COL_RIGHTCODE]);
  }
  if (!color_code[COL_ITALIC]) {
    color_code[COL_ITALIC] = xmalloc(strlen(color_code[COL_LEFTCODE])+strlen(color_code[COL_RIGHTCODE])+2);
    sprintf(color_code[COL_ITALIC],"%s3%s",color_code[COL_LEFTCODE],color_code[COL_RIGHTCODE]);
  }
  if (!color_code[COL_ENDCODE]) {
    color_code[COL_ENDCODE] = xmalloc(strlen(color_code[COL_LEFTCODE])+strlen(color_code[COL_RESET])+strlen(color_code[COL_RIGHTCODE])+1);
    sprintf(color_code[COL_ENDCODE],"%s%s%s",color_code[COL_LEFTCODE],color_code[COL_RESET],color_code[COL_RIGHTCODE]);
  }

  free(colors);
}

/*
 * You must free the pointer that is allocated by split() after you
 * are done using the array.
 */
char **split(char *str, const char *delim, size_t *nwrds)
{
  size_t n = 128;
  char **w = xmalloc(sizeof(char *) * n);

  w[*nwrds = 0] = strtok(str,delim);

  while (w[*nwrds]) {
    if (*nwrds == (n-2)) w = xrealloc(w,sizeof(char *) * (n+=256));
    w[++(*nwrds)] = strtok(NULL,delim);
  }

  w[*nwrds] = NULL;
  return w;
}

int cmd(char *s)
{
  static struct {
    char *cmd;
    char cmdnum;
  } cmds[] = {
    {"rs", COL_RESET}, {"no", COL_NORMAL}, {"fi", COL_FILE}, {"di", COL_DIR},
    {"ln", COL_LINK}, {"pi", COL_FIFO}, {"do", COL_DOOR}, {"bd", COL_BLK},
    {"cd", COL_CHR}, {"or", COL_ORPHAN}, {"so", COL_SOCK}, {"su", COL_SETUID},
    {"sg", COL_SETGID}, {"tw", COL_STICKY_OTHER_WRITABLE},
    {"ow", COL_OTHER_WRITABLE}, {"st", COL_STICKY}, {"ex", COL_EXEC},
    {"mi", COL_MISSING}, {"lc", COL_LEFTCODE}, {"rc", COL_RIGHTCODE},
    {"ec", COL_ENDCODE}, {NULL, 0}
  };
  int i;

  if (s == NULL) return ERROR;  /* Probably can't happen */

  if (s[0] == '*') return DOT_EXTENSION;
  for(i=0;cmds[i].cmdnum;i++) {
    if (!strcmp(cmds[i].cmd,s)) return cmds[i].cmdnum;
  }
  return ERROR;
}

bool print_color(int color)
{
  if (!color_code[color]) return false;

  fputs(color_code[COL_LEFTCODE],outfile);
  fputs(color_code[color],outfile);
  fputs(color_code[COL_RIGHTCODE],outfile);
  return true;
}

void endcolor(void)
{
  if (color_code[COL_ENDCODE])
    fputs(color_code[COL_ENDCODE],outfile);
}


void fancy(FILE *out, char *s)
{
  for (; *s; s++) {
    switch(*s) {
      case '\b': if (colorize && color_code[COL_BOLD])    fputs(color_code[COL_BOLD]   , out); break;
      case '\f': if (colorize && color_code[COL_ITALIC])  fputs(color_code[COL_ITALIC] , out); break;
      case '\r': if (colorize && color_code[COL_ENDCODE]) fputs(color_code[COL_ENDCODE], out); break;
      default:
	fputc(*s,out);
    }
  }
}

bool color(mode_t mode, const char *name, bool orphan, bool islink)
{
  struct extensions *e;
  size_t l, xl;

  if (orphan) {
    if (islink) {
      if (print_color(COL_MISSING)) return true;
    } else {
      if (print_color(COL_ORPHAN)) return true;
    }
  }

  /* It's probably safe to assume short-circuit evaluation, but we'll do it this way: */
  switch(mode & S_IFMT) {
    case S_IFIFO:
      return print_color(COL_FIFO);
    case S_IFCHR:
      return print_color(COL_CHR);
    case S_IFDIR:
      if (mode & S_ISVTX) {
	if ((mode & S_IWOTH))
	  if (print_color(COL_STICKY_OTHER_WRITABLE)) return true;
	if (!(mode & S_IWOTH))
	  if (print_color(COL_STICKY)) return true;
      }
      if ((mode & S_IWOTH))
	if (print_color(COL_OTHER_WRITABLE)) return true;
      return print_color(COL_DIR);
#ifndef __EMX__
    case S_IFBLK:
      return print_color(COL_BLK);
    case S_IFLNK:
      return print_color(COL_LINK);
  #ifdef S_IFDOOR
    case S_IFDOOR:
      return print_color(COL_DOOR);
  #endif
#endif
    case S_IFSOCK:
      return print_color(COL_SOCK);
    case S_IFREG:
      if ((mode & S_ISUID))
	if (print_color(COL_SETUID)) return true;
      if ((mode & S_ISGID))
	if (print_color(COL_SETGID)) return true;
      if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
	if (print_color(COL_EXEC)) return true;

      /* not a directory, link, special device, etc, so check for extension match */
      l = strlen(name);
      for(e=ext;e;e=e->nxt) {
	xl = strlen(e->ext);
	if (!strcmp((l>xl)?name+(l-xl):name,e->ext)) {
	  fputs(color_code[COL_LEFTCODE], outfile);
	  fputs(e->term_flg, outfile);
	  fputs(color_code[COL_RIGHTCODE], outfile);
	  return true;
	}
      }
      /* colorize just normal files too */
      return print_color(COL_FILE);
  }
  return print_color(COL_NORMAL);
}

/*
 * Charsets provided by Kyosuke Tokoro (NBG01720@nifty.ne.jp)
 */
const char *getcharset(void)
{
  char *cs;
  static char buffer[256];

  cs = getenv("TREE_CHARSET");
  if (cs) return strncpy(buffer,cs,255);

#ifndef __EMX__
  return NULL;
#else
  ULONG aulCpList[3],ulListSize,codepage=0;

  if(!getenv("WINDOWID"))
    if(!DosQueryCp(sizeof aulCpList,aulCpList,&ulListSize))
      if(ulListSize>=sizeof*aulCpList)
	codepage=*aulCpList;

  switch(codepage) {
    case 437: case 775: case 850: case 851: case 852: case 855:
    case 857: case 860: case 861: case 862: case 863: case 864:
    case 865: case 866: case 868: case 869: case 891: case 903:
    case 904:
      sprintf(buffer,"IBM%03lu",codepage);
      break;
    case 367:
      return"US-ASCII";
    case 813:
      return"ISO-8859-7";
    case 819:
      return"ISO-8859-1";
    case 881: case 882: case 883: case 884: case 885:
      sprintf(buffer,"ISO-8859-%lu",codepage-880);
      break;
    case  858: case  924:
      sprintf(buffer,"IBM%05lu",codepage);
      break;
    case 874:
      return"TIS-620";
    case 897: case 932: case 942: case 943:
      return"Shift_JIS";
    case 912:
      return"ISO-8859-2";
    case 915:
      return"ISO-8859-5";
    case 916:
      return"ISO-8859-8";
    case 949: case 970:
      return"EUC-KR";
    case 950:
      return"Big5";
    case 954:
      return"EUC-JP";
    case 1051:
      return"hp-roman8";
    case 1089:
      return"ISO-8859-6";
    case 1250: case 1251: case 1253: case 1254: case 1255: case 1256:
    case 1257: case 1258:
      sprintf(buffer,"windows-%lu",codepage);
      break;
    case 1252:
      return"ISO-8859-1-Windows-3.1-Latin-1";
    default:
      return NULL;
  }
  return buffer;
#endif
}

void initlinedraw(bool flag)
{
  static const char*latin1_3[]={
    "ISO-8859-1", "ISO-8859-1:1987", "ISO_8859-1", "latin1", "l1", "IBM819",
    "CP819", "csISOLatin1", "ISO-8859-3", "ISO_8859-3:1988", "ISO_8859-3",
    "latin3", "ls", "csISOLatin3", NULL
  };
  static const char*iso8859_789[]={
    "ISO-8859-7", "ISO_8859-7:1987", "ISO_8859-7", "ELOT_928", "ECMA-118",
    "greek", "greek8", "csISOLatinGreek", "ISO-8859-8", "ISO_8859-8:1988",
    "iso-ir-138", "ISO_8859-8", "hebrew", "csISOLatinHebrew", "ISO-8859-9",
    "ISO_8859-9:1989", "iso-ir-148", "ISO_8859-9", "latin5", "l5",
    "csISOLatin5", NULL
  };
  static const char*shift_jis[]={
    "Shift_JIS", "MS_Kanji", "csShiftJIS", NULL
  };
  static const char*euc_jp[]={
    "EUC-JP", "Extended_UNIX_Code_Packed_Format_for_Japanese",
    "csEUCPkdFmtJapanese", NULL
  };
  static const char*euc_kr[]={
    "EUC-KR", "csEUCKR", NULL
  };
  static const char*iso2022jp[]={
    "ISO-2022-JP", "csISO2022JP", "ISO-2022-JP-2", "csISO2022JP2", NULL
  };
  static const char*ibm_pc[]={
    "IBM437", "cp437", "437", "csPC8CodePage437", "IBM852", "cp852", "852",
    "csPCp852", "IBM863", "cp863", "863", "csIBM863", "IBM855", "cp855",
    "855", "csIBM855", "IBM865", "cp865", "865", "csIBM865", "IBM866",
    "cp866", "866", "csIBM866", NULL
  };
  static const char*ibm_ps2[]={
    "IBM850", "cp850", "850", "csPC850Multilingual", "IBM00858", "CCSID00858",
    "CP00858", "PC-Multilingual-850+euro", NULL
  };
  static const char*ibm_gr[]={
    "IBM869", "cp869", "869", "cp-gr", "csIBM869", NULL
  };
  static const char*gb[]={
    "GB2312", "csGB2312", NULL
  };
  static const char*utf8[]={
    "UTF-8", "utf8", NULL
  };
  static const char*big5[]={
    "Big5", "csBig5", NULL
  };
  static const char*viscii[]={
    "VISCII", "csVISCII", NULL
  };
  static const char*koi8ru[]={
    "KOI8-R", "csKOI8R", "KOI8-U", NULL
  };
  static const char*windows[]={
    "ISO-8859-1-Windows-3.1-Latin-1", "csWindows31Latin1",
    "ISO-8859-2-Windows-Latin-2", "csWindows31Latin2", "windows-1250",
    "windows-1251", "windows-1253", "windows-1254", "windows-1255",
    "windows-1256", "windows-1256", "windows-1257", NULL
  };
  
  static const struct linedraw cstable[]={
    { latin1_3,    "|  ",              "|--",            "&middot;--",     "&copy;",
      " [",        " [",               " [",             " [",             " ["       },
    { iso8859_789, "|  ",              "|--",            "&middot;--",     "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { shift_jis,   "\204\240 ",        "\204\245",       "\204\244",       "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { euc_jp,      "\250\242 ",        "\250\247",       "\250\246",       "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { euc_kr,      "\246\242 ",        "\246\247",       "\246\246",       "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { iso2022jp,   "\033$B(\"\033(B ", "\033$B('\033(B", "\033$B(&\033(B", "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { ibm_pc,      "\263  ",           "\303\304\304",   "\300\304\304",   "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { ibm_ps2,     "\263  ",           "\303\304\304",   "\300\304\304",   "\227",
      " [",        " [",               " [",             " [",             " ["       },
    { ibm_gr,      "\263  ",           "\303\304\304",   "\300\304\304",   "\270",
      " [",        " [",               " [",             " [",             " ["       },
    { gb,          "\251\246 ",        "\251\300",       "\251\270",       "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { utf8,        "\342\224\202\302\240\302\240", "\342\224\234\342\224\200\342\224\200",
      "\342\224\224\342\224\200\342\224\200", "\302\251",
      " \342\216\247", " \342\216\251", " \342\216\250", " \342\216\252",  " {"       },
    { big5,        "\242x ",           "\242u",          "\242|",          "(c)",
      " [",        " [",               " [",             " [",             " ["       },
    { viscii,      "|  ",              "|--",            "`--",            "\371",
      " [",        " [",               " [",             " [",             " ["       },
    { koi8ru,      "\201  ",           "\206\200\200",   "\204\200\200",   "\277",
      " [",        " [",               " [",             " [",             " ["       },
    { windows,     "|  ",              "|--",            "`--",            "\251",
      " [",        " [",               " [",             " [",             " ["       },
    { NULL,        "|  ",              "|--",            "`--",            "(c)",
      " [",        " [",               " [",             " [",             " ["       },
  };
  const char**s;

  if (flag) {
    fprintf(stderr,"Valid charsets include:\n");
    for(linedraw=cstable;linedraw->name;++linedraw) {
      for(s=linedraw->name;*s;++s) {
	fprintf(stderr,"  %s\n",*s);
      }
    }
    return;
  }
  if (charset) {
    for(linedraw=cstable;linedraw->name;++linedraw)
      for(s=linedraw->name;*s;++s)
	if(!strcasecmp(charset,*s)) return;
  }
  linedraw=cstable+sizeof cstable/sizeof*cstable-1;
}

```

`tree-mac/file.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

extern bool dflag, aflag, pruneflag, gitignore, showinfo;
extern bool matchdirs, fflinks;
extern int pattern, ipattern;

extern int (*topsort)(const struct _info **, const struct _info **);
extern FILE *outfile;

extern char *file_comment, *file_pathsep;

/* 64K paths maximum */
#define MAXPATH	64*1024

enum ftok { T_PATHSEP, T_DIR, T_FILE, T_EOP };

char *nextpc(char **p, int *tok)
{
  static char prev = 0;
  char *s = *p;
  if (!**p) {
    *tok = T_EOP;	/* Shouldn't happen. */
    return NULL;
  }
  if (prev) {
    prev = 0;
    *tok = T_PATHSEP;
    return NULL;
  }
  if (strchr(file_pathsep, **p) != NULL) {
    (*p)++;
    *tok = T_PATHSEP;
    return NULL;
  }
  while(**p && strchr(file_pathsep,**p) == NULL) (*p)++;

  if (**p) {
    *tok = T_DIR;
    prev = **p;
    *(*p)++ = '\0';
  } else *tok = T_FILE;
  return s;
}

struct _info *newent(const char *name) {
  struct _info *n = xmalloc(sizeof(struct _info));
  memset(n,0,sizeof(struct _info));
  n->name = scopy(name);
  n->child = NULL;
  n->tchild = n->next = NULL;
  return n;
}

/* Don't insertion sort, let fprune() do the sort if necessary */
struct _info *search(struct _info **dir, const char *name)
{
  struct _info *ptr, *prev, *n;
  int cmp;

  if (*dir == NULL) return (*dir = newent(name));

  for(prev = ptr = *dir; ptr != NULL; ptr=ptr->next) {
    cmp = strcmp(ptr->name,name);
    if (cmp == 0) return ptr;
    prev = ptr;
  }
  n = newent(name);
  n->next = ptr;
  if (prev == ptr) *dir = n;
  else prev->next = n;
  return n;
}

void freefiletree(struct _info *ent)
{
  struct _info *ptr = ent, *t;

  while (ptr != NULL) {
    if (ptr->tchild) freefiletree(ptr->tchild);
    t = ptr;
    ptr = ptr->next;
    free(t);
  }
}

/**
 * Recursively prune (unset show flag) files/directories of matches/ignored
 * patterns:
 */
struct _info **fprune(struct _info *head, const char *path, bool matched, bool root)
{
  struct _info **dir, *new = NULL, *end = NULL, *ent, *t;
  struct comment *com;
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  char *cur, *fpath = xmalloc(sizeof(char) * MAXPATH);
  size_t i, count = 0;
  bool show;

  strcpy(fpath, path);
  cur = fpath + strlen(fpath);
  *(cur++) = '/';

  push_files(path, &ig, &inf, root);

  for(ent = head; ent != NULL;) {
    strcpy(cur, ent->name);
    if (ent->tchild) ent->isdir = 1;

    show = true;
    if (dflag && !ent->isdir) show = false;
    if (!aflag && !root && ent->name[0] == '.') show = false;
    if (show && !matched) {
      if (!ent->isdir) {
	if (pattern && !patinclude(ent->name, 0)) show = false;
	if (ipattern && patignore(ent->name, 0)) show = false;
      }
      if (ent->isdir && show && matchdirs && pattern) {
	if (patinclude(ent->name, 1)) matched = true;
      }
    }
    if (pruneflag && !matched && ent->isdir && ent->tchild == NULL) show = false;
    if (gitignore && filtercheck(path, ent->name, ent->isdir)) show = false;
    if (show && showinfo && (com = infocheck(path, ent->name, inf != NULL, ent->isdir))) {
      for(i = 0; com->desc[i] != NULL; i++);
      ent->comment = xmalloc(sizeof(char *) * (i+1));
      for(i = 0; com->desc[i] != NULL; i++) ent->comment[i] = scopy(com->desc[i]);
      ent->comment[i] = NULL;
    }
    if (show && ent->tchild != NULL) ent->child = fprune(ent->tchild, fpath, matched, false);


    t = ent;
    ent = ent->next;
    if (show) {
      if (end) end = end->next = t;
      else new = end = t;
      count++;
    } else {
      t->next = NULL;
      freefiletree(t);
    }
  }
  if (end) end->next = NULL;

  dir = xmalloc(sizeof(struct _info *) * (count+1));
  for(count = 0, ent = new; ent != NULL; ent = ent->next, count++) {
    dir[count] = ent;
  }
  dir[count] = NULL;

  if (topsort) qsort(dir,count,sizeof(struct _info *), (int (*)(const void *, const void *))topsort);

  if (ig != NULL) ig = pop_filterstack();
  if (inf != NULL) inf = pop_infostack();
  free(fpath);

  return dir;
}

struct _info **file_getfulltree(char *d, u_long lev, dev_t dev, off_t *size, char **err)
{
  UNUSED(lev);UNUSED(dev);UNUSED(err);
  FILE *fp = (strcmp(d,".")? fopen(d,"r") : stdin);
  char *path, *spath, *s, *link;
  struct _info *root = NULL, **cwd, *ent;
  int tok;
  size_t l;

  *size = 0;
  if (fp == NULL) {
    fprintf(stderr,"tree: Error opening %s for reading.\n", d);
    return NULL;
  }
  path = xmalloc(sizeof(char) * MAXPATH);

  while(fgets(path, MAXPATH, fp) != NULL) {
    if (file_comment != NULL && strncmp(path,file_comment,strlen(file_comment)) == 0) continue;
    l = strlen(path);
    while(l && (path[l-1] == '\n' || path[l-1] == '\r')) path[--l] = '\0';
    if (l == 0) continue;

    spath = path;
    cwd = &root;

    link = fflinks? strstr(path, " -> ") : NULL;
    if (link) {
      *link = '\0';
      link += 4;
    }
    ent = NULL;
    do {
      s = nextpc(&spath, &tok);
      switch(tok) {
	case T_PATHSEP: continue;
	case T_FILE:
	case T_DIR:
	  /* Should probably handle '.' and '..' entries here */
	  ent = search(cwd, s);
	  /* Might be empty, but should definitely be considered a directory: */
	  if (tok == T_DIR) {
	    ent->isdir = 1;
	    ent->mode = S_IFDIR;
	  } else {
	    ent->mode = S_IFREG;
	  }

	  cwd = &(ent->tchild);
	  break;
      }
    } while (tok != T_FILE && tok != T_EOP);

    if (ent && link) {
      ent->isdir = 0;
      ent->mode = S_IFLNK;
      ent->lnk = scopy(link);
    }
  }
  if (fp != stdin) fclose(fp);

  free(path);

  /* Prune accumulated directory tree: */
  return fprune(root, "", false, true);
}

struct _info **tabedfile_getfulltree(char *d, u_long lev, dev_t dev, off_t *size, char **err)
{
  UNUSED(lev);UNUSED(dev);UNUSED(err);
  FILE *fp = (strcmp(d,".")? fopen(d,"r") : stdin);
  char *path, *spath, *link;
  struct _info *root = NULL, **istack, *ent;
  size_t line = 0, l, tabs, top = 0, maxstack = 2048;

  *size = 0;
  if (fp == NULL) {
    fprintf(stderr,"tree: Error opening %s for reading.\n", d);
    return NULL;
  }
  path = xmalloc(sizeof(char) * MAXPATH);
  istack = xmalloc(sizeof(struct _info *) * maxstack);
  memset(istack, 0, sizeof(struct _info *) * maxstack);

  while(fgets(path, MAXPATH, fp) != NULL) {
    line++;
    if (file_comment != NULL && strncmp(path,file_comment,strlen(file_comment)) == 0) continue;
    l = strlen(path);
    while(l && (path[l-1] == '\n' || path[l-1] == '\r')) path[--l] = '\0';
    if (l == 0) continue;

    for(tabs=0; path[tabs] == '\t'; tabs++);
    if (tabs >= maxstack) {
      fprintf(stderr, "tree: Tab depth exceeds maximum path depth (%ld >= %ld) on line %ld\n", tabs, maxstack, line);
      continue;
    }

    spath = path+tabs;

    link = fflinks? strstr(spath, " -> ") : NULL;
    if (link) {
      *link = '\0';
      link += 4;
    }
    if (tabs > 0 && ((tabs-1 > top) || (istack[tabs-1] == NULL))) {
      fprintf(stderr, "tree: Orphaned file [%s] on line %ld, check tab depth in file.\n", spath, line);
      continue;
    }
    ent = istack[tabs] = search(tabs? &(istack[tabs-1]->tchild) : &root, spath);
    ent->mode = S_IFREG;
    if (tabs) {
      istack[tabs-1]->isdir = 1;
      istack[tabs-1]->mode = S_IFDIR;
    }
    if (link) {
      ent->isdir = 0;
      ent->mode = S_IFLNK;
      ent->lnk = scopy(link);
    }
    top = tabs;
  }
  if (fp != stdin) fclose(fp);

  free(path);
  free(istack);

  /* Prune accumulated directory tree: */
  return fprune(root, "", false, true);
}

```

`tree-mac/filter.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

extern char xpattern[PATH_MAX];

struct ignorefile *filterstack = NULL;

void gittrim(char *s)
{
  ssize_t i, e = (ssize_t)strlen(s)-1;

  if (e < 0) return;
  if (s[e] == '\n') e--;

  for(i = e; i >= 0; i--) {
    if (s[i] != ' ') break;
    if (i && s[i-1] != '\\') e--;
  }
  s[e+1] = '\0';
  for(i = e = 0; s[i] != '\0';) {
    if (s[i] == '\\') i++;
    s[e++] = s[i++];
  }
  s[e] = '\0';
}

struct pattern *new_pattern(char *pattern)
{
  struct pattern *p = xmalloc(sizeof(struct pattern));
  char *sl;

  p->pattern = scopy(pattern + ((pattern[0] == '/')? 1 : 0));
  sl = strchr(pattern, '/');
  p->relative = (sl == NULL || (sl && !*(sl+1)));
  p->next = NULL;
  return p;
}

struct ignorefile *new_ignorefile(const char *path, bool checkparents)
{
  struct stat st;
  char buf[PATH_MAX], rpath[PATH_MAX];
  struct ignorefile *ig;
  struct pattern *remove = NULL, *remend, *p;
  struct pattern *reverse = NULL, *revend;
  int rev;
  FILE *fp;

  rev = stat(path, &st);
  if (rev < 0 || !S_ISREG(st.st_mode)) {
    snprintf(buf, PATH_MAX, "%s/.gitignore", path);
    fp = fopen(buf, "r");

    if (fp == NULL && checkparents) {
      strcpy(rpath, path);
      while ((fp == NULL) && (strcmp(rpath, "/") != 0)) {
	snprintf(buf, PATH_MAX, "%.*s/..", PATH_MAX-4, rpath);
	if (realpath(buf, rpath) == NULL) break;
	snprintf(buf, PATH_MAX, "%.*s/.gitignore", PATH_MAX-12, rpath);
	fp = fopen(buf, "r");
      }
    }
  } else fp = fopen(path, "r");
  if (fp == NULL) return NULL;

  while (fgets(buf, PATH_MAX, fp) != NULL) {
    if (buf[0] == '#') continue;
    rev = (buf[0] == '!');
    gittrim(buf);
    if (strlen(buf) == 0) continue;
    p = new_pattern(buf + (rev? 1 : 0));
    if (rev) {
      if (reverse == NULL) reverse = revend = p;
      else {
	revend->next = p;
	revend = p;
      }
    } else {
      if (remove == NULL) remove = remend = p;
      else {
	remend->next = p;
	remend = p;
      }
    }
  }

  fclose(fp);

  ig = xmalloc(sizeof(struct ignorefile));
  ig->remove = remove;
  ig->reverse = reverse;
  ig->path = scopy(path);
  ig->next = NULL;

  return ig;
}

void push_filterstack(struct ignorefile *ig)
{
  if (ig == NULL) return;
  ig->next = filterstack;
  filterstack = ig;
}

struct ignorefile *pop_filterstack(void)
{
  struct ignorefile *ig;
  struct pattern *p, *c;

  ig = filterstack;
  if (ig == NULL) return NULL;

  filterstack = filterstack->next;

  for(p=c=ig->remove; p != NULL; c = p) {
    p=p->next;
    free(c->pattern);
  }
  for(p=c=ig->reverse; p != NULL; c = p) {
    p=p->next;
    free(c->pattern);
  }
  free(ig->path);
  free(ig);
  return NULL;
}

/**
 * true if remove filter matches and no reverse filter matches.
 */
bool filtercheck(const char *path, const char *name, int isdir)
{
  bool filter = false;
  struct ignorefile *ig;
  struct pattern *p;

  for(ig = filterstack; !filter && ig; ig = ig->next) {
    int fpos = sprintf(xpattern, "%s/", ig->path);

    for(p = ig->remove; p != NULL; p = p->next) {
      if (p->relative) {
	if (patmatch(name, p->pattern, isdir) == 1) {
	  filter = true;
	  break;
	}
      } else {
	sprintf(xpattern + fpos, "%s", p->pattern);
	if (patmatch(path, xpattern, isdir) == 1) {
	  filter = true;
	  break;
	}
      }
     }
  }
  if (!filter) return false;

  for(ig = filterstack; ig; ig = ig->next) {
    int fpos = sprintf(xpattern, "%s/", ig->path);

    for(p = ig->reverse; p != NULL; p = p->next) {
      if (p->relative) {
	if (patmatch(name, p->pattern, isdir) == 1) return false;
      } else {
	sprintf(xpattern + fpos, "%s", p->pattern);
	if (patmatch(path, xpattern, isdir) == 1) return false;
      }
    }
  }

  return true;
}

```

`tree-mac/hash.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

/* Faster uid/gid -> name lookup with hash(tm)(r)(c) tables! */
#define HASH(x)		((x)&255)
struct xtable *gtable[256], *utable[256];

#define inohash(x)	((x)&255)
struct inotable *itable[256];

char *uidtoname(uid_t uid)
{
  struct xtable *o, *p, *t;
  struct passwd *ent;
  char ubuf[32];
  int uent = HASH(uid);
  
  for(o = p = utable[uent]; p ; p=p->nxt) {
    if (uid == p->xid) return p->name;
    else if (uid < p->xid) break;
    o = p;
  }
  /* Not found, do a real lookup and add to table */
  t = xmalloc(sizeof(struct xtable));
  if ((ent = getpwuid(uid)) != NULL) t->name = scopy(ent->pw_name);
  else {
    snprintf(ubuf,30,"%d",uid);
    ubuf[31] = 0;
    t->name = scopy(ubuf);
  }
  t->xid = uid;
  t->nxt = p;
  if (p == utable[uent]) utable[uent] = t;
  else o->nxt = t;
  return t->name;
}

char *gidtoname(gid_t gid)
{
  struct xtable *o, *p, *t;
  struct group *ent;
  char gbuf[32];
  int gent = HASH(gid);
  
  for(o = p = gtable[gent]; p ; p=p->nxt) {
    if (gid == p->xid) return p->name;
    else if (gid < p->xid) break;
    o = p;
  }
  /* Not found, do a real lookup and add to table */
  t = xmalloc(sizeof(struct xtable));
  if ((ent = getgrgid(gid)) != NULL) t->name = scopy(ent->gr_name);
  else {
    snprintf(gbuf,30,"%d",gid);
    gbuf[31] = 0;
    t->name = scopy(gbuf);
  }
  t->xid = gid;
  t->nxt = p;
  if (p == gtable[gent]) gtable[gent] = t;
  else o->nxt = t;
  return t->name;
}

/* Record inode numbers of followed sym-links to avoid re-following them */
void saveino(ino_t inode, dev_t device)
{
  struct inotable *it, *ip, *pp;
  int hp = inohash(inode);

  for(pp = ip = itable[hp];ip;ip = ip->nxt) {
    if (ip->inode > inode) break;
    if (ip->inode == inode && ip->device >= device) break;
    pp = ip;
  }

  if (ip && ip->inode == inode && ip->device == device) return;

  it = xmalloc(sizeof(struct inotable));
  it->inode = inode;
  it->device = device;
  it->nxt = ip;
  if (ip == itable[hp]) itable[hp] = it;
  else pp->nxt = it;
}

bool findino(ino_t inode, dev_t device)
{
  struct inotable *it;

  for(it=itable[inohash(inode)]; it; it=it->nxt) {
    if (it->inode > inode) break;
    if (it->inode == inode && it->device >= device) break;
  }

  if (it && it->inode == inode && it->device == device) return true;
  return false;
}

```

`tree-mac/html.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"


extern bool duflag, dflag, hflag, siflag;
extern bool metafirst, noindent, force_color, nolinks, htmloffset;

extern char *hversion;
extern char *host, *sp, *title, *Hintro, *Houtro;
extern const char *charset;

extern FILE *outfile;

extern const struct linedraw *linedraw;

size_t htmldirlen = 0;

char *class(struct _info *info)
{
  return
    info->isdir  ? "DIR"  :
    info->isexe  ? "EXEC" :
    info->isfifo ? "FIFO" :
    info->issok  ? "SOCK" : "NORM";
}

void html_encode(FILE *fd, char *s)
{
  for(;*s;s++) {
    switch(*s) {
      case '<':
	fputs("&lt;",fd);
	break;
      case '>':
	fputs("&gt;",fd);
	break;
      case '&':
	fputs("&amp;",fd);
	break;
      case '"':
	fputs("&quot;",fd);
	break;
      default:
	fputc(*s,fd);
	break;
    }
  }
}

void url_encode(FILE *fd, char *s)
{
  // Removes / from the reserved list:
  static const char *reserved = "!#$&'()*+,:;=?@[]";

  for(;*s;s++) {
    fprintf(fd, (isprint((u_int)*s) && (strchr(reserved, *s) == NULL))? "%c":"%%%02X", *s);
  }
}

void fcat(const char *filename)
{
  FILE *fp;
  char buf[PATH_MAX];
  size_t n;

  if ((fp = fopen(filename, "r")) == NULL) return;
  while((n = fread(buf, sizeof(char), PATH_MAX, fp)) > 0) {
    fwrite(buf, sizeof(char), n, outfile);
  }
  fclose(fp);
}

void html_intro(void)
{
  if (Hintro) fcat(Hintro);
  else {
    fprintf(outfile,
	"<!DOCTYPE html>\n"
	"<html>\n"
	"<head>\n"
	" <meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">\n"
	" <meta name=\"Author\" content=\"Made by 'tree'\">\n"
	" <meta name=\"GENERATOR\" content=\"", charset ? charset : "iso-8859-1");
    print_version(false);
    fprintf(outfile, "\">\n"
	" <title>%s</title>\n"
	" <style type=\"text/css\">\n"
	"  BODY { font-family : monospace, sans-serif;  color: black;}\n"
	"  P { font-family : monospace, sans-serif; color: black; margin:0px; padding: 0px;}\n"
	"  A:visited { text-decoration : none; margin : 0px; padding : 0px;}\n"
	"  A:link    { text-decoration : none; margin : 0px; padding : 0px;}\n"
	"  A:hover   { text-decoration: underline; background-color : yellow; margin : 0px; padding : 0px;}\n"
	"  A:active  { margin : 0px; padding : 0px;}\n"
	"  .VERSION { font-size: small; font-family : arial, sans-serif; }\n"
	"  .NORM  { color: black;  }\n"
	"  .FIFO  { color: purple; }\n"
	"  .CHAR  { color: yellow; }\n"
	"  .DIR   { color: blue;   }\n"
	"  .BLOCK { color: yellow; }\n"
	"  .LINK  { color: aqua;   }\n"
	"  .SOCK  { color: fuchsia;}\n"
	"  .EXEC  { color: green;  }\n"
	" </style>\n"
	"</head>\n"
	"<body>\n"
	"\t<h1>%s</h1><p>\n", title, title);
  }
}

void html_outtro(void)
{
  if (Houtro) fcat(Houtro);
  else {
    fprintf(outfile,"\t<hr>\n");
    fprintf(outfile,"\t<p class=\"VERSION\">\n");
    fprintf(outfile,hversion,linedraw->copy, linedraw->copy, linedraw->copy, linedraw->copy);
    fprintf(outfile,"\t</p>\n");
    fprintf(outfile,"</body>\n");
    fprintf(outfile,"</html>\n");
  }
}

void html_print(char *s)
{
  int i;
  for(i=0; s[i]; i++) {
    if (s[i] == ' ') fprintf(outfile,"%s",sp);
    else fprintf(outfile,"%c", s[i]);
  }
  fprintf(outfile,"%s%s", sp, sp);
}

int html_printinfo(char *dirname, struct _info *file, int level)
{
  UNUSED(dirname);

  char info[512];

  fillinfo(info,file);
  if (metafirst) {
    if (info[0] == '[') {
      html_print(info);
      fprintf(outfile,"%s%s", sp, sp);
    }
    if (!noindent) indent(level);
  } else {
    if (!noindent) indent(level);
    if (info[0] == '[') {
      html_print(info);
      fprintf(outfile,"%s%s", sp, sp);
    }
  }

  return 0;
}

/* descend == add 00Tree.html to the link */
int html_printfile(char *dirname, char *filename, struct _info *file, int descend)
{
  int i;
  /* Switch to using 'a' elements only. Omit href attribute if not a link */
  fprintf(outfile,"<a");
  if (file) {
    if (force_color) fprintf(outfile," class=\"%s\"", class(file));
    if (file->comment) {
      fprintf(outfile," title=\"");
      for(i=0; file->comment[i]; i++) {
	html_encode(outfile, file->comment[i]);
	if (file->comment[i+1]) fprintf(outfile, "\n");
      }
      fprintf(outfile, "\"");
    }

    if (!nolinks) {
      fprintf(outfile," href=\"%s",host);
      if (dirname != NULL) {
	size_t len = strlen(dirname);
	size_t off = (len >= htmldirlen? htmldirlen : 0);
	url_encode(outfile, dirname + (htmloffset? off : 0));
	if (strcmp(dirname, filename) != 0) {
	  if (dirname[strlen(dirname)-1] != '/') putc('/', outfile);
	  url_encode(outfile, filename);
	}
	fprintf(outfile,"%s%s\"",(descend > 1? "/00Tree.html" : ""), (file->isdir && descend < 2?"/":""));
      } else {
	if (host[strlen(host)-1] != '/') putc('/', outfile);
	url_encode(outfile, filename);
	fprintf(outfile,"%s\"",(descend > 1? "/00Tree.html" : ""));
      }
    }
  }
  fprintf(outfile, ">");

  if (dirname) html_encode(outfile,filename);
  else html_encode(outfile, host);

  fprintf(outfile,"</a>");
  return 0;
}

int html_error(char *error)
{
  fprintf(outfile, "  [%s]", error);
  return 0;
}

void html_newline(struct _info *file, int level, int postdir, int needcomma)
{
  UNUSED(file);UNUSED(level);UNUSED(postdir);UNUSED(needcomma);

  fprintf(outfile, "<br>\n");
}

void html_close(struct _info *file, int level, int needcomma)
{
  UNUSED(level);UNUSED(needcomma);

  fprintf(outfile, "</%s><br>\n", file->tag);
}

void html_report(struct totals tot)
{
  char buf[256];

  fprintf(outfile,"<br><br><p>\n\n");

  if (duflag) {
    psize(buf, tot.size);
    fprintf(outfile,"%s%s used in ", buf, hflag || siflag? "" : " bytes");
  }
  if (dflag)
    fprintf(outfile,"%ld director%s\n",tot.dirs,(tot.dirs==1? "y":"ies"));
  else
    fprintf(outfile,"%ld director%s, %ld file%s\n",tot.dirs,(tot.dirs==1? "y":"ies"),tot.files,(tot.files==1? "":"s"));

  fprintf(outfile, "\n</p>\n");
}

```

`tree-mac/info.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

/**
 * TODO: Make a "filenote" command for info comments.
 * maybe TODO: Support language extensions (i.e. .info.en, .info.gr, etc)
 * # comments
 * pattern
 * pattern
 * 	info messages
 * 	more info
 */
extern FILE *outfile;
extern const struct linedraw *linedraw;
extern char xpattern[PATH_MAX];

struct infofile *infostack = NULL;

struct comment *new_comment(struct pattern *phead, char **line, int lines)
{
  int i;

  struct comment *com = xmalloc(sizeof(struct comment));
  com->pattern = phead;
  com->desc = xmalloc(sizeof(char *) * (size_t)(lines+1));
  for(i=0; i < lines; i++) com->desc[i] = line[i];
  com->desc[i] = NULL;
  com->next = NULL;
  return com;
}

struct infofile *new_infofile(const char *path, bool checkparents)
{
  struct stat st;
  char buf[PATH_MAX], rpath[PATH_MAX];
  struct infofile *inf;
  struct comment *chead = NULL, *cend = NULL, *com;
  struct pattern *phead = NULL, *pend = NULL, *p;
  char *line[PATH_MAX];
  FILE *fp;
  int i, lines = 0;

  i = stat(path, &st);
  if (i < 0 || !S_ISREG(st.st_mode)) {
    snprintf(buf, PATH_MAX, "%s/.info", path);
    fp = fopen(buf, "r");

    if (fp == NULL && checkparents) {
      strcpy(rpath, path);
      while ((fp == NULL) && (strcmp(rpath, "/") != 0)) {
	snprintf(buf, PATH_MAX, "%.*s/..", PATH_MAX-4, rpath);
	if (realpath(buf, rpath) == NULL) break;
	snprintf(buf, PATH_MAX, "%.*s/.info", PATH_MAX-7, rpath);
	fp = fopen(buf, "r");
      }
    }
  } else fp = fopen(path, "r");
  if (fp == NULL) return NULL;

  while (fgets(buf, PATH_MAX, fp) != NULL) {
    if (buf[0] == '#') continue;
    gittrim(buf);
    if (strlen(buf) < 1) continue;

    if (buf[0] == '\t') {
      line[lines++] = scopy(buf+1);
    } else {
      if (lines) {
	/* Save previous pattern/message: */
	if (phead) {
	  com = new_comment(phead, line, lines);
	  if (!chead) chead = cend = com;
	  else cend = cend->next = com;
	} else {
	  /* Accumulated info message lines w/ no associated pattern? */
	  for(i=0; i < lines; i++) free(line[i]);
	}
	/* Reset for next pattern/message: */
	phead = pend = NULL;
	lines = 0;
      }
      p = new_pattern(buf);
      if (phead == NULL) phead = pend = p;
      else pend = pend->next = p;
    }
  }
  if (phead) {
    com = new_comment(phead, line, lines);
    if (!chead) chead = cend = com;
    else cend = cend->next = com;
  } else {
    for(i=0; i < lines; i++) free(line[i]);
  }

  fclose(fp);

  inf = xmalloc(sizeof(struct infofile));
  inf->comments = chead;
  inf->path = scopy(path);
  inf->next = NULL;

  return inf;
}

void push_infostack(struct infofile *inf)
{
  if (inf == NULL) return;
  inf->next = infostack;
  infostack = inf;
}

struct infofile *pop_infostack(void)
{
  struct infofile *inf;
  struct comment *cn, *cc;
  struct pattern *p, *c;
  int i;

  inf = infostack;
  if (inf == NULL) return NULL;

  infostack = infostack->next;

  for(cn = cc = inf->comments; cn != NULL; cc = cn) {
    cn = cn->next;
    for(p=c=cc->pattern; p != NULL; c = p) {
      p=p->next;
      free(c->pattern);
    }
    for(i=0; cc->desc[i] != NULL; i++) free(cc->desc[i]);
    free(cc->desc);
    free(cc);
  }
  free(inf->path);
  free(inf);
  return NULL;
}

/**
 * Returns an info pointer if a path matches a pattern.
 * top == 1 if called in a directory with a .info file.
 */
struct comment *infocheck(const char *path, const char *name, int top, bool isdir)
{
  struct infofile *inf = infostack;
  struct comment *com;
  struct pattern *p;

  if (inf == NULL) return NULL;

  for(inf = infostack; inf != NULL; inf = inf->next) {
    int fpos = sprintf(xpattern, "%s/", inf->path);

    for(com = inf->comments; com != NULL; com = com->next) {
      for(p = com->pattern; p != NULL; p = p->next) {
	if (patmatch(path, p->pattern, isdir) == 1) return com;
	if (top && patmatch(name, p->pattern, isdir) == 1) return com;

	sprintf(xpattern + fpos, "%s", p->pattern);
	if (patmatch(path, xpattern, isdir) == 1) return com;
      }
    }
    top = 0;
  }
  return NULL;
}

void printcomment(size_t line, size_t lines, char *s)
{
  if (lines == 1) fprintf(outfile, "%s ", linedraw->csingle);
  else {
    if (line == 0) fprintf(outfile, "%s ", linedraw->ctop);
    else if (line < 2) {
      fprintf(outfile, "%s ", (lines==2)? linedraw->cbot : linedraw->cmid);
    } else {
      fprintf(outfile, "%s ", (line == lines-1)? linedraw->cbot : linedraw->cext);
    }
  }
  fprintf(outfile, "%s\n", s);
}

```

`tree-mac/json.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

extern bool dflag, pflag, sflag, uflag, gflag, Dflag, inodeflag, devflag;
extern bool cflag, hflag, siflag, duflag, noindent;

extern const mode_t ifmt[];
extern const char *ftype[];

extern FILE *outfile;

/*  JSON code courtesy of Florian Sesser <fs@it-agenten.com>
[
  {"type": "directory", "name": "name", "mode": "0777", "user": "user", "group": "group", "inode": ###, "dev": ####, "time": "00:00 00-00-0000", "info": "<file comment>", "contents": [
    {"type": "link", "name": "name", "target": "name", "info": "...", "contents": [... if link is followed, otherwise this is empty.]}
    {"type": "file", "name": "name", "mode": "0777", "size": ###, "group": "group", "inode": ###, "dev": ###, "time": "00:00 00-00-0000", "info": "..."}
    {"type": "socket", "name": "", "info": "...", "error": "some error" ...}
    {"type": "block", "name": "" ...},
    {"type": "char", "name": "" ...},
    {"type": "fifo", "name": "" ...},
    {"type": "door", "name": "" ...},
    {"type": "port", "name": "" ...}
  ]},
  {"type": "report", "size": ###, "files": ###, "directories": ###}
]
*/

/**
 * JSON encoded strings are not HTML/XML strings:
 * https://tools.ietf.org/html/rfc8259#section-7
 * FIXME: Still not UTF-8
 */
void json_encode(FILE *fd, char *s)
{
  char *ctrl = "0-------btn-fr------------------";
  
  for(;*s;s++) {
    if ((unsigned char)*s < 32) {
      if (ctrl[(unsigned char)*s] != '-') fprintf(fd, "\\%c", ctrl[(unsigned char)*s]);
      else fprintf(fd, "\\u%04x", (unsigned char)*s);
    } else if (*s == '"' || *s == '\\') fprintf(fd, "\\%c", *s);
    else fprintf(fd, "%c", *s);
  }
}

void json_indent(int maxlevel)
{
  int i;

  fprintf(outfile, "  ");
  for(i=0; i<maxlevel; i++)
    fprintf(outfile, "  ");
}

void json_fillinfo(struct _info *ent)
{
  #ifdef __USE_FILE_OFFSET64
  if (inodeflag) fprintf(outfile,",\"inode\":%lld",(long long)ent->inode);
  #else
  if (inodeflag) fprintf(outfile,",\"inode\":%ld",(long int)ent->inode);
  #endif
  if (devflag) fprintf(outfile, ",\"dev\":%d", (int)ent->dev);
  #ifdef __EMX__
  if (pflag) fprintf(outfile, ",\"mode\":\"%04o\",\"prot\":\"%s\"",ent->attr, prot(ent->attr));
  #else
  if (pflag) fprintf(outfile, ",\"mode\":\"%04o\",\"prot\":\"%s\"", ent->mode & (S_IRWXU|S_IRWXG|S_IRWXO|S_ISUID|S_ISGID|S_ISVTX), prot(ent->mode));
  #endif
  if (uflag) fprintf(outfile, ",\"user\":\"%s\"", uidtoname(ent->uid));
  if (gflag) fprintf(outfile, ",\"group\":\"%s\"", gidtoname(ent->gid));
  if (sflag) {
    if (hflag || siflag) {
      char nbuf[64];
      int i;
      psize(nbuf,ent->size);
      for(i=0; isspace(nbuf[i]); i++);	/* trim() hack */
      fprintf(outfile, ",\"size\":\"%s\"", nbuf+i);
    } else
      fprintf(outfile, ",\"size\":%lld", (long long int)ent->size);
  }
  if (Dflag) fprintf(outfile, ",\"time\":\"%s\"", do_date(cflag? ent->ctime : ent->mtime));
}


void json_intro(void)
{
  extern char *_nl;
  fprintf(outfile, "[%s", noindent? "" : _nl);
}

void json_outtro(void)
{
  extern char *_nl;
  fprintf(outfile, "%s]\n", noindent? "" : _nl);
}

int json_printinfo(char *dirname, struct _info *file, int level)
{
  UNUSED(dirname);
  mode_t mt;
  int t;

  if (!noindent) json_indent(level);

  if (file != NULL) {
    if (file->lnk) mt = file->mode & S_IFMT;
    else mt = file->mode & S_IFMT;
  } else mt = 0;

  for(t=0;ifmt[t];t++)
    if (ifmt[t] == mt) break;
  fprintf(outfile,"{\"type\":\"%s\"", ftype[t]);

  return 0;
}

int json_printfile(char *dirname, char *filename, struct _info *file, int descend)
{
  UNUSED(dirname);
  int i;

  fprintf(outfile, ",\"name\":\"");
  json_encode(outfile, filename);
  fputc('"',outfile);

  if (file && file->comment) {
    fprintf(outfile, ",\"info\":\"");
    for(i=0; file->comment[i]; i++) {
      json_encode(outfile, file->comment[i]);
      if (file->comment[i+1]) fprintf(outfile, "\\n");
    }
    fprintf(outfile, "\"");
  }

  if (file && file->lnk) {
    fprintf(outfile, ",\"target\":\"");
    json_encode(outfile, file->lnk);
    fputc('"',outfile);
  }
  if (file) json_fillinfo(file);

//   if (file && file->err) fprintf(outfile, ",\"error\": \"%s\"", file->err);
  if (descend || (file->isdir && file->err)) fprintf(outfile, ",\"contents\":[");
  else fputc('}',outfile);

  return descend || (file->isdir && file->err);
}

int json_error(char *error)
{
  fprintf(outfile,"{\"error\": \"%s\"}%s",error, noindent?"":"");
  return 0;
}

void json_newline(struct _info *file, int level, int postdir, int needcomma)
{
  UNUSED(file);UNUSED(level);UNUSED(postdir);
  extern char *_nl;

  fprintf(outfile, "%s%s", needcomma? "," : "", _nl);
}

void json_close(struct _info *file, int level, int needcomma)
{
  UNUSED(file);
  if (!noindent) json_indent(level);
  fprintf(outfile,"]}%s%s", needcomma? ",":"", noindent? "":"\n");
}

void json_report(struct totals tot)
{
  fprintf(outfile, ",%s{\"type\":\"report\"",noindent?"":"\n  ");
  if (duflag) fprintf(outfile,",\"size\":%lld", (long long int)tot.size);
  fprintf(outfile,",\"directories\":%ld", tot.dirs);
  if (!dflag) fprintf(outfile,",\"files\":%ld", tot.files);
  fprintf(outfile, "}");
}

```

`tree-mac/list.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

extern bool fflag, duflag, lflag, Rflag, Jflag, xdev, noreport, hyperflag, Hflag;

extern struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev, off_t *size, char **err);
extern int (*topsort)(struct _info **, struct _info **);
extern FILE *outfile;
extern int flimit, *dirs, errors;
extern ssize_t Level;
extern size_t htmldirlen;

static char errbuf[256];
char realbasepath[PATH_MAX];
size_t dirpathoffset = 0;

/**
 * Maybe TODO: Refactor the listing calls / when they are called.  A more thorough
 * analysis of the different outputs is required.  This all is not as clean as I
 * had hoped it to be.
 */

extern struct listingcalls lc;

void null_intro(void)
{
  return;
}

void null_outtro(void)
{
  return;
}

void null_close(struct _info *file, int level, int needcomma)
{
  UNUSED(file);UNUSED(level);UNUSED(needcomma);
}

void emit_tree(char **dirname, bool needfulltree)
{
  struct totals tot = { 0 }, subtotal;
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **dir = NULL, *info = NULL;
  char *err;
  ssize_t n;
  int i, needsclosed;
  size_t j;
  struct stat st;

  lc.intro();

  for(i=0; dirname[i]; i++) {
    if (hyperflag) {
      if (realpath(dirname[i], realbasepath) == NULL) { realbasepath[0] = '\0'; dirpathoffset = 0; }
      else dirpathoffset = strlen(dirname[i]);
    }

    if (fflag) {
      j=strlen(dirname[i]);
      do {
	if (j > 1 && dirname[i][j-1] == '/') dirname[i][--j] = 0;
      } while (j > 1 && dirname[i][j-1] == '/');
    }
    if (Hflag) htmldirlen = strlen(dirname[i]);

    if ((n = lstat(dirname[i],&st)) >= 0) {
      saveino(st.st_ino, st.st_dev);
      info = stat2info(&st);
      info->name = ""; //dirname[i];

      if (needfulltree) {
	dir = getfulltree(dirname[i], 0, st.st_dev, &(info->size), &err);
	n = err? -1 : 0;
      } else {
	push_files(dirname[i], &ig, &inf, true);
	dir = read_dir(dirname[i], &n, inf != NULL);
      }

      lc.printinfo(dirname[i], info, 0);
    } else info = NULL;

    needsclosed = lc.printfile(dirname[i], dirname[i], info, (dir != NULL) || (!dir && n));
    subtotal = (struct totals){0, 0, 0};

    if (!dir && n) {
      lc.error("error opening dir");
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      if (!info) errors++;
      else subtotal.files++;
    } else if (flimit > 0 && n > flimit) {
      sprintf(errbuf,"%ld entries exceeds filelimit, not opening dir", n);
      lc.error(errbuf);
      lc.newline(info, 0, 0, dirname[i+1] != NULL);
      subtotal.dirs++;
    } else {
      lc.newline(info, 0, 0, 0);
      if (dir) {
	subtotal = listdir(dirname[i], dir, 1, st.st_dev, needfulltree);
	subtotal.dirs++;
      }
    }
    if (dir) {
      free_dir(dir);
      dir = NULL;
    }
    if (needsclosed) lc.close(info, 0, dirname[i+1] != NULL);

    tot.files += subtotal.files;
    tot.dirs += subtotal.dirs;
    // Do not bother to accumulate tot.size in listdir.
    // This is already done in getfulltree()
    if (duflag) tot.size += info? info->size : 0;

    if (ig != NULL) ig = pop_filterstack();
    if (inf != NULL) inf = pop_infostack();
  }

  if (!noreport) lc.report(tot);

  lc.outtro();
}

struct totals listdir(char *dirname, struct _info **dir, int lev, dev_t dev, bool hasfulltree)
{
  struct totals tot = {0}, subtotal;
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **subdir = NULL;
  size_t namemax = 257, namelen;
  int descend, htmldescend = 0;
  int needsclosed;
  ssize_t n;
  size_t dirlen = strlen(dirname)+2, pathlen = dirlen + 257;
  bool found;
  char *path, *newpath, *filename, *err = NULL;

  int es = (dirname[strlen(dirname) - 1] == '/');

  // Sanity check on dir, may or may not be necessary when using --fromfile:
  if (dir == NULL || *dir == NULL) return tot;

  for(n=0; dir[n]; n++);
  if (topsort) qsort(dir, (size_t)n, sizeof(struct _info *), (int (*)(const void *, const void *))topsort);

  dirs[lev] = *(dir+1)? 1 : 2;

  path = xmalloc(sizeof(char) * pathlen);

  for (;*dir != NULL; dir++) {
    lc.printinfo(dirname, *dir, lev);

    namelen = strlen((*dir)->name) + 1;
    if (namemax < namelen)
      path = xrealloc(path, dirlen + (namemax = namelen));
    if (es) sprintf(path,"%s%s",dirname,(*dir)->name);
    else sprintf(path,"%s/%s",dirname,(*dir)->name);
    if (fflag) filename = path;
    else filename = (*dir)->name;

    descend = 0;
    err = NULL;
    newpath = path;

    if ((*dir)->isdir) {
      tot.dirs++;

      if (!hasfulltree) {
	found = findino((*dir)->inode,(*dir)->dev);
	if (!found) {
	  saveino((*dir)->inode, (*dir)->dev);
	}
      } else found = false;

      if (!(xdev && dev != (*dir)->dev) && (!(*dir)->lnk || ((*dir)->lnk && lflag))) {
	descend = 1;

	if ((*dir)->lnk) {
	  if (*(*dir)->lnk == '/') newpath = (*dir)->lnk;
	  else {
	    if (fflag && !strcmp(dirname,"/")) sprintf(path,"%s%s",dirname,(*dir)->lnk);
	    else sprintf(path,"%s/%s",dirname,(*dir)->lnk);
	  }
	  if (found) {
	    err = "recursive, not followed";
	    /* Not actually a problem if we weren't going to descend anyway: */
	    if (Level >= 0 && lev > Level) err = NULL;
	    descend = -1;
	  }
	}

	if ((Level >= 0) && (lev > Level)) {
	  if (Rflag) {
	    FILE *outsave = outfile;
	    char *paths[2] = {newpath, NULL}, *output = xmalloc(strlen(newpath) + 13);
	    int *dirsave = xmalloc(sizeof(int) * (size_t)(lev + 2));

	    memcpy(dirsave, dirs, sizeof(int) * (size_t)(lev+1));
	    sprintf(output, "%s/00Tree.html", newpath);
	    setoutput(output);
	    emit_tree(paths, hasfulltree);

	    free(output);
	    fclose(outfile);
	    outfile = outsave;

	    memcpy(dirs, dirsave, sizeof(int) * (size_t)(lev+1));
	    free(dirsave);
	    htmldescend = 10;
	  } else htmldescend = 0;
	  descend = 0;
	}

	if (descend > 0) {
	  if (hasfulltree) {
	    subdir = (*dir)->child;
	    err = (*dir)->err;
	  } else {
	    push_files(newpath, &ig, &inf, false);
	    subdir = read_dir(newpath, &n, inf != NULL);
	    if (!subdir && n) {
	      err = "error opening dir";
	      errors++;
	    } if (flimit > 0 && n > flimit) {
	      sprintf(err = errbuf,"%ld entries exceeds filelimit, not opening dir", n);
	      errors++;
	      free_dir(subdir);
	      subdir = NULL;
	    }
	  }
	  if (subdir == NULL) descend = 0;
	}
      }
    } else tot.files++;

    needsclosed = lc.printfile(dirname, filename, *dir, descend + htmldescend + (Jflag && errors));
    if (err) lc.error(err);

    if (descend > 0) {
      lc.newline(*dir, lev, 0, 0);

      subtotal = listdir(newpath, subdir, lev+1, dev, hasfulltree);
      tot.dirs += subtotal.dirs;
      tot.files += subtotal.files;
    } else if (!needsclosed) lc.newline(*dir, lev, 0, *(dir+1)!=NULL);

    if (subdir) {
      free_dir(subdir);
      subdir = NULL;
    }
    if (needsclosed) lc.close(*dir, descend? lev : -1, *(dir+1)!=NULL);

    if (*(dir+1) && !*(dir+2)) dirs[lev] = 2;

    if (ig != NULL) ig = pop_filterstack();
    if (inf != NULL) inf = pop_infostack();
  }

  dirs[lev] = 0;
  free(path);
  return tot;
}

```

`tree-mac/tree.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tree.h"

char *version = "$Version: $ tree v2.2.1 %s 1996 - 2024 by Steve Baker, Thomas Moore, Francesc Rocher, Florian Sesser, Kyosuke Tokoro $";
char *hversion= "\t\t tree v2.2.1 %s 1996 - 2024 by Steve Baker and Thomas Moore <br>\n"
		"\t\t HTML output hacked and copyleft %s 1998 by Francesc Rocher <br>\n"
		"\t\t JSON output hacked and copyleft %s 2014 by Florian Sesser <br>\n"
		"\t\t Charsets / OS/2 support %s 2001 by Kyosuke Tokoro\n";

/* Globals */
bool dflag, lflag, pflag, sflag, Fflag, aflag, fflag, uflag, gflag;
bool qflag, Nflag, Qflag, Dflag, inodeflag, devflag, hflag, Rflag;
bool Hflag, siflag, cflag, Xflag, Jflag, duflag, pruneflag, hyperflag;
bool noindent, force_color, nocolor, xdev, noreport, nolinks;
bool ignorecase, matchdirs, fromfile, metafirst, gitignore, showinfo;
bool reverse, fflinks, htmloffset;
int flimit;

struct listingcalls lc;

int pattern = 0, maxpattern = 0, ipattern = 0, maxipattern = 0;
char **patterns = NULL, **ipatterns = NULL;

char *host = NULL, *title = "Directory Tree", *sp = " ", *_nl = "\n";
char *Hintro = NULL, *Houtro = NULL, *scheme = "file://", *authority = NULL;
char *file_comment = "#", *file_pathsep = "/";
char *timefmt = NULL;
const char *charset = NULL;

struct _info **(*getfulltree)(char *d, u_long lev, dev_t dev, off_t *size, char **err) = unix_getfulltree;
/* off_t (*listdir)(char *, int *, int *, u_long, dev_t) = unix_listdir; */
int (*basesort)(struct _info **, struct _info **) = alnumsort;
int (*topsort)(struct _info **, struct _info **) = NULL;

char *sLevel, *curdir;
FILE *outfile = NULL;
int *dirs;
ssize_t Level;
size_t maxdirs;
int errors;

char xpattern[PATH_MAX];

int mb_cur_max;

#ifdef __EMX__
const u_short ifmt[]={ FILE_ARCHIVED, FILE_DIRECTORY, FILE_SYSTEM, FILE_HIDDEN, FILE_READONLY, 0};
#else
  #ifdef S_IFPORT
  const mode_t ifmt[] = {S_IFREG, S_IFDIR, S_IFLNK, S_IFCHR, S_IFBLK, S_IFSOCK, S_IFIFO, S_IFDOOR, S_IFPORT, 0};
  const char fmt[] = "-dlcbspDP?";
  const char *ftype[] = {"file", "directory", "link", "char", "block", "socket", "fifo", "door", "port", "unknown", NULL};
  #else
  const mode_t ifmt[] = {S_IFREG, S_IFDIR, S_IFLNK, S_IFCHR, S_IFBLK, S_IFSOCK, S_IFIFO, 0};
  const char fmt[] = "-dlcbsp?";
  const char *ftype[] = {"file", "directory", "link", "char", "block", "socket", "fifo", "unknown", NULL};
  #endif
#endif

struct sorts {
  char *name;
  int (*cmpfunc)(struct _info **, struct _info **);
} sorts[] = {
  {"name", alnumsort},
  {"version", versort},
  {"size", fsizesort},
  {"mtime", mtimesort},
  {"ctime", ctimesort},
  {"none", NULL},
  {NULL, NULL}
};

/* Externs */
/* hash.c */
extern struct xtable *gtable[256], *utable[256];
extern struct inotable *itable[256];

/* color.c */
extern bool colorize, ansilines, linktargetcolor;
extern char *leftcode, *rightcode, *endcode;
extern const struct linedraw *linedraw;

/* Time to switch to getopt()? */
char *long_arg(char *argv[], size_t i, size_t *j, size_t *n, char *prefix) {
  char *ret = NULL;
  size_t len = strlen(prefix);

  if (!strncmp(prefix,argv[i], len)) {
    *j = len;
    if (*(argv[i]+(*j)) == '=') {
      if (*(argv[i]+ (++(*j)))) {
	ret=(argv[i] + (*j));
	*j = strlen(argv[i])-1;
      } else {
	fprintf(stderr,"tree: Missing argument to %s=\n", prefix);
	if (strcmp(prefix, "--charset=") == 0) initlinedraw(true);
	exit(1);
      }
    } else if (argv[*n] != NULL) {
      ret = argv[*n];
      (*n)++;
      *j = strlen(argv[i])-1;
    } else {
      fprintf(stderr,"tree: Missing argument to %s\n", prefix);
      if (strcmp(prefix, "--charset") == 0) initlinedraw(true);
      exit(1);
    }
  }
  return ret;
}

int main(int argc, char **argv)
{
  struct ignorefile *ig;
  struct infofile *inf;
  char **dirname = NULL;
  size_t i, j=0, k, n, p = 0, q = 0;
  bool optf = true;
  char *stmp, *outfilename = NULL, *arg;
  char *stddata_fd;
  bool needfulltree, showversion = false, opt_toggle = false;

  aflag = dflag = fflag = lflag = pflag = sflag = Fflag = uflag = gflag = false;
  Dflag = qflag = Nflag = Qflag = Rflag = hflag = Hflag = siflag = cflag = false;
  noindent = force_color = nocolor = xdev = noreport = nolinks = reverse = false;
  ignorecase = matchdirs = inodeflag = devflag = Xflag = Jflag = fflinks = false;
  duflag = pruneflag = metafirst = gitignore = hyperflag = htmloffset = false;

  flimit = 0;
  dirs = xmalloc(sizeof(int) * (size_t)(maxdirs=PATH_MAX));
  memset(dirs, 0, sizeof(int) * (size_t)maxdirs);
  dirs[0] = 0;
  Level = -1;

  setlocale(LC_CTYPE, "");
  setlocale(LC_COLLATE, "");

  charset = getcharset();
  if (charset == NULL && 
       (strcmp(nl_langinfo(CODESET), "UTF-8") == 0 ||
        strcmp(nl_langinfo(CODESET), "utf8") == 0)) {
    charset = "UTF-8";
  }

  lc = (struct listingcalls){
    null_intro, null_outtro, unix_printinfo, unix_printfile, unix_error, unix_newline,
    null_close, unix_report
  };

/* Still a hack, but assume that if the macro is defined, we can use it: */
#ifdef MB_CUR_MAX
  mb_cur_max = (int)MB_CUR_MAX;
#else
  mb_cur_max = 1;
#endif

#ifdef __linux__
  /* Output JSON automatically to "stddata" if present: */
  stddata_fd = getenv(ENV_STDDATA_FD);
  if (stddata_fd != NULL) {
    int std_fd = atoi(stddata_fd);
    if (std_fd <= 0) std_fd = STDDATA_FILENO;
    if (fcntl(std_fd, F_GETFD) >= 0) {
      Jflag = noindent = true;
      _nl = "";
      lc = (struct listingcalls){
	json_intro, json_outtro, json_printinfo, json_printfile, json_error, json_newline,
	json_close, json_report
      };
      outfile = fdopen(std_fd, "w");
    }
  }
#endif

  memset(utable,0,sizeof(utable));
  memset(gtable,0,sizeof(gtable));
  memset(itable,0,sizeof(itable));

  for(n=i=1;i<(size_t)argc;i=n) {
    n++;
    if (optf && argv[i][0] == '-' && argv[i][1]) {
      for(j=1;argv[i][j];j++) {
	switch(argv[i][j]) {
	case 'N':
	  Nflag = (opt_toggle? !Nflag : true);
	  break;
	case 'q':
	  qflag = (opt_toggle? !qflag : true);
	  break;
	case 'Q':
	  Qflag = (opt_toggle? !Qflag : true);
	  break;
	case 'd':
	  dflag = (opt_toggle? !dflag : true);
	  break;
	case 'l':
	  lflag = (opt_toggle? !lflag : true);
	  break;
	case 's':
	  sflag = (opt_toggle? !sflag : true);
	  break;
	case 'h':
	  /* Assume they also want -s */
	  sflag = (hflag = (opt_toggle? !hflag : true));
	  break;
	case 'u':
	  uflag = (opt_toggle? !uflag : true);
	  break;
	case 'g':
	  gflag = (opt_toggle? !gflag : true);
	  break;
	case 'f':
	  fflag = (opt_toggle? !fflag : true);
	  break;
	case 'F':
	  Fflag = (opt_toggle? !Fflag : true);
	  break;
	case 'a':
	  aflag = (opt_toggle? !aflag : true);
	  break;
	case 'p':
	  pflag = (opt_toggle? !pflag : true);
	  break;
	case 'i':
	  noindent = (opt_toggle? !noindent : true);
	  _nl = "";
	  break;
	case 'C':
	  force_color = (opt_toggle? !force_color : true);
	  break;
	case 'n':
	  nocolor = (opt_toggle? !nocolor : true);
	  break;
	case 'x':
	  xdev = (opt_toggle? !xdev : true);
	  break;
	case 'P':
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: Missing argument to -P option.\n");
	    exit(1);
	  }
	  if (pattern >= maxpattern-1) patterns = xrealloc(patterns, sizeof(char *) * (size_t)(maxpattern += 10));
	  patterns[pattern++] = argv[n++];
	  patterns[pattern] = NULL;
	  break;
	case 'I':
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: Missing argument to -I option.\n");
	    exit(1);
	  }
	  if (ipattern >= maxipattern-1) ipatterns = xrealloc(ipatterns, sizeof(char *) * (size_t)(maxipattern += 10));
	  ipatterns[ipattern++] = argv[n++];
	  ipatterns[ipattern] = NULL;
	  break;
	case 'A':
	  ansilines = (opt_toggle? !ansilines : true);
	  break;
	case 'S':
	  charset = "IBM437";
	  break;
	case 'D':
	  Dflag = (opt_toggle? !Dflag : true);
	  break;
	case 't':
	  basesort = mtimesort;
	  break;
	case 'c':
	  basesort = ctimesort;
	  cflag = true;
	  break;
	case 'r':
	  reverse = (opt_toggle? !reverse : true);
	  break;
	case 'v':
	  basesort = versort;
	  break;
	case 'U':
	  basesort = NULL;
	  break;
	case 'X':
	  Xflag = true;
	  Hflag = Jflag = false;
	  lc = (struct listingcalls){
	    xml_intro, xml_outtro, xml_printinfo, xml_printfile, xml_error, xml_newline,
	    xml_close, xml_report
	  };
	  break;
	case 'J':
	  Jflag = true;
	  Xflag = Hflag = false;
	  lc = (struct listingcalls){
	    json_intro, json_outtro, json_printinfo, json_printfile, json_error, json_newline,
	    json_close, json_report
	  };
	  break;
	case 'H':
	  Hflag = true;
	  Xflag = Jflag = false;
	  lc = (struct listingcalls){
	    html_intro, html_outtro, html_printinfo, html_printfile, html_error, html_newline,
	    html_close, html_report
	  };
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: Missing argument to -H option.\n");
	    exit(1);
	  }
	  host = argv[n++];
	  k = strlen(host)-1;
	  if (host[0] == '-') {
	    htmloffset = true;
	    host++;
	  }
	  /* Allows a / if that is the only character as the 'host': */
//	  if (k && host[k] == '/') host[k] = '\0';
	  sp = "&nbsp;";
	  break;
	case 'T':
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: Missing argument to -T option.\n");
	    exit(1);
	  }
	  title = argv[n++];
	  break;
	case 'R':
	  Rflag = (opt_toggle? !Rflag : true);
	  break;
	case 'L':
	  if (isdigit(argv[i][j+1])) {
	    for(k=0; (argv[i][j+1+k] != '\0') && (isdigit(argv[i][j+1+k])) && (k < PATH_MAX-1); k++) {
	      xpattern[k] = argv[i][j+1+k];
	    }
	    xpattern[k] = '\0';
	    j += k;
	    sLevel = xpattern;
	  } else {
	    if ((sLevel = argv[n++]) == NULL) {
	      fprintf(stderr,"tree: Missing argument to -L option.\n");
	      exit(1);
	    }
	  }
	  Level = (int)strtoul(sLevel,NULL,0)-1;
	  if (Level < 0) {
	    fprintf(stderr,"tree: Invalid level, must be greater than 0.\n");
	    exit(1);
	  }
	  break;
	case 'o':
	  if (argv[n] == NULL) {
	    fprintf(stderr,"tree: Missing argument to -o option.\n");
	    exit(1);
	  }
	  outfilename = argv[n++];
	  break;
	case '-':
	  if (j == 1) {
	    if (!strcmp("--", argv[i])) {
	      optf = false;
	      break;
	    }
	    /* Long options that don't take parameters should just use strcmp: */
	    if (!strcmp("--help",argv[i])) {
	      usage(2);
	      exit(0);
	    }
	    if (!strcmp("--version",argv[i])) {
	      j = strlen(argv[i])-1;
	      showversion = true;
	      break;
	    }
	    if (!strcmp("--inodes",argv[i])) {
	      j = strlen(argv[i])-1;
	      inodeflag = (opt_toggle? !inodeflag : true);
	      break;
	    }
	    if (!strcmp("--device",argv[i])) {
	      j = strlen(argv[i])-1;
	      devflag = (opt_toggle? !devflag : true);
	      break;
	    }
	    if (!strcmp("--noreport",argv[i])) {
	      j = strlen(argv[i])-1;
	      noreport = (opt_toggle? !noreport : true);
	      break;
	    }
	    if (!strcmp("--nolinks",argv[i])) {
	      j = strlen(argv[i])-1;
	      nolinks = (opt_toggle? !nolinks : true);
	      break;
	    }
	    if (!strcmp("--dirsfirst",argv[i])) {
	      j = strlen(argv[i])-1;
	      topsort = dirsfirst;
	      break;
	    }
	    if (!strcmp("--filesfirst",argv[i])) {
	      j = strlen(argv[i])-1;
	      topsort = filesfirst;
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--filelimit")) != NULL) {
	      flimit = atoi(arg);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--charset")) != NULL) {
	      charset = arg;
	      break;
	    }
	    if (!strcmp("--si", argv[i])) {
	      j = strlen(argv[i])-1;
	      sflag = hflag = siflag = (opt_toggle? !siflag : true);
	      break;
	    }
	    if (!strcmp("--du",argv[i])) {
	      j = strlen(argv[i])-1;
	      sflag = duflag = (opt_toggle? !duflag : true);
	      break;
	    }
	    if (!strcmp("--prune",argv[i])) {
	      j = strlen(argv[i])-1;
	      pruneflag = (opt_toggle? !pruneflag : true);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--timefmt")) != NULL) {
	      timefmt = scopy(arg);
	      Dflag = true;
	      break;
	    }
	    if (!strcmp("--ignore-case",argv[i])) {
	      j = strlen(argv[i])-1;
	      ignorecase = (opt_toggle? !ignorecase : true);
	      break;
	    }
	    if (!strcmp("--matchdirs",argv[i])) {
	      j = strlen(argv[i])-1;
	      matchdirs = (opt_toggle? !matchdirs : true);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--sort")) != NULL) {
	      basesort = NULL;
	      for(k=0;sorts[k].name;k++) {
		if (strcasecmp(sorts[k].name,arg) == 0) {
		  basesort = sorts[k].cmpfunc;
		  break;
		}
	      }
	      if (sorts[k].name == NULL) {
		fprintf(stderr,"tree: Sort type '%s' not valid, should be one of: ", arg);
		for(k=0; sorts[k].name; k++)
		  printf("%s%c", sorts[k].name, sorts[k+1].name? ',': '\n');
		exit(1);
	      }
	      break;
	    }
	    if (!strcmp("--fromtabfile", argv[i])) {
	      j = strlen(argv[i])-1;
	      fromfile=true;
	      getfulltree = tabedfile_getfulltree;
	      break;
	    }
	    if (!strcmp("--fromfile",argv[i])) {
	      j = strlen(argv[i])-1;
	      fromfile=true;
	      getfulltree = file_getfulltree;
	      break;
	    }
	    if (!strcmp("--metafirst",argv[i])) {
	      j = strlen(argv[i])-1;
	      metafirst = (opt_toggle? !metafirst : true);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--gitfile")) != NULL) {
	      gitignore=true;
	      ig = new_ignorefile(arg, false);
	      if (ig != NULL) push_filterstack(ig);
	      else {
		fprintf(stderr,"tree: Could not load gitignore file\n");
		exit(1);
	      }
	      break;
	    }
	    if (!strcmp("--gitignore",argv[i])) {
	      j = strlen(argv[i])-1;
	      gitignore = (opt_toggle? !gitignore : true);
	      break;
	    }
	    if (!strcmp("--info",argv[i])) {
	      j = strlen(argv[i])-1;
	      showinfo = (opt_toggle? !showinfo : true);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--infofile")) != NULL) {
	      showinfo = true;
	      inf = new_infofile(arg, false);
	      if (inf != NULL) push_infostack(inf);
	      else {
		fprintf(stderr,"tree: Could not load infofile\n");
		exit(1);
	      }
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--hintro")) != NULL) {
	      Hintro = scopy(arg);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--houtro")) != NULL) {
	      Houtro = scopy(arg);
	      break;
	    }
	    if (!strcmp("--fflinks",argv[i])) {
	      j = strlen(argv[i])-1;
	      fflinks = (opt_toggle? !fflinks : true);
	      break;
	    }
	    if (!strcmp("--hyperlink", argv[i])) {
	      j = strlen(argv[i])-1;
	      hyperflag = (opt_toggle? !hyperflag : true);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--scheme")) != NULL) {
	      if (strchr(arg, ':') == NULL) {
		sprintf(xpattern, "%s://", arg);
		arg = scopy(xpattern);
	      } else scheme = scopy(arg);
	      break;
	    }
	    if ((arg = long_arg(argv, i, &j, &n, "--authority")) != NULL) {
	      // I don't believe that . by itself can be a valid hostname,
	      // so it will do as a null authority.
	      if (strcmp(arg, ".") == 0) authority = scopy("");
	      else authority = scopy(arg);
	      break;
	    }
	    if (!strcmp("--opt-toggle", argv[i])) {
	      j = strlen(argv[i])-1;
	      opt_toggle = !opt_toggle;
	      break;
	    }

	    fprintf(stderr,"tree: Invalid argument `%s'.\n",argv[i]);
	    usage(1);
	    exit(1);
	  }
	  /* Falls through */
	default:
	  /* printf("here i = %d, n = %d\n", i, n); */
	  fprintf(stderr,"tree: Invalid argument -`%c'.\n",argv[i][j]);
	  usage(1);
	  exit(1);
	  break;
	}
      }
    } else {
      if (!dirname) dirname = (char **)xmalloc(sizeof(char *) * (q=MINIT));
      else if (p == (q-2)) dirname = (char **)xrealloc(dirname,sizeof(char *) * (q+=MINC));
      dirname[p++] = scopy(argv[i]);
    }
  }
  if (p) dirname[p] = NULL;

  setoutput(outfilename);

  parse_dir_colors();
  initlinedraw(false);
  
  if (showversion) {
    print_version(true);
    exit(0);
  }

  /* Insure sensible defaults and sanity check options: */
  if (dirname == NULL) {
    dirname = xmalloc(sizeof(char *) * 2);
    dirname[0] = scopy(".");
    dirname[1] = NULL;
  }
  if (topsort == NULL) topsort = basesort;
  if (basesort == NULL) topsort = NULL;
  if (timefmt) setlocale(LC_TIME,"");
  if (dflag) pruneflag = false;  /* You'll just get nothing otherwise. */
  if (Rflag && (Level == -1)) Rflag = false;
  
  if (hyperflag && authority == NULL) {
    // If the hostname is longer than PATH_MAX, maybe it's just as well we don't
    // try to use it.
    if (gethostname(xpattern,PATH_MAX) < 0) {
      fprintf(stderr,"Unable to get hostname, using 'localhost'.\n");
      authority = "localhost";
    } else authority = scopy(xpattern);
  }

  /* Not going to implement git configs so no core.excludesFile support. */
  if (gitignore && (stmp = getenv("GIT_DIR"))) {
    char *path = xmalloc(PATH_MAX);
    snprintf(path, PATH_MAX, "%s/info/exclude", stmp);
    push_filterstack(new_ignorefile(path, false));
    free(path);
  }
  if (showinfo) {
    push_infostack(new_infofile(INFO_PATH, false));
  }

  needfulltree = duflag || pruneflag || matchdirs || fromfile;

  emit_tree(dirname, needfulltree);

  if (outfilename != NULL) fclose(outfile);

  return errors ? 2 : 0;
}

void print_version(int nl)
{
  char buf[PATH_MAX], *v;
  v = version+12;
  sprintf(buf, "%.*s%s", (int)strlen(v)-2, v, nl?"\n":"");
  fprintf(outfile, buf, linedraw->copy);
}

void setoutput(const char *filename)
{
  if (filename == NULL) {
#ifdef __EMX__
    _fsetmode(outfile=stdout,Hflag?"b":"t");
#else
    if (outfile == NULL) outfile = stdout;
#endif
  } else {
#ifdef __EMX__
    outfile = fopen(filename, Hflag? "wb":"wt");
#else
    outfile = fopen(filename, "w");
#endif
    if (outfile == NULL) {
      fprintf(stderr,"tree: invalid filename '%s'\n", filename);
      exit(1);
    }
  }
}

void usage(int n)
{
  parse_dir_colors();
  initlinedraw(false);

  /*     123456789!123456789!123456789!123456789!123456789!123456789!123456789!123456789! */
  /*     \t9!123456789!123456789!123456789!123456789!123456789!123456789!123456789! */
  fancy(n < 2? stderr: stdout,
	"usage: \btree\r [\b-acdfghilnpqrstuvxACDFJQNSUX\r] [\b-L\r \flevel\r [\b-R\r]] [\b-H\r [-]\fbaseHREF\r]\n"
	"\t[\b-T\r \ftitle\r] [\b-o\r \ffilename\r] [\b-P\r \fpattern\r] [\b-I\r \fpattern\r] [\b--gitignore\r]\n"
	"\t[\b--gitfile\r[\b=\r]\ffile\r] [\b--matchdirs\r] [\b--metafirst\r] [\b--ignore-case\r]\n"
	"\t[\b--nolinks\r] [\b--hintro\r[\b=\r]\ffile\r] [\b--houtro\r[\b=\r]\ffile\r] [\b--inodes\r] [\b--device\r]\n"
	"\t[\b--sort\r[\b=\r]\fname\r] [\b--dirsfirst\r] [\b--filesfirst\r] [\b--filelimit\r[\b=\r]\f#\r] [\b--si\r]\n"
	"\t[\b--du\r] [\b--prune\r] [\b--charset\r[\b=\r]\fX\r] [\b--timefmt\r[\b=\r]\fformat\r] [\b--fromfile\r]\n"
	"\t[\b--fromtabfile\r] [\b--fflinks\r] [\b--info\r] [\b--infofile\r[\b=\r]\ffile\r] [\b--noreport\r]\n"
	"\t[\b--hyperlink\r] [\b--scheme\r[\b=\r]\fschema\r] [\b--authority\r[\b=\r]\fhost\r] [\b--opt-toggle\r]\n"
	"\t[\b--version\r] [\b--help\r] [\b--\r] [\fdirectory\r \b...\r]\n");

  if (n < 2) return;
  fancy(stdout,
	"  \b------- Listing options -------\r\n"
	"  \b-a\r            All files are listed.\n"
	"  \b-d\r            List directories only.\n"
	"  \b-l\r            Follow symbolic links like directories.\n"
	"  \b-f\r            Print the full path prefix for each file.\n"
	"  \b-x\r            Stay on current filesystem only.\n"
	"  \b-L\r \flevel\r      Descend only \flevel\r directories deep.\n"
	"  \b-R\r            Rerun tree when max dir level reached.\n"
	"  \b-P\r \fpattern\r    List only those files that match the pattern given.\n"
	"  \b-I\r \fpattern\r    Do not list files that match the given pattern.\n"
	"  \b--gitignore\r   Filter by using \b.gitignore\r files.\n"
	"  \b--gitfile\r \fX\r   Explicitly read a gitignore file.\n"
	"  \b--ignore-case\r Ignore case when pattern matching.\n"
	"  \b--matchdirs\r   Include directory names in \b-P\r pattern matching.\n"
	"  \b--metafirst\r   Print meta-data at the beginning of each line.\n"
	"  \b--prune\r       Prune empty directories from the output.\n"
	"  \b--info\r        Print information about files found in \b.info\r files.\n"
	"  \b--infofile\r \fX\r  Explicitly read info file.\n"
	"  \b--noreport\r    Turn off file/directory count at end of tree listing.\n"
	"  \b--charset\r \fX\r   Use charset \fX\r for terminal/HTML and indentation line output.\n"
	"  \b--filelimit\r \f#\r Do not descend dirs with more than \f#\r files in them.\n"
	"  \b-o\r \ffilename\r   Output to file instead of stdout.\n"
	"  \b------- File options -------\r\n"
	"  \b-q\r            Print non-printable characters as '\b?\r'.\n"
	"  \b-N\r            Print non-printable characters as is.\n"
	"  \b-Q\r            Quote filenames with double quotes.\n"
	"  \b-p\r            Print the protections for each file.\n"
	"  \b-u\r            Displays file owner or UID number.\n"
	"  \b-g\r            Displays file group owner or GID number.\n"
	"  \b-s\r            Print the size in bytes of each file.\n"
	"  \b-h\r            Print the size in a more human readable way.\n"
	"  \b--si\r          Like \b-h\r, but use in SI units (powers of 1000).\n"
	"  \b--du\r          Compute size of directories by their contents.\n"
	"  \b-D\r            Print the date of last modification or (-c) status change.\n"
	"  \b--timefmt\r \ffmt\r Print and format time according to the format \ffmt\r.\n"
	"  \b-F\r            Appends '\b/\r', '\b=\r', '\b*\r', '\b@\r', '\b|\r' or '\b>\r' as per \bls -F\r.\n"
	"  \b--inodes\r      Print inode number of each file.\n"
	"  \b--device\r      Print device ID number to which each file belongs.\n");
  fancy(stdout,
	"  \b------- Sorting options -------\r\n"
	"  \b-v\r            Sort files alphanumerically by version.\n"
	"  \b-t\r            Sort files by last modification time.\n"
	"  \b-c\r            Sort files by last status change time.\n"
	"  \b-U\r            Leave files unsorted.\n"
	"  \b-r\r            Reverse the order of the sort.\n"
	"  \b--dirsfirst\r   List directories before files (\b-U\r disables).\n"
	"  \b--filesfirst\r  List files before directories (\b-U\r disables).\n"
	"  \b--sort\r \fX\r      Select sort: \b\fname\r,\b\fversion\r,\b\fsize\r,\b\fmtime\r,\b\fctime\r,\b\fnone\r.\n"
	"  \b------- Graphics options -------\r\n"
	"  \b-i\r            Don't print indentation lines.\n"
	"  \b-A\r            Print ANSI lines graphic indentation lines.\n"
	"  \b-S\r            Print with CP437 (console) graphics indentation lines.\n"
	"  \b-n\r            Turn colorization off always (\b-C\r overrides).\n"
	"  \b-C\r            Turn colorization on always.\n"
	"  \b------- XML/HTML/JSON/HYPERLINK options -------\r\n"
	"  \b-X\r            Prints out an XML representation of the tree.\n"
	"  \b-J\r            Prints out an JSON representation of the tree.\n"
	"  \b-H\r \fbaseHREF\r   Prints out HTML format with \fbaseHREF\r as top directory.\n"
	"  \b-T\r \fstring\r     Replace the default HTML title and H1 header with \fstring\r.\n"
	"  \b--nolinks\r     Turn off hyperlinks in HTML output.\n"
	"  \b--hintro\r \fX\r    Use file \fX\r as the HTML intro.\n"
	"  \b--houtro\r \fX\r    Use file \fX\r as the HTML outro.\n"
	"  \b--hyperlink\r   Turn on OSC 8 terminal hyperlinks.\n"
	"  \b--scheme\r \fX\r    Set OSC 8 hyperlink scheme, default \b\ffile://\r\n"
	"  \b--authority\r \fX\r Set OSC 8 hyperlink authority/hostname.\n"
	"  \b------- Input options -------\r\n"
	"  \b--fromfile\r    Reads paths from files (\b.\r=stdin)\n"
	"  \b--fromtabfile\r Reads trees from tab indented files (\b.\r=stdin)\n"
	"  \b--fflinks\r     Process link information when using \b--fromfile\r.\n"
	"  \b------- Miscellaneous options -------\r\n"
	"  \b--opt-toggle\r  Enable option toggling.\n"
	"  \b--version\r     Print version and exit.\n"
	"  \b--help\r        Print usage and this help message and exit.\n"
	"  \b--\r            Options processing terminator.\n");
  exit(0);
}

/**
 * True if file matches an -I pattern
 */
int patignore(const char *name, bool isdir)
{
  int i;
  for(i=0; i < ipattern; i++)
    if (patmatch(name, ipatterns[i], isdir)) return 1;
  return 0;
}

/**
 * True if name matches a -P pattern
 */
int patinclude(const char *name, bool isdir)
{
  int i;
  for(i=0; i < pattern; i++) {
    if (patmatch(name, patterns[i], isdir)) {
      return 1;
    }
  }
  return 0;
}

/**
 * Split out stat portion from read_dir as prelude to just using stat structure directly.
 */
struct _info *getinfo(const char *name, char *path)
{
  static char *lbuf = NULL;
  static size_t lbufsize = 0;
  struct _info *ent;
  struct stat st, lst;
  ssize_t len;
  int rs;
  bool isdir;

  if (lbuf == NULL) lbuf = xmalloc(lbufsize = PATH_MAX);

  if (lstat(path,&lst) < 0) return NULL;

  if ((lst.st_mode & S_IFMT) == S_IFLNK) {
    if ((rs = stat(path,&st)) < 0) memset(&st, 0, sizeof(st));
  } else {
    rs = 0;
    st.st_mode = lst.st_mode;
    st.st_dev = lst.st_dev;
    st.st_ino = lst.st_ino;
  }

  isdir = (st.st_mode & S_IFMT) == S_IFDIR;

#ifndef __EMX__
  if (gitignore && filtercheck(path, name, isdir)) return NULL;

  if ((lst.st_mode & S_IFMT) != S_IFDIR && !(lflag && ((st.st_mode & S_IFMT) == S_IFDIR))) {
    if (pattern && !patinclude(name, isdir)) return NULL;
  }
  if (ipattern && patignore(name, isdir)) return NULL;
#endif

  if (dflag && ((st.st_mode & S_IFMT) != S_IFDIR)) return NULL;

#ifndef __EMX__
/*    if (pattern && ((lst.st_mode & S_IFMT) == S_IFLNK) && !lflag) continue; */
#endif

  ent = (struct _info *)xmalloc(sizeof(struct _info));
  memset(ent, 0, sizeof(struct _info));

  ent->name = scopy(name);
  /* We should just incorporate struct stat into _info, and eliminate this unnecessary copying.
   * Made sense long ago when we had fewer options and didn't need half of stat.
   */
  ent->mode   = lst.st_mode;
  ent->uid    = lst.st_uid;
  ent->gid    = lst.st_gid;
  ent->size   = lst.st_size;
  ent->dev    = st.st_dev;
  ent->inode  = st.st_ino;
  ent->ldev   = lst.st_dev;
  ent->linode = lst.st_ino;
  ent->lnk    = NULL;
  ent->orphan = false;
  ent->err    = NULL;
  ent->child  = NULL;

  ent->atime  = lst.st_atime;
  ent->ctime  = lst.st_ctime;
  ent->mtime  = lst.st_mtime;

#ifdef __EMX__
  ent->attr   = lst.st_attr;
#else

  /* These should be eliminated, as they're barely used: */
  ent->isdir  = isdir;
  ent->issok  = ((st.st_mode & S_IFMT) == S_IFSOCK);
  ent->isfifo = ((st.st_mode & S_IFMT) == S_IFIFO);
  ent->isexe  = (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? 1 : 0;

  if ((lst.st_mode & S_IFMT) == S_IFLNK) {
    if ((size_t)lst.st_size+1 > lbufsize) lbuf = xrealloc(lbuf,lbufsize=((size_t)lst.st_size+8192));
    if ((len=readlink(path,lbuf,lbufsize-1)) < 0) {
      ent->lnk = scopy("[Error reading symbolic link information]");
      ent->isdir = false;
      ent->lnkmode = st.st_mode;
    } else {
      lbuf[len] = 0;
      ent->lnk = scopy(lbuf);
      if (rs < 0) ent->orphan = true;
      ent->lnkmode = st.st_mode;
    }
  }
#endif

  ent->comment = NULL;

  return ent;
}

struct _info **read_dir(char *dir, ssize_t *n, int infotop)
{
  struct comment *com;
  static char *path = NULL;
  static size_t pathsize;
  struct _info **dl, *info;
  struct dirent *ent;
  DIR *d;
  size_t ne, p = 0, i;
  bool es = (dir[strlen(dir)-1] == '/');

  if (path == NULL) {
    path=xmalloc(pathsize = strlen(dir)+PATH_MAX);
  }

  *n = -1;
  if ((d=opendir(dir)) == NULL) return NULL;

  dl = (struct _info **)xmalloc(sizeof(struct _info *) * (ne = MINIT));

  while((ent = (struct dirent *)readdir(d))) {
    if (!strcmp("..",ent->d_name) || !strcmp(".",ent->d_name)) continue;
    if (Hflag && !strcmp(ent->d_name,"00Tree.html")) continue;
    if (!aflag && ent->d_name[0] == '.') continue;

    if (strlen(dir)+strlen(ent->d_name)+2 > pathsize) path = xrealloc(path,pathsize=(strlen(dir)+strlen(ent->d_name)+PATH_MAX));
    if (es) sprintf(path, "%s%s", dir, ent->d_name);
    else sprintf(path,"%s/%s",dir,ent->d_name);

    info = getinfo(ent->d_name, path);
    if (info) {
      if (showinfo && (com = infocheck(path, ent->d_name, infotop, info->isdir))) {
	for(i = 0; com->desc[i] != NULL; i++);
	info->comment = xmalloc(sizeof(char *) * (i+1));
	for(i = 0; com->desc[i] != NULL; i++) info->comment[i] = scopy(com->desc[i]);
	info->comment[i] = NULL;
      }
      if (p == (ne-1)) dl = (struct _info **)xrealloc(dl,sizeof(struct _info *) * (ne += MINC));
      dl[p++] = info;
    }
  }
  closedir(d);

  if ((*n = (ssize_t)p) == 0) {
    free(dl);
    return NULL;
  }

  dl[p] = NULL;
  return dl;
}

void push_files(const char *dir, struct ignorefile **ig, struct infofile **inf, bool top)
{
  if (gitignore) {
    *ig = new_ignorefile(dir, top);
    if (*ig != NULL) push_filterstack(*ig);
  }
  if (showinfo) {
    *inf = new_infofile(dir, top);
    if (*inf != NULL) push_infostack(*inf);
  }
}

/* This is for all the impossible things people wanted the old tree to do.
 * This can and will use a large amount of memory for large directory trees
 * and also take some time.
 */
struct _info **unix_getfulltree(char *d, u_long lev, dev_t dev, off_t *size, char **err)
{
  char *path;
  size_t pathsize = 0;
  struct ignorefile *ig = NULL;
  struct infofile *inf = NULL;
  struct _info **dir, **sav, **p, *xp;
  struct stat sb;
  ssize_t n;
  u_long lev_tmp;
  int tmp_pattern = 0;
  char *start_rel_path;

  *err = NULL;
  if (Level >= 0 && lev > (u_long)Level) return NULL;
  if (xdev && lev == 0) {
    stat(d,&sb);
    dev = sb.st_dev;
  }
  /* if the directory name matches, turn off pattern matching for contents */
  if (matchdirs && pattern) {
    lev_tmp = lev;
    start_rel_path = d + strlen(d);
    for (start_rel_path = d + strlen(d); start_rel_path != d; --start_rel_path) {
      if (*start_rel_path == '/')
        --lev_tmp;
      if (lev_tmp <= 0) {
        if (*start_rel_path)
          ++start_rel_path;
        break;
      }
    }
    if (*start_rel_path && patinclude(start_rel_path, 1)) {
      tmp_pattern = pattern;
      pattern = 0;
    }
  }

  push_files(d, &ig, &inf, lev==0);

  sav = dir = read_dir(d, &n, inf != NULL);
  if (tmp_pattern) {
    pattern = tmp_pattern;
    tmp_pattern = 0;
  }
  if (dir == NULL && n) {
    *err = scopy("error opening dir");
    return NULL;
  }
  if (n == 0) {
    if (sav != NULL) free_dir(sav);
    return NULL;
  }
  path = xmalloc(pathsize=PATH_MAX);
  
  if (flimit > 0 && n > flimit) {
    sprintf(path,"%ld entries exceeds filelimit, not opening dir",n);
    *err = scopy(path);
    free_dir(sav);
    free(path);
    return NULL;
  }

  if (lev >= (u_long)maxdirs-1) {
    dirs = xrealloc(dirs,sizeof(int) * (maxdirs += 1024));
  }

  while (*dir) {
    if ((*dir)->isdir && !(xdev && dev != (*dir)->dev)) {
      if ((*dir)->lnk) {
	if (lflag) {
	  if (findino((*dir)->inode,(*dir)->dev)) {
	    (*dir)->err = scopy("recursive, not followed");
	  } else {
	    saveino((*dir)->inode, (*dir)->dev);
	    if (*(*dir)->lnk == '/')
	      (*dir)->child = unix_getfulltree((*dir)->lnk,lev+1,dev,&((*dir)->size),&((*dir)->err));
	    else {
	      if (strlen(d)+strlen((*dir)->lnk)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	      if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->lnk);
	      else sprintf(path,"%s/%s",d,(*dir)->lnk);
	      (*dir)->child = unix_getfulltree(path,lev+1,dev,&((*dir)->size),&((*dir)->err));
	    }
	  }
	}
      } else {
	if (strlen(d)+strlen((*dir)->name)+2 > pathsize) path=xrealloc(path,pathsize=(strlen(d)+strlen((*dir)->name)+1024));
	if (fflag && !strcmp(d,"/")) sprintf(path,"%s%s",d,(*dir)->name);
	else sprintf(path,"%s/%s",d,(*dir)->name);
	saveino((*dir)->inode, (*dir)->dev);
	(*dir)->child = unix_getfulltree(path,lev+1,dev,&((*dir)->size),&((*dir)->err));
      }
      /* prune empty folders, unless they match the requested pattern */
      if (pruneflag && (*dir)->child == NULL &&
	  !(matchdirs && pattern && patinclude((*dir)->name, (*dir)->isdir))) {
	xp = *dir;
	for(p=dir;*p;p++) *p = *(p+1);
	n--;
	free(xp->name);
	if (xp->lnk) free(xp->lnk);
	free(xp);
	continue;
      }
    }
    if (duflag) *size += (*dir)->size;
    dir++;
  }

  /* sorting needs to be deferred for --du: */
  if (topsort) qsort(sav,(size_t)n,sizeof(struct _info *), (int (*)(const void *, const void *))topsort);

  free(path);
  if (n == 0) {
    free_dir(sav);
    return NULL;
  }
  if (ig != NULL) pop_filterstack();
  if (inf != NULL) pop_infostack();
  return sav;
}

/**
 * filesfirst and dirsfirst are now top-level meta-sorts.
 */
int filesfirst(struct _info **a, struct _info **b)
{
  if ((*a)->isdir != (*b)->isdir) {
    return (*a)->isdir ? 1 : -1;
  }
  return basesort(a, b);
}

int dirsfirst(struct _info **a, struct _info **b)
{
  if ((*a)->isdir != (*b)->isdir) {
    return (*a)->isdir ? -1 : 1;
  }
  return basesort(a, b);
}

/* Sorting functions */
int alnumsort(struct _info **a, struct _info **b)
{
  int v = strcoll((*a)->name,(*b)->name);
  return reverse? -v : v;
}

int versort(struct _info **a, struct _info **b)
{
  int v = strverscmp((*a)->name,(*b)->name);
  return reverse? -v : v;
}

int mtimesort(struct _info **a, struct _info **b)
{
  int v;

  if ((*a)->mtime == (*b)->mtime) {
    v = strcoll((*a)->name,(*b)->name);
    return reverse? -v : v;
  }
  v =  (*a)->mtime == (*b)->mtime? 0 : ((*a)->mtime < (*b)->mtime ? -1 : 1);
  return reverse? -v : v;
}

int ctimesort(struct _info **a, struct _info **b)
{
  int v;

  if ((*a)->ctime == (*b)->ctime) {
    v = strcoll((*a)->name,(*b)->name);
    return reverse? -v : v;
  }
  v = (*a)->ctime == (*b)->ctime? 0 : ((*a)->ctime < (*b)->ctime? -1 : 1);
  return reverse? -v : v;
}

int sizecmp(off_t a, off_t b)
{
  return (a == b)? 0 : ((a < b)? 1 : -1);
}

int fsizesort(struct _info **a, struct _info **b)
{
  int v = sizecmp((*a)->size, (*b)->size);
  if (v == 0) v = strcoll((*a)->name,(*b)->name);
  return reverse? -v : v;
}

void *xmalloc (size_t size)
{
  register void *value = malloc (size);
  if (value == NULL) {
    fprintf(stderr,"tree: virtual memory exhausted.\n");
    exit(1);
  }
  return value;
}

void *xrealloc (void *ptr, size_t size)
{
  register void *value = realloc (ptr,size);
  if (value == NULL) {
    fprintf(stderr,"tree: virtual memory exhausted.\n");
    exit(1);
  }
  return value;
}

void free_dir(struct _info **d)
{
  int i;

  for(i=0;d[i];i++) {
    free(d[i]->name);
    if (d[i]->lnk) free(d[i]->lnk);
    free(d[i]);
  }
  free(d);
}

char *gnu_getcwd(void)
{
  size_t size = 100;
  char *buffer = (char *) xmalloc (size);

  while (1) {
    char *value = getcwd (buffer, size);
    if (value != 0) return buffer;
    size *= 2;
    free (buffer);
    buffer = (char *) xmalloc (size);
  }
}

static char cond_lower(char c)
{
  return ignorecase ? (char)tolower(c) : c;
}

/*
 * Patmatch() code courtesy of Thomas Moore (dark@mama.indstate.edu)
 * '|' support added by David MacMahon (davidm@astron.Berkeley.EDU)
 * Case insensitive support added by Jason A. Donenfeld (Jason@zx2c4.com)
 * returns:
 *    1 on a match
 *    0 on a mismatch
 *   -1 on a syntax error in the pattern
 */
int patmatch(const char *buf, const char *pat, bool isdir)
{
  int match = 1, n;
  char *bar = strchr(pat, '|');
  char m, pprev = 0;

  /* If a bar is found, call patmatch recursively on the two sub-patterns */
  if (bar) {
    /* If the bar is the first or last character, it's a syntax error */
    if (bar == pat || !bar[1]) {
      return -1;
    }
    /* Break pattern into two sub-patterns */
    *bar = '\0';
    match = patmatch(buf, pat, isdir);
    if (!match) {
      match = patmatch(buf, bar+1, isdir);
    }
    /* Join sub-patterns back into one pattern */
    *bar = '|';
    return match;
  }

  while(*pat && match) {
    switch(*pat) {
    case '[':
      pat++;
      if(*pat != '^') {
	n = 1;
	match = 0;
      } else {
	pat++;
	n = 0;
      }
      while(*pat != ']'){
	if(*pat == '\\') pat++;
	if(!*pat /* || *pat == '/' */ ) return -1;
	if(pat[1] == '-'){
	  m = *pat;
	  pat += 2;
	  if(*pat == '\\' && *pat)
	    pat++;
	  if(cond_lower(*buf) >= cond_lower(m) && cond_lower(*buf) <= cond_lower(*pat))
	    match = n;
	  if(!*pat)
	    pat--;
	} else if(cond_lower(*buf) == cond_lower(*pat)) match = n;
	pat++;
      }
      buf++;
      break;
    case '*':
      pat++;
      if(!*pat) {
	int f = (strchr(buf, '/') == NULL);
        return f;
      }
      match = 0;
      /* "Support" ** for .gitignore support, mostly the same as *: */
      if (*pat == '*') {
	pat++;
	if(!*pat) return 1;

	while(*buf && !(match = patmatch(buf, pat, isdir))) {
	  /* ** between two /'s is allowed to match a null /: */
	  if (pprev == '/' && *pat == '/' && *(pat+1) && (match = patmatch(buf, pat+1, isdir))) return match;
	  buf++;
	  while(*buf && *buf != '/') buf++;
	}
      } else {
	while(*buf && !(match = patmatch(buf++, pat, isdir)))
	  if (*buf == '/') break;
      }
      if (!match && (!*buf || *buf == '/')) match = patmatch(buf, pat, isdir);
      return match;
    case '?':
      if(!*buf) return 0;
      buf++;
      break;
    case '/':
      if (!*(pat+1) && !*buf) return isdir;
      match = (*buf++ == *pat);
      break;
    case '\\':
      if(*pat)
	pat++;
      /* Falls through */
    default:
      match = (cond_lower(*buf++) == cond_lower(*pat));
      break;
    }
    pprev = *pat++;
    if(match<1) return match;
  }
  if(!*buf) return match;
  return 0;
}


/**
 * They cried out for ANSI-lines (not really), but here they are, as an option
 * for the xterm and console capable among you, as a run-time option.
 */
void indent(int maxlevel)
{
  int i;

  if (ansilines) {
    if (dirs[1]) fprintf(outfile,"\033(0");
    for(i=1; (i <= maxlevel) && dirs[i]; i++) {
      if (dirs[i+1]) {
	if (dirs[i] == 1) fprintf(outfile,"\170   ");
	else printf("    ");
      } else {
	if (dirs[i] == 1) fprintf(outfile,"\164\161\161 ");
	else fprintf(outfile,"\155\161\161 ");
      }
    }
    if (dirs[1]) fprintf(outfile,"\033(B");
  } else {
    if (Hflag) fprintf(outfile,"\t");
    for(i=1; (i <= maxlevel) && dirs[i]; i++) {
      fprintf(outfile,"%s ",
	      dirs[i+1] ? (dirs[i]==1 ? linedraw->vert     : (Hflag? "&nbsp;&nbsp;&nbsp;" : "   ") )
			: (dirs[i]==1 ? linedraw->vert_left:linedraw->corner));
    }
  }
}


#ifdef __EMX__
char *prot(long m)
#else
char *prot(mode_t m)
#endif
{
#ifdef __EMX__
  const u_short *p;
  static char buf[6];
  char*cp;

  for(p=ifmt,cp=strcpy(buf,"adshr");*cp;++p,++cp)
    if(!(m&*p))
      *cp='-';
#else
  static char buf[11], perms[] = "rwxrwxrwx";
  int i;
  mode_t b;

  for(i=0;ifmt[i] && (m&S_IFMT) != ifmt[i];i++);
  buf[0] = fmt[i];

  /**
   * Nice, but maybe not so portable, it is should be no less portable than the
   * old code.
   */
  for(b=S_IRUSR,i=0; i<9; b>>=1,i++)
    buf[i+1] = (m & (b)) ? perms[i] : '-';
  if (m & S_ISUID) buf[3] = (buf[3]=='-')? 'S' : 's';
  if (m & S_ISGID) buf[6] = (buf[6]=='-')? 'S' : 's';
  if (m & S_ISVTX) buf[9] = (buf[9]=='-')? 'T' : 't';

  buf[10] = 0;
#endif
  return buf;
}

#define SIXMONTHS (6*31*24*60*60)

char *do_date(time_t t)
{
  static char buf[256];
  struct tm *tm;

  tm = localtime(&t);

  if (timefmt) {
    strftime(buf,255,timefmt,tm);
    buf[255] = 0;
  } else {
    time_t c = time(0);
    /* Use strftime() so that locale is respected: */
    if (t > c || (t+SIXMONTHS) < c)
      strftime(buf,255,"%b %e  %Y",tm);
    else
      strftime(buf,255,"%b %e %R", tm);
  }
  return buf;
}

/**
 * Must fix this someday
 */
void printit(const char *s)
{
  int c;
  size_t cs;

  if (Nflag) {
    if (Qflag) fprintf(outfile, "\"%s\"",s);
    else fprintf(outfile,"%s",s);
    return;
  }
  if (mb_cur_max > 1) {
    wchar_t *ws, *tp;
    ws = xmalloc(sizeof(wchar_t)* (cs=(strlen(s)+1)));
    if (mbstowcs(ws,s,cs) != (size_t)-1) {
      if (Qflag) putc('"',outfile);
      for(tp=ws;*tp && cs > 1;tp++, cs--) {
	if (iswprint((wint_t)*tp)) fprintf(outfile,"%lc",(wint_t)*tp);
	else {
	  if (qflag) putc('?',outfile);
	  else fprintf(outfile,"\\%03o",(unsigned int)*tp);
	}
      }
      if (Qflag) putc('"',outfile);
      free(ws);
      return;
    }
    free(ws);
  }
  if (Qflag) putc('"',outfile);
  for(;*s;s++) {
    c = (unsigned char)*s;
#ifdef __EMX__
    if(_nls_is_dbcs_lead(*(unsigned char*)s)){
      putc(*s,outfile);
      putc(*++s,outfile);
      continue;
    }
#endif
    if((c >= 7 && c <= 13) || c == '\\' || (c == '"' && Qflag) || (c == ' ' && !Qflag)) {
      putc('\\',outfile);
      if (c > 13) putc(c, outfile);
      else putc("abtnvfr"[c-7], outfile);
    } else if (isprint(c)) putc(c,outfile);
    else {
      if (qflag) {
	if (mb_cur_max > 1 && c > 127) putc(c,outfile);
	else putc('?',outfile);
      } else fprintf(outfile,"\\%03o",c);
    }
  }
  if (Qflag) putc('"',outfile);
}

int psize(char *buf, off_t size)
{
  static char *iec_unit="BKMGTPEZY", *si_unit = "dkMGTPEZY";
  char *unit = siflag ? si_unit : iec_unit;
  int idx, usize = siflag ? 1000 : 1024;

  if (hflag || siflag) {
    for (idx=size<usize?0:1; size >= (usize*usize); idx++,size/=usize);
    if (!idx) return sprintf(buf, " %4d", (int)size);
    else return sprintf(buf, (((size+52)/usize) >= 10)? " %3.0f%c" : " %3.1f%c" , (float)size/(float)usize,unit[idx]);
  } else return sprintf(buf, sizeof(off_t) == sizeof(long long)? " %11lld" : " %9lld", (long long int)size);
}

char Ftype(mode_t mode)
{
  int m = mode & S_IFMT;
  if (!dflag && m == S_IFDIR) return '/';
  else if (m == S_IFSOCK) return '=';
  else if (m == S_IFIFO) return '|';
  else if (m == S_IFLNK) return '@'; /* Here, but never actually used though. */
#ifdef S_IFDOOR
  else if (m == S_IFDOOR) return '>';
#endif
  else if ((m == S_IFREG) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) return '*';
  return 0;
}

struct _info *stat2info(const struct stat *st)
{
  static struct _info info;

  info.linode = st->st_ino;
  info.ldev = st->st_dev;
#ifdef __EMX__
  info.attr = st->st_attr
#endif
  info.mode = st->st_mode;
  info.uid = st->st_uid;
  info.gid = st->st_gid;
  info.size = st->st_size;
  info.atime = st->st_atime;
  info.ctime = st->st_ctime;
  info.mtime = st->st_mtime;

  info.isdir  = ((st->st_mode & S_IFMT) == S_IFDIR);
  info.issok  = ((st->st_mode & S_IFMT) == S_IFSOCK);
  info.isfifo = ((st->st_mode & S_IFMT) == S_IFIFO);
  info.isexe  = (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? 1 : 0;

  return &info;
}

char *fillinfo(char *buf, const struct _info *ent)
{
  int n;
  buf[n=0] = 0;
  #ifdef __USE_FILE_OFFSET64
  if (inodeflag) n += sprintf(buf," %7lld",(long long)ent->linode);
  #else
  if (inodeflag) n += sprintf(buf," %7ld",(long int)ent->linode);
  #endif
  if (devflag) n += sprintf(buf+n, " %3d", (int)ent->ldev);
  #ifdef __EMX__
  if (pflag) n += sprintf(buf+n, " %s",prot(ent->attr));
  #else
  if (pflag) n += sprintf(buf+n, " %s", prot(ent->mode));
  #endif
  if (uflag) n += sprintf(buf+n, " %-8.32s", uidtoname(ent->uid));
  if (gflag) n += sprintf(buf+n, " %-8.32s", gidtoname(ent->gid));
  if (sflag) n += psize(buf+n,ent->size);
  if (Dflag) n += sprintf(buf+n, " %s", do_date(cflag? ent->ctime : ent->mtime));

  if (buf[0] == ' ') {
      buf[0] = '[';
      sprintf(buf+n, "]");
  }

  return buf;
}

```

`tree-mac/unix.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"

extern FILE *outfile;
extern bool dflag, Fflag, duflag, metafirst, hflag, siflag, noindent;
extern bool colorize, linktargetcolor, hyperflag;
extern const struct linedraw *linedraw;
extern int *dirs;
extern char *scheme, *authority;
static char info[512] = {0};

extern char realbasepath[PATH_MAX], xpattern[PATH_MAX];
extern size_t dirpathoffset;

int unix_printinfo(char *dirname, struct _info *file, int level)
{
  UNUSED(dirname);

  fillinfo(info, file);
  if (metafirst) {
    if (info[0] == '[') fprintf(outfile, "%s  ",info);
    if (!noindent) indent(level);
  } else {
    if (!noindent) indent(level);
    if (info[0] == '[') fprintf(outfile, "%s  ",info);
  }
  return 0;
}

void open_hyperlink(char *dirname, char *filename)
{
  fprintf(outfile,"\033]8;;%s", scheme);
  url_encode(outfile, authority);
  url_encode(outfile, realbasepath);
  url_encode(outfile, dirname+dirpathoffset);
  fputc('/',outfile);
  url_encode(outfile, filename);
  fprintf(outfile,"\033\\");
}

void close_hyperlink(void)
{
  fprintf(outfile, "\033]8;;\033\\");
}

int unix_printfile(char *dirname, char *filename, struct _info *file, int descend)
{
  UNUSED(descend);

  bool colored = false;
  int c;

  if (hyperflag) open_hyperlink(dirname, file->name);

  if (file && colorize) {
    if (file->lnk && linktargetcolor) colored = color(file->lnkmode, file->name, file->orphan, false);
    else colored = color(file->mode, file->name, file->orphan, false);
  }

  printit(filename);

  if (colored) endcolor();

  if (hyperflag) close_hyperlink();

  if (file) {
    if (Fflag && !file->lnk) {
      if ((c = Ftype(file->mode))) fputc(c, outfile);
    }

    if (file->lnk) {
      fprintf(outfile," -> ");
      if (hyperflag) open_hyperlink(dirname, file->name);
      if (colorize) colored = color(file->lnkmode, file->lnk, file->orphan, true);
      printit(file->lnk);
      if (colored) endcolor();
      if (hyperflag) close_hyperlink();
      if (Fflag) {
	if ((c = Ftype(file->lnkmode))) fputc(c, outfile);
      }
    }
  }
  return 0;
}

int unix_error(char *error)
{
  fprintf(outfile, "  [%s]", error);
  return 0;
}

void unix_newline(struct _info *file, int level, int postdir, int needcomma)
{
  UNUSED(needcomma);

  if (postdir <= 0) fprintf(outfile, "\n");
  if (file && file->comment) {
    size_t infosize = 0, line, lines;
    if (metafirst) infosize = info[0] == '['? strlen(info)+2 : 0;

    for(lines = 0; file->comment[lines]; lines++);
    dirs[level+1] = 1;
    for(line = 0; line < lines; line++) {
      if (metafirst) {
	printf("%*s", (int)infosize, "");
      }
      indent(level);
      printcomment(line, lines, file->comment[line]);
    }
    dirs[level+1] = 0;
  }
}

void unix_report(struct totals tot)
{
  char buf[256];

  fputc('\n', outfile);
  if (duflag) {
    psize(buf, tot.size);
    fprintf(outfile,"%s%s used in ", buf, hflag || siflag? "" : " bytes");
  }
  if (dflag)
    fprintf(outfile,"%ld director%s\n",tot.dirs,(tot.dirs==1? "y":"ies"));
  else
    fprintf(outfile,"%ld director%s, %ld file%s\n",tot.dirs,(tot.dirs==1? "y":"ies"),tot.files,(tot.files==1? "":"s"));
}

```

`tree-mac/xml.c`:

```c
/* $Copyright: $
 * Copyright (c) 1996 - 2024 by Steve Baker (steve.baker.llc@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "tree.h"


extern bool dflag, pflag, sflag, uflag, gflag;
extern bool Dflag, inodeflag, devflag, cflag, duflag;
extern bool noindent;

extern const char *charset;
extern const mode_t ifmt[];
extern const char *ftype[];

extern FILE *outfile;

/*
<tree>
  <directory name="name" mode=0777 size=### user="user" group="group" inode=### dev=### time="00:00 00-00-0000">
    <link name="name" target="name" ...>
      ... if link is followed, otherwise this is empty.
    </link>
    <file name="name" mode=0777 size=### user="user" group="group" inode=### dev=### time="00:00 00-00-0000"></file>
    <socket name="" ...><error>some error</error></socket>
    <block name="" ...></block>
    <char name="" ...></char>
    <fifo name="" ...></fifo>
    <door name="" ...></door>
    <port name="" ...></port>
    ...
  </directory>
  <report>
    <size>#</size>
    <files>#</files>
    <directories>#</directories>
  </report>
</tree>
*/

void xml_indent(int maxlevel)
{
  int i;
  
  fprintf(outfile, "  ");
  for(i=0; i<maxlevel; i++)
    fprintf(outfile, "  ");
}

void xml_fillinfo(struct _info *ent)
{
  #ifdef __USE_FILE_OFFSET64
  if (inodeflag) fprintf(outfile," inode=\"%lld\"",(long long)ent->inode);
  #else
  if (inodeflag) fprintf(outfile," inode=\"%ld\"",(long int)ent->inode);
  #endif
  if (devflag) fprintf(outfile, " dev=\"%d\"", (int)ent->dev);
  #ifdef __EMX__
  if (pflag) fprintf(outfile, " mode=\"%04o\" prot=\"%s\"",ent->attr, prot(ent->attr));
  #else
  if (pflag) fprintf(outfile, " mode=\"%04o\" prot=\"%s\"", ent->mode & (S_IRWXU|S_IRWXG|S_IRWXO|S_ISUID|S_ISGID|S_ISVTX), prot(ent->mode));
  #endif
  if (uflag) fprintf(outfile, " user=\"%s\"", uidtoname(ent->uid));
  if (gflag) fprintf(outfile, " group=\"%s\"", gidtoname(ent->gid));
  if (sflag) fprintf(outfile, " size=\"%lld\"", (long long int)(ent->size));
  if (Dflag) fprintf(outfile, " time=\"%s\"", do_date(cflag? ent->ctime : ent->mtime));
}

void xml_intro(void)
{
  extern char *_nl;

  fprintf(outfile,"<?xml version=\"1.0\"");
  if (charset) fprintf(outfile," encoding=\"%s\"",charset);
  fprintf(outfile,"?>%s<tree>%s",_nl,_nl);
}

void xml_outtro(void)
{
  fprintf(outfile,"</tree>\n");
}

int xml_printinfo(char *dirname, struct _info *file, int level)
{
  UNUSED(dirname);

  mode_t mt;
  int t;

  if (!noindent) xml_indent(level);

  if (file != NULL) {
    if (file->lnk) mt = file->mode & S_IFMT;
    else mt = file->mode & S_IFMT;
  } else mt = 0;

  for(t=0;ifmt[t];t++)
    if (ifmt[t] == mt) break;
  fprintf(outfile,"<%s", (file->tag = ftype[t]));

  return 0;
}

int xml_printfile(char *dirname, char *filename, struct _info *file, int descend)
{
  UNUSED(dirname);UNUSED(descend);

  int i;

  fprintf(outfile, " name=\"");
  html_encode(outfile, filename);
  fputc('"',outfile);

  if (file && file->comment) {
    fprintf(outfile, " info=\"");
    for(i=0; file->comment[i]; i++) {
      html_encode(outfile, file->comment[i]);
      if (file->comment[i+1]) fprintf(outfile, "\n");
    }
    fputc('"', outfile);
  }

  if (file && file->lnk) {
    fprintf(outfile, " target=\"");
    html_encode(outfile,file->lnk);
    fputc('"',outfile);
  }
  if (file) xml_fillinfo(file);
  fputc('>',outfile);

  return 1;
}

int xml_error(char *error)
{
  fprintf(outfile,"<error>%s</error>", error);

  return 0;
}

void xml_newline(struct _info *file, int level, int postdir, int needcomma)
{
  UNUSED(file);UNUSED(level);UNUSED(needcomma);

  if (postdir >= 0) fprintf(outfile, "\n");
}

void xml_close(struct _info *file, int level, int needcomma)
{
  UNUSED(needcomma);

  if (!noindent && level >= 0) xml_indent(level);
  fprintf(outfile,"</%s>%s", file? file->tag : "unknown", noindent? "" : "\n");
}


void xml_report(struct totals tot)
{
  extern char *_nl;

  fprintf(outfile,"%s<report>%s",noindent?"":"  ", _nl);
  if (duflag) fprintf(outfile,"%s<size>%lld</size>%s", noindent?"":"    ", (long long int)tot.size, _nl);
  fprintf(outfile,"%s<directories>%ld</directories>%s", noindent?"":"    ", tot.dirs, _nl);
  if (!dflag) fprintf(outfile,"%s<files>%ld</files>%s", noindent?"":"    ", tot.files, _nl);
  fprintf(outfile,"%s</report>%s",noindent?"":"  ", _nl);
}

```