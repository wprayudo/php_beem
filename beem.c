
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_beem.h"
#include <beem/be.h>
#include <beem/pubsub.h>

ZEND_DECLARE_MODULE_GLOBALS(beem)

/*{{{ */
static zend_class_entry *beem_ce;
static zend_object_handlers beem_object_handlers;
/* }}} */

static int le_beem;

/* {{{ PHP_INI Namespace Info
 */

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("beem.use_namespace", "0", PHP_INI_SYSTEM, OnUpdateBool, use_namespace, zend_beem_globals, beem_globals)
PHP_INI_END()

/* }}} */

ZEND_BEGIN_ARG_INFO_EX(beem_construct_args, 0, 0, 2)
    ZEND_ARG_INFO(0, domain)
    ZEND_ARG_INFO(0, protocol)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(beem_bind_args, 0, 0, 1)
    ZEND_ARG_INFO(0, endpoint)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(beem_cobe.ct_args, 0, 0, 1)
    ZEND_ARG_INFO(0, addr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(beem_send_args, 0, 0, 2)
    ZEND_ARG_INFO(0, message)
    ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(beem_recv_args, 0, 0, 2)
    ZEND_ARG_INFO(0, buf_length)
    ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(beem_shutdown_args, 0, 0, 1)
    ZEND_ARG_INFO(0, eid)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(beem_setOption_args, 0, 0, 3)
    ZEND_ARG_INFO(0, level)
    ZEND_ARG_INFO(0, option)
    ZEND_ARG_INFO(0, optval)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(beem_getOption_args, 0, 0, 2)
    ZEND_ARG_INFO(0, level)
    ZEND_ARG_INFO(0, option)
ZEND_END_ARG_INFO()

/* {{{ Free Object
 */
static void beem_object_free(php_beem_object *object TSRMLS_DC){
    php_beem_object *std;
#ifdef ZEND_ENGINE_3  
    std = (php_beem_object *)((char *)object - XtOffsetOf(php_beem_object, std));
#else
    std = (php_beem_object *) object;
#endif  
    if( !std ){
      return;
    }

    if( std->s >= 0 ){
      be.close (std->s);
    }

    zend_object_std_dtor(&std->std);
    efree(object);
}
/* }}}
 */

/* {{{ Create new php_beem_object
 */
#ifdef ZEND_ENGINE_3
static zend_object *beem_create_new_object(zend_class_entry *ce TSRMLS_DC){
  
    php_beem_object *object;
  
    object = ecalloc(1, sizeof(php_beem_object) + zend_object_properties_size(ce));

    object->s = -1;

    zend_object_std_init(&(object->std), ce TSRMLS_CC);
    object_properties_init(&(object->std), ce);

    beem_object_handlers.offset = XtOffsetOf(php_beem_object, std);
    beem_object_handlers.free_obj  = (zend_object_free_obj_t) beem_object_free;
    beem_object_handlers.clone_obj = NULL;
    object->std.handlers = &beem_object_handlers;
    
    return &object->std;
}
#else
static zend_object_value beem_create_new_object(zend_class_entry *ce TSRMLS_DC){
    zend_object_value retval;
    php_beem_object *object;

    object = ecalloc(1,sizeof(php_beem_object));
    object->std.ce = ce;

    object->s = -1;

    object_properties_init(&object->std, ce);
    retval.handle  = zend_objects_store_put(object, NULL, (zend_objects_free_object_storage_t) beem_object_free, NULL TSRMLS_CC);
    retval.handlers = &beem_object_handlers;

    return retval;
}
#endif
/* }}}
 */

/* {{{ new Beem( BeeM::AF_BEE, BeeM::BE_BUS );
 */
PHP_METHOD(Beem, __construct){

    php_beem_object *object;
    long domain, protocol;

    if( zend_parse_parameters(ZEND_NUM_ARGS () TSRMLS_CC, "ll", &domain, &protocol) == FAILURE ){
      php_error_docref(NULL, E_ERROR, "Expected at least 2 parameter.");
      return;
    }

    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);

    object->s = be.socket (domain, protocol);
    if (object->s < 0){
      char *errorInfo;
      spprintf(&errorInfo,0,"Error creating beem socket:%s",be.strerror (errno));
      PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 400);
      PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
      efree(errorInfo);
      return;
    }
}
/* }}}
 */

/* {{{  Beem bind $beem->bind('ipc:///tmp/node0.ipc');
 */
PHP_METHOD(Beem, bind){
  
    php_beem_object *object;

    int endpoint_id,endpoint_length;
    char *endpoint;

    if(zend_parse_parameters(ZEND_NUM_ARGS () TSRMLS_CC, "s", &endpoint, &endpoint_length) == FAILURE) {
        return;
    }

    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);
    endpoint_id = be.bind (object->s, endpoint);

    if (endpoint_id < 0) {
      char *errorInfo;
      spprintf(&errorInfo,0,"Beem bind error:%s",be.strerror (errno));
      PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 401);
      PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
      efree(errorInfo);
        return;
    }
    
    RETURN_LONG(endpoint_id);
}
/* }}}
 */

/* {{{  Beem cobe.ct $beem->cobe.ct('ipc:///tmp/node0.ipc');
 */
PHP_METHOD(Beem, cobe.ct){
  
    php_beem_object *object;

    int endpoint_id,endpoint_length;
    char *addr;

    if(zend_parse_parameters(ZEND_NUM_ARGS () TSRMLS_CC, "s", &addr, &endpoint_length) == FAILURE) {
        return;
    }

    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);
    endpoint_id = be.cobe.ct (object->s, addr);

    if (endpoint_id < 0) {
      char *errorInfo;
      spprintf(&errorInfo,0,"Beem cobe.ct error:%s",be.strerror (errno));
      PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 402);
      PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
      efree(errorInfo);
        return;
    }
    
    RETURN_LONG(endpoint_id);
}
/* }}}
 */

/* {{{  Beem send $beem->send('data',0);
 */

PHP_METHOD(Beem, send){
  
    php_beem_object *object;

    int  message_length,send_length;
    char *message;
    long flags = 0;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &message, &message_length, &flags) == FAILURE) {
        return;
    }

    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);
    
    if( (send_length = be.send (object->s, message, message_length, flags)) < 0 ){
      if( flags & BE_DONTWAIT && errno == EAGAIN ){
        char *errorInfo;
        spprintf(&errorInfo,0,"Beem send error:%s",be.strerror (errno));
        PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 403);
        PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
        efree(errorInfo);
        RETURN_LONG(0);
      }
      return;
    }

    RETURN_LONG(send_length);
}
/* }}}
 */

/* {{{  Beem set socket option $beem->setOption(BeeM::BE_SOL_SOCKET, BeeM::BE_RCVTIMEO, 100);
 */
PHP_METHOD(Beem, setOption){
    php_beem_object *object;
    long level, option;
    zval *optval;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llz", &level, &option, &optval) == FAILURE ){
      return;
    }

    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);

    if( level == BE_SUB && (option == BE_SUB_SUBSCRIBE || option == BE_SUB_UNSUBSCRIBE) ){
      if( be.setsockopt(object->s, level, option, Z_STRVAL_P(optval), Z_STRLEN_P(optval)) >= 0 ){
        RETURN_TRUE;
        return;
      }
    }else{
      const void *option_val = &Z_LVAL_P(optval);
      if( be.setsockopt(object->s, level, option, option_val, sizeof(int)) >= 0 ){
        RETURN_TRUE;
        return;
      }
    }

    char *errorInfo;
    spprintf(&errorInfo,0,"Beem set socket option error:%s",be.strerror (errno));
    PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 406);
    PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
    efree(errorInfo);
    RETURN_FALSE;
}
/* }}}
 */

/* {{{  Beem get socket option $beem->getOption(BeeM::BE_SOL_SOCKET, BeeM::BE_RCVTIMEO);
 */
PHP_METHOD(Beem, getOption){
    php_beem_object *object;
    long level, option;
    int optval;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &level, &option) == FAILURE ){
      return;
    }

    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);

    size_t sz = sizeof(optval);
    if( be.getsockopt (object->s, level, option, &optval, &sz) >= 0 ){
      RETURN_LONG(optval);
      return;
    }

    char *errorInfo;
    spprintf(&errorInfo,0,"Beem set socket option error:%s",be.strerror (errno));
    PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 406);
    PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
    efree(errorInfo);
    RETURN_FALSE;
}
/* }}}
 */

/* {{{  Beem recv $beem->recv(0,0);
 */
PHP_METHOD(Beem, recv){
  
    php_beem_object *object;

    void *buf = NULL;
    long flags = 0;
    long buf_length = 0;

    if(zend_parse_parameters(ZEND_NUM_ARGS () TSRMLS_CC, "l|l", &buf_length, &flags) == FAILURE) {
        return;
    }
    
    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);

    if( buf_length > 0 ){
      char buf_recv[buf_length];
      if( be.recv (object->s, buf_recv, buf_length, flags) > 0 ){
#ifdef ZEND_ENGINE_3
        ZVAL_STRINGL(return_value, buf_recv, buf_length);
#else
        ZVAL_STRINGL(return_value, buf_recv, buf_length, 1);
#endif
        return;
      }
    }else{
      if( (buf_length = be.recv (object->s, &buf, BE_MSG, flags)) > 0 ){
#ifdef ZEND_ENGINE_3
        ZVAL_STRINGL(return_value, buf, buf_length);
#else
        ZVAL_STRINGL(return_value, buf, buf_length, 1);
#endif
        be.freemsg(buf);
        return;
      }
    }

    char *errorInfo;
    spprintf(&errorInfo,0,"Beem recv error:%s",be.strerror (errno));
    PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 404);
    PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
    efree(errorInfo);
    RETURN_FALSE;
}
/* }}}
 */

/* {{{  Beem shutdown $beem->shutdown($eid);
 */
PHP_METHOD(Beem, shutdown){
    php_beem_object *object;
    int eid;

    if(zend_parse_parameters(ZEND_NUM_ARGS () TSRMLS_CC, "l", &eid) == FAILURE) {
        return;
    }
    
    object = PHP_BEEM_GET_OBJECT(getThis() TSRMLS_CC);

    if( be.shutdown(object->s, eid) >= 0 ){
      RETURN_TRUE;
      return;
    }

    char *errorInfo;
    spprintf(&errorInfo,0,"Beem shutdown error:%s",be.strerror (errno));
    PHP_BEEM_UPDATE_ERROR_CODE(beem_ce, 405);
    PHP_BEEM_UPDATE_ERROR_INFO(beem_ce, errorInfo);
    efree(errorInfo);
    RETURN_FALSE;
}
/* }}}
 */

/* {{{ beem_functions[]
 *
 * Every user visible function must have an entry in beem_functions[].
 */
const zend_function_entry beem_method[] = {
    PHP_ME(Beem, __construct, beem_construct_args,  ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Beem, bind,        beem_bind_args,       ZEND_ACC_PUBLIC)
    PHP_ME(Beem, cobe.ct,     beem_cobe.ct_args,    ZEND_ACC_PUBLIC)
    PHP_ME(Beem, send,      	 beem_send_args,       ZEND_ACC_PUBLIC)
    PHP_ME(Beem, recv,      	 beem_recv_args,       ZEND_ACC_PUBLIC)
    PHP_ME(Beem, shutdown,    beem_shutdown_args,   ZEND_ACC_PUBLIC)
    PHP_ME(Beem, setOption,   beem_setOption_args,  ZEND_ACC_PUBLIC)
    PHP_ME(Beem, getOption,   beem_getOption_args,  ZEND_ACC_PUBLIC)
    PHP_FE_END  /* Must be the last line in beem_functions[] */
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(beem)
{
    REGISTER_INI_ENTRIES();

    memcpy(&beem_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    
    zend_class_entry ce;
    BEEM_INIT_CLASS_ENTRY(ce, "Beem", "Widuu\\Beem", beem_method);
    ce.create_object = beem_create_new_object;
    beem_ce = zend_register_internal_class(&ce TSRMLS_CC);
    
    // 注册静态变量
    int value, i;
      for (i = 0; ; ++i) {
          const char *name = be.symbol (i, &value);
          if (name == NULL)
              break;
      zend_declare_class_constant_long(beem_ce, name, strlen(name), (long) value TSRMLS_CC);
    }

    // 最后一条错误信息
    zend_declare_property_string(beem_ce, ZEND_STRL("errorInfo"), "", ZEND_ACC_PUBLIC TSRMLS_CC); 
    // 错误代码
    zend_declare_property_long(beem_ce, ZEND_STRL("errorCode"), 0, ZEND_ACC_PUBLIC TSRMLS_CC);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(beem)
{
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(beem)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "beem support", "enabled");
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */


/* {{{ beem_module_entry
 */
zend_module_entry beem_module_entry = {
    STANDARD_MODULE_HEADER,
    "beem",
    NULL,
    PHP_MINIT(beem),
    PHP_MSHUTDOWN(beem),
    NULL,
    NULL,
    PHP_MINFO(beem),
    PHP_BEEM_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_BEEM
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(beem)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
