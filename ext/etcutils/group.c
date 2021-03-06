#include "etcutils.h"
VALUE rb_cGroup, rb_cGshadow;

#ifdef HAVE_PUTGRENT
static VALUE group_gr_put(VALUE self, VALUE io)
{
  struct group grp;
  VALUE path;
#ifdef HAVE_FGETGRENT
  struct group *tmp_grp;
  long i = 0;
#endif

  Check_EU_Type(self, rb_cGroup);
  Check_Writes(io, FMODE_WRITABLE);

  path = RFILE_PATH(io);

  rewind(RFILE_FPTR(io));
  grp.gr_name     = RSTRING_PTR(rb_ivar_get(self, id_name));

#ifdef HAVE_FGETGRENT
  while ( (tmp_grp = fgetgrent(RFILE_FPTR(io))) )
    if ( !strcmp(tmp_grp->gr_name, grp.gr_name) )
      rb_raise(rb_eArgError, "%s is already mentioned in %s:%ld",
	       tmp_grp->gr_name,  StringValuePtr(path), ++i );
#endif

  grp.gr_passwd = RSTRING_PTR(rb_ivar_get(self, id_passwd));
  grp.gr_gid    = NUM2GIDT( rb_ivar_get(self, id_gid) );
  grp.gr_mem    = setup_char_members( rb_iv_get(self, "@members") );

  if ( putgrent(&grp,RFILE_FPTR(io)) )
    eu_errno(RFILE_PATH(io));

  free_char_members(grp.gr_mem, (int)RARRAY_LEN( rb_iv_get(self, "@members") ));

  return Qtrue;
}
#endif

static VALUE group_gr_sprintf(VALUE self)
{
  VALUE args[5];
  args[0] = setup_safe_str("%s:%s:%s:%s\n");
  args[1] = rb_ivar_get(self, id_name);
  args[2] = rb_ivar_get(self, id_passwd);
  args[3] = rb_ivar_get(self,id_gid);
  args[4] = rb_ary_join((VALUE)rb_iv_get(self, "@members"), (VALUE)setup_safe_str(","));
  return rb_f_sprintf(5, args);
}

VALUE group_putgrent(VALUE self, VALUE io)
{
#ifdef HAVE_PUTGRENT
  return group_gr_put(self, io);
#else
  return group_gr_sprintf(self);
#endif
}

VALUE group_gr_entry(VALUE self)
{
  return eu_to_entry(self, group_putgrent);
}

VALUE group_putsgent(VALUE self, VALUE io)
{
#ifdef GSHADOW
  struct sgrp sgroup, *tmp_sgrp;
  VALUE path;
  long i = 0;

  Check_EU_Type(self, rb_cGshadow);
  Check_Writes(io, FMODE_WRITABLE);

  path = RFILE_PATH(io);

  rewind(RFILE_FPTR(io));
  SGRP_NAME(&sgroup)  = RSTRING_PTR(rb_ivar_get(self, id_name));

  while ( (tmp_sgrp = fgetsgent(RFILE_FPTR(io))) )
    if ( !strcmp(SGRP_NAME(tmp_sgrp), SGRP_NAME(&sgroup)) )
      rb_raise(rb_eArgError, "%s is already mentioned in %s:%ld",
	       RSTRING_PTR(rb_ivar_get(self, id_name)), StringValuePtr(path), ++i );

  sgroup.sg_passwd = RSTRING_PTR(rb_ivar_get(self, id_passwd));
  sgroup.sg_adm    = setup_char_members( rb_iv_get(self,"@admins") );
  sgroup.sg_mem    = setup_char_members( rb_iv_get(self, "@members") );

  if ( putsgent(&sgroup,RFILE_FPTR(io)) )
    eu_errno(RFILE_PATH(io));

  free_char_members(sgroup.sg_adm, RARRAY_LEN( rb_iv_get(self, "@admins") ));
  free_char_members(sgroup.sg_mem, RARRAY_LEN( rb_iv_get(self, "@members") ));

  return Qtrue;
#else
  return Qnil;
#endif
}

VALUE group_sg_entry(VALUE self)
{
  return eu_to_entry(self, group_putsgent);
}

VALUE setup_group(struct group *grp)
{
  VALUE obj;
  if (!grp) errno  || (errno = 61); // ENODATA
  eu_errno( setup_safe_str ( "Error setting up Group instance." ) );

  obj = rb_obj_alloc(rb_cGroup);

  rb_ivar_set(obj, id_name, setup_safe_str(grp->gr_name));
  rb_ivar_set(obj, id_passwd, setup_safe_str(grp->gr_passwd));
  rb_ivar_set(obj, id_gid, GIDT2NUM(grp->gr_gid));
  rb_iv_set(obj, "@members", setup_safe_array(grp->gr_mem));

  return obj;
}

#if defined(HAVE_GSHADOW_H) || defined(HAVE_GSHADOW__H)
VALUE setup_gshadow(struct sgrp *sgroup)
{
  VALUE obj;
  if (!sgroup) errno  || (errno = 61); // ENODATA
  eu_errno( setup_safe_str ( "Error setting up GShadow instance." ) );

  obj = rb_obj_alloc(rb_cGshadow);

  rb_ivar_set(obj, id_name, setup_safe_str(SGRP_NAME(sgroup)));
  rb_ivar_set(obj, id_passwd, setup_safe_str(sgroup->sg_passwd));
  rb_iv_set(obj, "@admins", setup_safe_array(sgroup->sg_adm));
  rb_iv_set(obj, "@members", setup_safe_array(sgroup->sg_mem));
  return obj;
}
#endif

void Init_etcutils_group()
{
  /* Define-const: Group
   *
   * The struct contains the following members:
   *
   * name::
   *    contains the name of the group as a String.
   * passwd::
   *    contains the encrypted password as a String. An 'x' is
   *    returned if password access to the group is not available; an empty
   *    string is returned if no password is needed to obtain membership of
   *    the group.
   * gid::
   *    contains the group's numeric ID as an integer.
   * mem::
   *    is an Array of Strings containing the short login names of the
   *    members of the group.
   */
#ifdef GROUP
  rb_define_attr(rb_cGroup, "name", 1, 1);
  rb_define_attr(rb_cGroup, "passwd", 1, 1);
  rb_define_attr(rb_cGroup, "gid", 1, 1);
  rb_define_attr(rb_cGroup, "members", 1, 1);

  rb_define_singleton_method(rb_cGroup,"get",eu_getgrent,0);
  rb_define_singleton_method(rb_cGroup,"find",eu_getgrp,1);
  rb_define_singleton_method(rb_cGroup,"parse",eu_sgetgrent,1);
  rb_define_singleton_method(rb_cGroup,"set",eu_setgrent,0);
  rb_define_singleton_method(rb_cGroup,"end",eu_endgrent,0);
  rb_define_singleton_method(rb_cGroup,"each",eu_getgrent,0);

  rb_define_method(rb_cGroup, "fputs", group_putgrent,1);
  rb_define_method(rb_cGroup, "to_entry", group_gr_entry,0);
#endif

#ifdef GSHADOW
  rb_define_attr(rb_cGshadow, "name", 1, 1);
  rb_define_attr(rb_cGshadow, "passwd", 1, 1);
  rb_define_attr(rb_cGshadow, "admins", 1, 1);
  rb_define_attr(rb_cGshadow, "members", 1, 1);

  rb_define_singleton_method(rb_cGshadow,"get",eu_getsgent,0);
  rb_define_singleton_method(rb_cGshadow,"find",eu_getsgrp,1); //getsgent, getsguid
  rb_define_singleton_method(rb_cGshadow,"parse",eu_sgetsgent,1);
  rb_define_singleton_method(rb_cGshadow,"set",eu_setsgent,0);
  rb_define_singleton_method(rb_cGshadow,"end",eu_endsgent,0);
  rb_define_singleton_method(rb_cGshadow,"each",eu_getsgent,0);
  rb_define_method(rb_cGshadow, "fputs", group_putsgent, 1);
  rb_define_method(rb_cGshadow, "to_entry", group_sg_entry,0);
#endif
}
