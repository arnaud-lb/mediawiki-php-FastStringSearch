/*
 * PHP extension for fast string search routines
 * Copyright Tim Starling, 2006
 * License: DWTFYWWI
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "Zend/zend.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fss.h"
#include "zend_smart_str.h"
#include "kwset.h"

typedef struct {
	kwset_t set;
	int replace_size;
	zval * replace[1];
} fss_resource_t;

static void _php_fss_close(zend_resource *rsrc TSRMLS_DC);

/* True global resources - no need for thread safety here */
static int le_fss;

/* {{{ fss_functions[]
 */
const zend_function_entry fss_functions[] = {
	PHP_FE(fss_prep_search,	    NULL)
	PHP_FE(fss_exec_search,	    NULL)
	PHP_FE(fss_prep_replace,	NULL)
	PHP_FE(fss_exec_replace,	NULL)
	PHP_FE(fss_free,	        NULL)
    PHP_FE_END
};
/* }}} */

/* {{{ fss_module_entry
 */
zend_module_entry fss_module_entry = {
        STANDARD_MODULE_HEADER,
	    "fss",
	    fss_functions,
	    PHP_MINIT(fss),
	    PHP_MSHUTDOWN(fss),
	    NULL, /* RINIT */
	    NULL, /* RSHUTDOWN */
	    PHP_MINFO(fss),
        "0.1.1",
        STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FSS
ZEND_GET_MODULE(fss)
#endif



/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(fss)
{
	le_fss = zend_register_list_destructors_ex(_php_fss_close, NULL, "fss", module_number);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(fss)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(fss)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Fast string search support", "enabled");
	php_info_print_table_end();
}
/* }}} */


/* {{{ proto resource fss_prep_search(mixed needle)
   Prepare a string search */
PHP_FUNCTION(fss_prep_search)
{
	zval *needle = NULL, *elem;
    fss_resource_t *res;
	const char *error;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &needle) == FAILURE) {
		return;
	}

	res = emalloc(sizeof(fss_resource_t));
	res->set = kwsalloc(NULL);
	res->replace_size = 0;

	if (Z_TYPE_P(needle) == IS_ARRAY) {
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(needle), elem) {
                    if(!Z_STRLEN_P(elem)) {
                        continue;
                    }
                    error = kwsincr(res->set, Z_STRVAL_P(elem), Z_STRLEN_P(elem));
                    if(error) {
                        php_error(E_WARNING, "fss_prep_search: %s", error);
                    }
                }ZEND_HASH_FOREACH_END();

    } else {
        convert_to_string_ex(needle);
		error = kwsincr(res->set, Z_STRVAL_P(needle), Z_STRLEN_P(needle));
		if(error) {
			php_error(E_WARNING, "fss_prep_search: %s", error);
		}
	}
	kwsprep(res->set);
    RETURN_RES(zend_register_resource(res, le_fss));
}
/* }}} */

/* {{{ proto array fss_exec_search(resource handle, string haystack [, int offset])
   Execute a string search, return the first match */
PHP_FUNCTION(fss_exec_search)
{
	char *haystack = NULL;
	size_t haystack_len;
	long offset = 0;
    zval *handle = NULL, *temp;
	fss_resource_t *res;
	struct kwsmatch m;
	size_t match_pos;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &handle, &haystack, &haystack_len, &offset) == FAILURE) {
        return;
    }

	if (offset >= haystack_len || offset < 0) {
		RETURN_FALSE;
	}

	if( (res = (fss_resource_t *)zend_fetch_resource(Z_RES_P(handle), "fss", le_fss)) == NULL) {
        RETURN_FALSE;
    };

	match_pos = kwsexec(res->set, haystack + offset, haystack_len - offset, &m);

	if (match_pos == (size_t)-1) {
		RETURN_FALSE;
	}


	array_init(return_value);

	zval res_temp[2];
	ZVAL_LONG(&res_temp[0], m.offset[0] + offset);
	zend_hash_next_index_insert(HASH_OF(return_value), (void *)&res_temp[0]);
	ZVAL_LONG(&res_temp[1], m.size[0]);
	zend_hash_next_index_insert(HASH_OF(return_value), (void *)&res_temp[1]);

    zval_ptr_dtor(&res_temp[1]);
    zval_ptr_dtor(&res_temp[0]);

}
/* }}} */

/* {{{ proto resource fss_prep_replace(array replace_pairs)
   Prepare a search and replace operation */
PHP_FUNCTION(fss_prep_replace)
{
	zval *replace_pairs = NULL, *value;
    zend_string *string_key;
	HashTable * hash;
	HashPosition hpos;
	fss_resource_t * res;
	const char *error;
	int i;
	char buffer[40];
    size_t string_key_len;
	zend_long num_key;


	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &replace_pairs) == FAILURE) {
        return;
    }

	hash = Z_ARRVAL_P(replace_pairs);

	/* fss_resource_t has an open-ended array, we allocate enough memory for the
	   header plus all the array elements, plus one extra element for good measure */
	res = safe_emalloc(sizeof(zval*), hash->nNumOfElements, sizeof(fss_resource_t));
	res->set = kwsalloc(NULL);
	res->replace_size = hash->nNumOfElements;

//	for (zend_hash_internal_pointer_reset_ex(hash, &hpos), i = 0; (value=zend_hash_get_current_data_ex(hash, &hpos)) == SUCCESS; zend_hash_move_forward_ex(hash, &hpos), ++i)
//	{
//		/* Convert numeric keys to string */
//		if (zend_hash_get_current_key_ex(hash, &string_key, &num_key, &hpos) == HASH_KEY_IS_LONG)
//		{
//			sprintf(buffer, "%lu", num_key);
//            string_key = zend_string_init(buffer, strlen(buffer), 0);
//			string_key_len = strlen(buffer);
//		} else {
//			/* Minus one for null */
//			string_key_len--;
//		}
//
//		/* Don't add zero-length strings, that will cause infinite loops in
//		   search routines */
//		if (!string_key_len) {
//			res->replace[i] = NULL;
//			continue;
//		}
//
//		/* Add the key to the search tree */
//		error = kwsincr(res->set, ZSTR_VAL(string_key), string_key_len);
//		if (error) {
//			res->replace[i] = NULL;
//			php_error(E_WARNING, "fss_prep_replace: %s", error);
//			continue;
//		}
//
//		/* Add the value to the replace array */
//		convert_to_string_ex(value);
//
//		ZVAL_ADDREF(*value);
//
//		res->replace[i] = value;
//	}


    i = 0;
    ZEND_HASH_FOREACH_KEY_VAL(hash, num_key, string_key, value) {

                /* Convert numeric keys to string */
                if(!string_key) {
                    sprintf(buffer, "%lu", num_key);
                    string_key = zend_string_init(buffer, strlen(buffer), 0);
                    string_key_len = strlen(buffer);
                } else {
                    string_key_len = string_key->len;
                    /* Minus one for null */
                    string_key_len--;
                }

                /* Don't add zero-length strings, that will cause infinite loops in search routines */
                if (!string_key_len) {
                    res->replace[i] = NULL;
                    continue;
                }

                /* Add the key to the search tree */
                error = kwsincr(res->set, ZSTR_VAL(string_key), string_key_len);
                if (error) {
                    res->replace[i] = NULL;
                    php_error(E_WARNING, "fss_prep_replace: %s", error);
                    i++;
                    continue;
                }

                /* Add the value to the replace array */
                convert_to_string_ex(value);

                ZVAL_MAKE_REF(value);
                Z_ADDREF_P(value);
                res->replace[i] = value;
                i++;
            } ZEND_HASH_FOREACH_END();


	kwsprep(res->set);

    ZVAL_RES(return_value, zend_register_resource(res, le_fss));
}
/* }}} */

/* {{{ proto string fss_exec_replace(resource handle, string subject)
   Execute a search and replace operation */
PHP_FUNCTION(fss_exec_replace)
{
	char *subject = NULL;
	int argc = ZEND_NUM_ARGS();
	int handle_id = -1;
	size_t subject_len;
    zval *handle = NULL;
	size_t match_pos = 0;
	fss_resource_t * res;
	struct kwsmatch m;
	smart_str result = {0};
	zval *temp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs", &handle, &subject, &subject_len) == FAILURE) {
        return;
    }

    if( (res = (fss_resource_t *)zend_fetch_resource(Z_RES_P(handle), "fss", le_fss)) == NULL) {
        RETURN_FALSE;
    };

	while (subject_len > 0 && (size_t)-1 != (match_pos = kwsexec(res->set, subject, subject_len, &m)))
	{
		/* Output the leading portion */
		smart_str_appendl(&result, subject, match_pos);

		/* Output the replacement portion
		   The index may be above the size of the replacement array if the
		   object was prepared as a search object instead of a replacement
		   object. In that case, we replace the item with an empty string
		 */
		if (m.index < res->replace_size) {
			temp = res->replace[m.index];
			if (temp) {
				smart_str_appendl(&result, Z_STRVAL_P(temp), Z_STRLEN_P(temp));
			}
		}

		/* Increment and continue */
		subject_len -= match_pos + m.size[0];
		subject += match_pos + m.size[0];
	}
	/* Output the remaining portion */
	if (subject_len > 0) {
		smart_str_appendl(&result, subject, subject_len);
	}
	/* Return the result */
	if (result.s) {
		smart_str_0(&result);
		RETURN_STR(result.s);
	} else {
		RETURN_EMPTY_STRING();
	}
}
/* }}} */

/* {{{ proto void fss_free(resource handle)
   Free an FSS object */
PHP_FUNCTION(fss_free)
{
    zval *handle = NULL;
	fss_resource_t * res;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &handle) == FAILURE) {
        return;
    }

    if( (res = (fss_resource_t *)zend_fetch_resource(Z_RES_P(handle), "fss", le_fss)) == NULL) {
        RETURN_FALSE;
    };

	if (handle) {
        if(zend_list_close(Z_RES_P(handle)) == FAILURE) {
            RETURN_FALSE;
        };
        RETURN_TRUE;
	}
}
/* }}} */

/* {{{ _php_fss_close
   List destructor for FSS handles */
static void _php_fss_close(zend_resource *rsrc TSRMLS_DC)
{
	int i;
	fss_resource_t * res = (fss_resource_t *)rsrc->ptr;
	/* Destroy the replace strings */
	for (i = 0; i < res->replace_size; i++) {
		if (res->replace[i]) {
			zval_ptr_dtor(res->replace[i]);
		}
	}
	/* Destroy the kwset structure */
	kwsfree(res->set);
	/* Destroy the resource structure itself */
	efree(res);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
