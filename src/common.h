#ifndef COMMON_H_
#define COMMON_H_

// MonetDB inclusions
#include "monetdb_config.hpp"

// Error handling, using the convention that the label 'error' exists and can be jumped to, the variable
// function_name indicates the name of the function, the variable rc exists and
#if !defined(NDEBUG) /* debug only */
#define _CCHECK_ERRLINE_EXPAND(LINE) #LINE
#define _CCHECK_ERRLINE(LINE) _CCHECK_ERRLINE_EXPAND(LINE)
#define _CCHECK_ERRMSG(EXPR, ERROR) "[" __FILE__ ":" _CCHECK_ERRLINE(__LINE__) "] " ERROR ": `" #EXPR "'"
#else /* release mode */
#define _CCHECK_ERRMSG(EXPR, ERROR) ERROR
#endif
#define CCHECK( EXPR, ERROR ) if ( !(EXPR) ) \
	{ rc = createException(MAL, function_name /*__FUNCTION__?*/, _CCHECK_ERRMSG( EXPR, ERROR ) ); goto error; }
#ifndef __cplusplus
#define CHECK CCHECK
#endif

// Commodity shortcut, free the bat if it was referenced in the first place
#define BATfree( b ) if(b) { BBPunfix(b->batCacheid); }

#endif /* COMMON_H_ */
