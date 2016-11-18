

/* $Id$ */

#ifndef PHP_BEEM_H
#define PHP_BEEM_H

extern zend_module_entry beem_module_entry;
#define phpext_beem_ptr &beem_module_entry

#define PHP_BEEM_VERSION "0.0.1"

#ifdef PHP_WIN32
# define PHP_BEEM_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_BEEM_API __attribute__ ((visibility("default")))
#else
# define PHP_BEEM_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(beem)
  zend_bool use_namespace;
ZEND_END_MODULE_GLOBALS(beem)

#ifdef ZTS
#define BEEM_G(v) TSRMG(beem_globals_id, zend_beem_globals *, v)
#else
#define BEEM_G(v) (beem_globals.v)
#endif


/* {{{ 注册命名空间 */
#define BEEM_INIT_CLASS_ENTRY(ce, name, ns_name, methods) \
        if( BEEM_G(use_namespace) ){ \
            INIT_CLASS_ENTRY(ce, ns_name, methods); \
        }else{ \
            INIT_CLASS_ENTRY(ce, name, methods); \
        }
/* }}} */

/* {{{  BeeM Object */
typedef struct
{
    zend_object std;
    int  s;
}php_beem_object;
/* }}} */


#define PHP_BEEM_UPDATE_ERROR_CODE(ce, value) zend_update_property_long(ce, getThis(), ZEND_STRL("errorCode"), (long)value TSRMLS_CC);

#define PHP_BEEM_UPDATE_ERROR_INFO(ce, info) zend_update_property_string(ce, getThis(), ZEND_STRL("errorInfo"), info TSRMLS_CC);


static inline php_beem_object *php_beem_object_fetch(zend_object *obj) {
  return (php_beem_object *)((char *)obj - XtOffsetOf(php_beem_object, std));
}

#ifndef Z_OBJ_P
#define Z_OBJ_P(pvz) ((zend_object *) zend_object_store_get_object(pvz TSRMLS_CC))
#endif

#define PHP_BEEM_GET_OBJECT(obj) php_beem_object_fetch(Z_OBJ_P(obj))

#endif  /* PHP_BEEM_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
