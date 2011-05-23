/* a.out object file format
   Copyright (C) 1989, 90, 91, 92, 93, 94, 95, 1996
   Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2,
or (at your option) any later version.

GAS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with GAS; see the file COPYING.  If not, write
to the Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#include "as.h"
#ifdef BFD_ASSEMBLER
#undef NO_RELOC
#include "aout/aout64.h"
#endif
#include "obstack.h"

#ifndef BFD_ASSEMBLER
/* in: segT   out: N_TYPE bits */
const short seg_N_TYPE[] =
{
  N_ABS,
  N_TEXT,
  N_DATA,
  N_BSS,
  N_UNDF,			/* unknown */
  N_UNDF,			/* error */
  N_UNDF,			/* expression */
  N_UNDF,			/* debug */
  N_UNDF,			/* ntv */
  N_UNDF,			/* ptv */
  N_REGISTER,			/* register */
};

const segT N_TYPE_seg[N_TYPE + 2] =
{				/* N_TYPE == 0x1E = 32-2 */
  SEG_UNKNOWN,			/* N_UNDF == 0 */
  SEG_GOOF,
  SEG_ABSOLUTE,			/* N_ABS == 2 */
  SEG_GOOF,
  SEG_TEXT,			/* N_TEXT == 4 */
  SEG_GOOF,
  SEG_DATA,			/* N_DATA == 6 */
  SEG_GOOF,
  SEG_BSS,			/* N_BSS == 8 */
  SEG_GOOF,
  SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
  SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
  SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
  SEG_REGISTER,			/* dummy N_REGISTER for regs = 30 */
  SEG_GOOF,
};
#endif

static void obj_aout_line PARAMS ((int));
static void obj_aout_weak PARAMS ((int));
static void obj_aout_type PARAMS ((int));
static void obj_aout_size PARAMS ((int));

int aout_pic_flag = 0;

const pseudo_typeS obj_pseudo_table[] =
{
  {"line", obj_aout_line, 0},	/* source code line number */
  {"ln", obj_aout_line, 0},	/* coff line number that we use anyway */

  {"weak", obj_aout_weak, 0},	/* mark symbol as weak.  */

  {"type", obj_aout_type, 0},

  /* coff debug pseudos (ignored) */
  {"def", s_ignore, 0},
  {"dim", s_ignore, 0},
  {"endef", s_ignore, 0},
  {"ident", s_ignore, 0},
  {"line", s_ignore, 0},
  {"ln", s_ignore, 0},
  {"scl", s_ignore, 0},
  {"size", obj_aout_size, 0},
  {"tag", s_ignore, 0},
  {"val", s_ignore, 0},
  {"version", s_ignore, 0},

  {"optim", s_ignore, 0},	/* For sun386i cc (?) */

  /* other stuff */
  {"ABORT", s_abort, 0},

  {NULL}			/* end sentinel */
};				/* obj_pseudo_table */


#ifdef BFD_ASSEMBLER

/* Do NetBSD specific things to the symbols. This includes adding SIZE
 * symbols and making weak symbols global (The MI code clears global
 * flags when setting weak symbols.) */
void
obj_aout_nbsd_frob_file()
{
  /* We don't generate N_SIZE symbols unless we are working with PIC code. 
   * (and the non global weak symbol is only troublesome for PIC code) */
  if (!aout_pic_flag)
    return;

  if (symbol_rootP)
    {
      symbolS *sym;

      for (sym = symbol_rootP; sym; sym = symbol_next (sym))
	{
	  int type, other;

	  if (S_IS_WEAK (sym))
	    {
	      /* All weak symbols are global. */
	      sym->bsym->flags |= BSF_GLOBAL;
	    }

	  type = S_GET_TYPE (sym);
	  other = S_GET_OTHER (sym);

	  /* We do only add SIZE symbols for objects (i.e other == 1)
	     that have a .size directive. */
	  if ((type != N_SIZE)
	      && (type != (N_SIZE | N_EXT)) 
	      && (other == 1)
	      && ((expressionS*)sym->sy_sizexp != NULL))
	    {
	      expressionS *exp = (expressionS*)sym->sy_sizexp;
	      symbolS *new_sym;
	      int new_type;
	      long size;

	      new_type = N_SIZE;

	      if (type && N_EXT)
		new_type |= N_EXT;

	      new_sym = symbol_make(S_GET_NAME(sym));
	      S_SET_TYPE(new_sym, new_type);
	      new_sym->bsym->section = bfd_abs_section_ptr;

	      /* N_SIZE symbols cannot be weak. */
	      new_sym->bsym->flags = sym->bsym->flags & ~BSF_WEAK;

	      size = exp->X_add_number;

	      S_SET_VALUE(new_sym,size);
	    }
	}
    }
}

void
obj_aout_frob_symbol (sym, punt)
     symbolS *sym;
     int *punt;
{
  flagword flags;
  asection *sec;
  int desc, type, other;

  flags = sym->bsym->flags;
  desc = S_GET_DESC (sym);
  type = S_GET_TYPE (sym);
  other = S_GET_OTHER (sym);
  sec = sym->bsym->section;

  /* Only frob simple symbols this way right now.  */
  if (! (type & ~ (N_TYPE | N_EXT)))
    {
      if (type == (N_UNDF | N_EXT)
	  && sec == &bfd_abs_section)
	sym->bsym->section = sec = bfd_und_section_ptr;

      if ((type & N_TYPE) != N_INDR
	  && (type & N_TYPE) != N_SETA
	  && (type & N_TYPE) != N_SETT
	  && (type & N_TYPE) != N_SETD
	  && (type & N_TYPE) != N_SETB
	  && type != N_WARNING
	  && (sec == &bfd_abs_section
	      || sec == &bfd_und_section))
	return;
      if (flags & BSF_EXPORT)
	type |= N_EXT;

      switch (type & N_TYPE)
	{
	case N_SETA:
	case N_SETT:
	case N_SETD:
	case N_SETB:
	  /* Set the debugging flag for constructor symbols so that
	     BFD leaves them alone.  */
	  sym->bsym->flags |= BSF_DEBUGGING;

	  /* You can't put a common symbol in a set.  The way a set
	     element works is that the symbol has a definition and a
	     name, and the linker adds the definition to the set of
	     that name.  That does not work for a common symbol,
	     because the linker can't tell which common symbol the
	     user means.  FIXME: Using as_bad here may be
	     inappropriate, since the user may want to force a
	     particular type without regard to the semantics of sets;
	     on the other hand, we certainly don't want anybody to be
	     mislead into thinking that their code will work.  */
	  if (S_IS_COMMON (sym))
	    as_bad ("Attempt to put a common symbol into set %s",
		    S_GET_NAME (sym));
	  /* Similarly, you can't put an undefined symbol in a set.  */
	  else if (! S_IS_DEFINED (sym))
	    as_bad ("Attempt to put an undefined symbol into set %s",
		    S_GET_NAME (sym));

	  break;
	case N_INDR:
	  /* Put indirect symbols in the indirect section.  */
	  sym->bsym->section = bfd_ind_section_ptr;
	  sym->bsym->flags |= BSF_INDIRECT;
	  if (type & N_EXT)
	    {
	      sym->bsym->flags |= BSF_EXPORT;
	      sym->bsym->flags &=~ BSF_LOCAL;
	    }
	  break;
	case N_WARNING:
	  /* Mark warning symbols.  */
	  sym->bsym->flags |= BSF_WARNING;
	  break;
	}
    }
  else
    {
      sym->bsym->flags |= BSF_DEBUGGING;
    }

  S_SET_TYPE (sym, type);

  /* Double check weak symbols.  */
  if (sym->bsym->flags & BSF_WEAK)
    {
      if (S_IS_COMMON (sym))
	as_bad ("Symbol `%s' can not be both weak and common",
		S_GET_NAME (sym));
    }
}

void
obj_aout_frob_file ()
{
  /* Relocation processing may require knowing the VMAs of the sections.
     Since writing to a section will cause the BFD back end to compute the
     VMAs, fake it out here....  */
  bfd_byte b = 0;
  boolean x = true;
  if (bfd_section_size (stdoutput, text_section) != 0)
    {
      x = bfd_set_section_contents (stdoutput, text_section, &b, (file_ptr) 0,
				    (bfd_size_type) 1);
    }
  else if (bfd_section_size (stdoutput, data_section) != 0)
    {
      x = bfd_set_section_contents (stdoutput, data_section, &b, (file_ptr) 0,
				    (bfd_size_type) 1);
    }
  assert (x == true);

  /* Some archetectures has a 'pic' flag in their a.out header. I'm not
     sure this is the right place to set it, but this is the best hook
     I have found... */
  if (aout_pic_flag)
    {
      stdoutput->flags = BFD_PIC;
    }
}

#else

/* Relocation. */

/*
 *		emit_relocations()
 *
 * Crawl along a fixS chain. Emit the segment's relocations.
 */
void
obj_emit_relocations (where, fixP, segment_address_in_file)
     char **where;
     fixS *fixP;		/* Fixup chain for this segment. */
     relax_addressT segment_address_in_file;
{
  for (; fixP; fixP = fixP->fx_next)
    if (fixP->fx_done == 0)
      {
	symbolS *sym;

	sym = fixP->fx_addsy;
	while (sym->sy_value.X_op == O_symbol
	       && (! S_IS_DEFINED (sym) || S_IS_COMMON (sym)))
	  sym = sym->sy_value.X_add_symbol;
	fixP->fx_addsy = sym;

	if (! sym->sy_resolved && ! S_IS_DEFINED (sym))
	  {
	    char *file;
	    unsigned int line;

	    if (expr_symbol_where (sym, &file, &line))
	      as_bad_where (file, line, "unresolved relocation");
	    else
	      as_bad ("bad relocation: symbol `%s' not in symbol table",
		      S_GET_NAME (sym));
	  }

	tc_aout_fix_to_chars (*where, fixP, segment_address_in_file);
	*where += md_reloc_size;
      }
}

#ifndef obj_header_append
/* Aout file generation & utilities */
void
obj_header_append (where, headers)
     char **where;
     object_headers *headers;
{
  tc_headers_hook (headers);

#ifdef CROSS_COMPILE
  md_number_to_chars (*where, headers->header.a_info, sizeof (headers->header.a_info));
  *where += sizeof (headers->header.a_info);
  md_number_to_chars (*where, headers->header.a_text, sizeof (headers->header.a_text));
  *where += sizeof (headers->header.a_text);
  md_number_to_chars (*where, headers->header.a_data, sizeof (headers->header.a_data));
  *where += sizeof (headers->header.a_data);
  md_number_to_chars (*where, headers->header.a_bss, sizeof (headers->header.a_bss));
  *where += sizeof (headers->header.a_bss);
  md_number_to_chars (*where, headers->header.a_syms, sizeof (headers->header.a_syms));
  *where += sizeof (headers->header.a_syms);
  md_number_to_chars (*where, headers->header.a_entry, sizeof (headers->header.a_entry));
  *where += sizeof (headers->header.a_entry);
  md_number_to_chars (*where, headers->header.a_trsize, sizeof (headers->header.a_trsize));
  *where += sizeof (headers->header.a_trsize);
  md_number_to_chars (*where, headers->header.a_drsize, sizeof (headers->header.a_drsize));
  *where += sizeof (headers->header.a_drsize);

#else /* CROSS_COMPILE */

  append (where, (char *) &headers->header, sizeof (headers->header));
#endif /* CROSS_COMPILE */

}
#endif

void
obj_symbol_to_chars (where, symbolP)
     char **where;
     symbolS *symbolP;
{
  md_number_to_chars ((char *) &(S_GET_OFFSET (symbolP)), S_GET_OFFSET (symbolP), sizeof (S_GET_OFFSET (symbolP)));
  md_number_to_chars ((char *) &(S_GET_DESC (symbolP)), S_GET_DESC (symbolP), sizeof (S_GET_DESC (symbolP)));
  md_number_to_chars ((char *) &(symbolP->sy_symbol.n_value), S_GET_VALUE (symbolP), sizeof (symbolP->sy_symbol.n_value));

  append (where, (char *) &symbolP->sy_symbol, sizeof (obj_symbol_type));
}

void
obj_emit_symbols (where, symbol_rootP)
     char **where;
     symbolS *symbol_rootP;
{
  symbolS *symbolP;

  /* Emit all symbols left in the symbol chain.  */
  for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next (symbolP))
    {
      /* Used to save the offset of the name. It is used to point
	 to the string in memory but must be a file offset. */
      register char *temp;

      temp = S_GET_NAME (symbolP);
      S_SET_OFFSET (symbolP, symbolP->sy_name_offset);

      /* Any symbol still undefined and is not a dbg symbol is made N_EXT. */
      if (!S_IS_DEBUG (symbolP) && !S_IS_DEFINED (symbolP))
	S_SET_EXTERNAL (symbolP);

      /* Adjust the type of a weak symbol.  */
      if (S_GET_WEAK (symbolP))
	{
#ifdef TE_NetBSD
	  S_SET_OTHER(symbolP, S_GET_OTHER(symbolP) | 0x20);
#else
	  switch (S_GET_TYPE (symbolP))
	    {
	    case N_UNDF: S_SET_TYPE (symbolP, N_WEAKU); break;
	    case N_ABS:	 S_SET_TYPE (symbolP, N_WEAKA); break;
	    case N_TEXT: S_SET_TYPE (symbolP, N_WEAKT); break;
	    case N_DATA: S_SET_TYPE (symbolP, N_WEAKD); break;
	    case N_BSS:  S_SET_TYPE (symbolP, N_WEAKB); break;
	    default: as_bad ("%s: bad type for weak symbol", temp); break;
	    }
#endif
	}

      obj_symbol_to_chars (where, symbolP);
      S_SET_NAME (symbolP, temp);
    }
}

#endif /* ! BFD_ASSEMBLER */

static void
obj_aout_line (ignore)
     int ignore;
{
  /* Assume delimiter is part of expression.
     BSD4.2 as fails with delightful bug, so we
     are not being incompatible here. */
  new_logical_line ((char *) NULL, (int) (get_absolute_expression ()));
  demand_empty_rest_of_line ();
}				/* obj_aout_line() */

/* Handle .weak.  This is a GNU extension.  */

static void
obj_aout_weak (ignore)
     int ignore;
{
  char *name;
  int c;
  symbolS *symbolP;

  do
    {
      name = input_line_pointer;
      c = get_symbol_end ();
      symbolP = symbol_find_or_make (name);
      *input_line_pointer = c;
      SKIP_WHITESPACE ();
      S_SET_WEAK (symbolP);
      if (c == ',')
	{
	  input_line_pointer++;
	  SKIP_WHITESPACE ();
	  if (*input_line_pointer == '\n')
	    c = '\n';
	}
    }
  while (c == ',');
  demand_empty_rest_of_line ();
}

/* Handle .type.  On {Net,Open}BSD, this is used to set the n_other field,
   which is then apparently used when doing dynamic linking.  Older
   versions of gas ignored the .type pseudo-op, so we also ignore it if
   we can't parse it.  */

static void
obj_aout_type (ignore)
     int ignore;
{
  char *name;
  int c;
  symbolS *sym;

  SKIP_WHITESPACE ();
  name = input_line_pointer;
  c = get_symbol_end ();
  sym = symbol_find_or_make (name);
  *input_line_pointer = c;
  if (sym != NULL)
    {
      SKIP_WHITESPACE ();
      if (*input_line_pointer == ',')
	{
	  ++input_line_pointer;
	  SKIP_WHITESPACE ();
	  if (*input_line_pointer == '@')
	    {
	      ++input_line_pointer;
	      if (strncmp (input_line_pointer, "object", 6) == 0)
		S_SET_OTHER (sym, 1);
	      else if (strncmp (input_line_pointer, "function", 8) == 0)
		S_SET_OTHER (sym, 2);
	      else if (strncmp (input_line_pointer, "label", 5) == 0)
		S_SET_OTHER (sym, 3);
	    }
	}
    }

  /* Ignore everything else on the line.  */
  s_ignore (0);
}

void
obj_read_begin_hook ()
{
}

#ifndef BFD_ASSEMBLER

void
obj_crawl_symbol_chain (headers)
     object_headers *headers;
{
  symbolS *symbolP;
  symbolS **symbolPP;
  int symbol_number = 0;

  tc_crawl_symbol_chain (headers);

  symbolPP = &symbol_rootP;	/*->last symbol chain link. */
  while ((symbolP = *symbolPP) != NULL)
    {
      if (symbolP->sy_mri_common)
	{
	  if (S_IS_EXTERNAL (symbolP))
	    as_bad ("%s: global symbols not supported in common sections",
		    S_GET_NAME (symbolP));
	  *symbolPP = symbol_next (symbolP);
	  continue;
	}

      if (flag_readonly_data_in_text && (S_GET_SEGMENT (symbolP) == SEG_DATA))
	{
	  S_SET_SEGMENT (symbolP, SEG_TEXT);
	}			/* if pusing data into text */

      resolve_symbol_value (symbolP, 1);

      /* Skip symbols which were equated to undefined or common
	 symbols.  */
      if (symbolP->sy_value.X_op == O_symbol
	  && (! S_IS_DEFINED (symbolP) || S_IS_COMMON (symbolP)))
	{
	  *symbolPP = symbol_next (symbolP);
	  continue;
	}

      /* OK, here is how we decide which symbols go out into the brave
	 new symtab.  Symbols that do are:

	 * symbols with no name (stabd's?)
	 * symbols with debug info in their N_TYPE

	 Symbols that don't are:
	 * symbols that are registers
	 * symbols with \1 as their 3rd character (numeric labels)
	 * "local labels" as defined by S_LOCAL_NAME(name) if the -L
	 switch was passed to gas.

	 All other symbols are output.  We complain if a deleted
	 symbol was marked external. */


      if (!S_IS_REGISTER (symbolP)
	  && (!S_GET_NAME (symbolP)
	      || S_IS_DEBUG (symbolP)
	      || !S_IS_DEFINED (symbolP)
	      || S_IS_EXTERNAL (symbolP)
	      || (S_GET_NAME (symbolP)[0] != '\001'
		  && (flag_keep_locals || !S_LOCAL_NAME (symbolP)))))
	{
	  symbolP->sy_number = symbol_number++;

	  /* The + 1 after strlen account for the \0 at the
			   end of each string */
	  if (!S_IS_STABD (symbolP))
	    {
	      /* Ordinary case. */
	      symbolP->sy_name_offset = string_byte_count;
	      string_byte_count += strlen (S_GET_NAME (symbolP)) + 1;
	    }
	  else			/* .Stabd case. */
	    symbolP->sy_name_offset = 0;
	  symbolPP = &(symbol_next (symbolP));
	}
      else
	{
	  if (S_IS_EXTERNAL (symbolP) || !S_IS_DEFINED (symbolP))
	    /* This warning should never get triggered any more.
	       Well, maybe if you're doing twisted things with
	       register names...  */
	    {
	      as_bad ("Local symbol %s never defined.", decode_local_label_name (S_GET_NAME (symbolP)));
	    }			/* oops. */

	  /* Unhook it from the chain */
	  *symbolPP = symbol_next (symbolP);
	}			/* if this symbol should be in the output */
    }				/* for each symbol */

  H_SET_SYMBOL_TABLE_SIZE (headers, symbol_number);
}

/*
 * Find strings by crawling along symbol table chain.
 */

void
obj_emit_strings (where)
     char **where;
{
  symbolS *symbolP;

#ifdef CROSS_COMPILE
  /* Gotta do md_ byte-ordering stuff for string_byte_count first - KWK */
  md_number_to_chars (*where, string_byte_count, sizeof (string_byte_count));
  *where += sizeof (string_byte_count);
#else /* CROSS_COMPILE */
  append (where, (char *) &string_byte_count, (unsigned long) sizeof (string_byte_count));
#endif /* CROSS_COMPILE */

  for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next (symbolP))
    {
      if (S_GET_NAME (symbolP))
	append (&next_object_file_charP, S_GET_NAME (symbolP),
		(unsigned long) (strlen (S_GET_NAME (symbolP)) + 1));
    }				/* walk symbol chain */
}

#ifndef AOUT_VERSION
#define AOUT_VERSION 0
#endif

void
obj_pre_write_hook (headers)
     object_headers *headers;
{
  H_SET_DYNAMIC (headers, 0);
  H_SET_VERSION (headers, AOUT_VERSION);
  H_SET_MACHTYPE (headers, AOUT_MACHTYPE);
  tc_aout_pre_write_hook (headers);
}

void
DEFUN_VOID (s_sect)
{
  /* Strip out the section name */
  char *section_name;
  char *section_name_end;
  char c;

  unsigned int len;
  unsigned int exp;
  char *save;

  section_name = input_line_pointer;
  c = get_symbol_end ();
  section_name_end = input_line_pointer;

  len = section_name_end - section_name;
  input_line_pointer++;
  save = input_line_pointer;

  SKIP_WHITESPACE ();
  if (c == ',')
    {
      exp = get_absolute_expression ();
    }
  else if (*input_line_pointer == ',')
    {
      input_line_pointer++;
      exp = get_absolute_expression ();
    }
  else
    {
      input_line_pointer = save;
      exp = 0;
    }
  if (exp >= 1000)
    {
      as_bad ("subsegment index too high");
    }

  if (strcmp (section_name, ".text") == 0)
    {
      subseg_set (SEG_TEXT, (subsegT) exp);
    }

  if (strcmp (section_name, ".data") == 0)
    {
      if (flag_readonly_data_in_text)
	subseg_set (SEG_TEXT, (subsegT) exp + 1000);
      else
	subseg_set (SEG_DATA, (subsegT) exp);
    }

  *section_name_end = c;
}

#endif /* ! BFD_ASSEMBLER */

static segT
get_segmented_expression (expP)
     register expressionS *expP;
{
  register segT retval;
  
  retval = expression (expP);
  if (expP->X_op == O_illegal
      || expP->X_op == O_absent
      || expP->X_op == O_big)
    {
      as_bad ("expected address expression; zero assumed");
      expP->X_op = O_constant;
      expP->X_add_number = 0;
      retval = absolute_section;
    }

  return retval;
}

static segT 
get_known_segmented_expression (expP)
     register expressionS *expP;
{
  register segT retval;
  
  if ((retval = get_segmented_expression (expP)) == undefined_section)
    {
      /* There is no easy way to extract the undefined symbol from the
	 expression.  */
      if (expP->X_add_symbol != NULL
	  && S_GET_SEGMENT (expP->X_add_symbol) != expr_section)
	as_warn ("symbol \"%s\" undefined; zero assumed",
		 S_GET_NAME (expP->X_add_symbol));
      else
	as_warn ("some symbol undefined; zero assumed");
      retval = absolute_section;
      expP->X_op = O_constant;
      expP->X_add_number = 0;
    }
  
  know (retval == absolute_section || SEG_NORMAL (retval));
  return (retval);
} /* get_known_segmented_expression() */

static void obj_aout_size(ignore) 
     int ignore;
{
  register char *name;
  register char c;
  register char *p;
  register int temp;
  register symbolS *symbolP;
  expressionS     *exp;
  segT            seg;
  segT retval;

  /* SIZE_T is only used in PIC code. */
  if (!aout_pic_flag)
    {
      s_ignore(0);
      return;
    }
  
  SKIP_WHITESPACE();
  name = input_line_pointer;
  c = get_symbol_end();
  /* just after name is now '\0' */
  symbolP = symbol_find(name);
  p = input_line_pointer;
  *p = c;
  
  if (symbolP == NULL || (S_GET_OTHER(symbolP) != 1))
    {
      /* Not an object. Ignore. */
      /* XXX This assumes that the symbol is created before the .size
	 directive. */
      s_ignore(0);
      return;
    }
  
  SKIP_WHITESPACE();
  if (*input_line_pointer != ',') 
    {
      as_bad("Expected comma after symbol-name: rest of line ignored.");
      ignore_rest_of_line();
      return;
    }
  input_line_pointer ++; /* skip ',' */
  exp = (expressionS *)xmalloc(sizeof(expressionS));
  retval = get_known_segmented_expression(exp); 
  if (retval !=  absolute_section)
    {
      as_bad("Illegal .size expression");
      ignore_rest_of_line();
      return;
    }
  
  *p = 0;
  symbolP = symbol_find_or_make(name);
  *p = c;
  if (symbolP->sy_sizexp) {
    as_warn("\"%s\" already has a size", S_GET_NAME(symbolP));
  } else
    symbolP->sy_sizexp = (void *)exp;
  
  demand_empty_rest_of_line();
} /* obj_aout_size() */

/* end of obj-aout.c */
