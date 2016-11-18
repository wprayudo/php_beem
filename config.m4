dnl $Id$
dnl config.m4 for extension beem

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(beem, for beem support,
Make sure that the comment is aligned:
[  --with-beem             Include beem support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(beem, whether to enable beem support,
Make sure that the comment is aligned:
[  --enable-beem           Enable beem support])

if test "$PHP_BEEM" != "no"; then
  PHP_NEW_EXTENSION(beem, beem.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
