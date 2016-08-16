/* *INDENT-OFF* */
#if !defined(IINQ_H_)
#define IINQ_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>
#include <alloca.h>
#include "../dictionary/dictionary_types.h"
#include "../dictionary/ion_master_table.h"
#include "../key_value/kv_system.h"
#include "../util/sort/external_sort/external_sort.h"

/**
@brief		Page size in bytes.
*/
#define IINQ_PAGE_SIZE	512

/**
@brief		A size type for IINQ results (row data from an IINQ query).
*/
typedef unsigned int ion_iinq_result_size_t;

/**
@brief		IINQ result type.
@details	At present, this does not contain any schema information.
*/
typedef struct {
	/**> The number of bytes contained of these results. */
	ion_iinq_result_size_t	num_bytes;
	/**> Pointer to the bytes of this result. */
	unsigned char		*data;
} ion_iinq_result_t;

/**
@brief		Function pointer type for processing data produced by IINQ queries.
@details	The function is only used when the query does not modify any
			source's data.
*/
typedef	void	(*ion_iinq_query_processor_func_t)(ion_iinq_result_t*, void*);

/**
@brief		Create a new IINQ result processor function, with the correct
			signature.
*/
#define IINQ_NEW_PROCESSOR_FUNC(name) \
void name(ion_iinq_result_t *result, void* state)

/**
@brief		The IINQ query processor object.
@details	This object contains a state, as well as a pointer to a function
			that is executed once per result row generated by an IINQ query.
			The state, as well as the result row, is passed into the query
			processing function. The state is user defined, and permits complex
			post processing.
*/
typedef struct {
	/**> The query processing function to execute for each result row. */
	ion_iinq_query_processor_func_t	execute;
	/**> A user-defined query state passed into @ref execute. */
	void							*state;
} ion_iinq_query_processor_t;

/**
@brief		A macro that aids in the creation of stack-allocated
			query processor objects.
*/
#define IINQ_QUERY_PROCESSOR(execute, state)	((ion_iinq_query_processor_t){ execute, state })

/**
@brief		An object containing pointers to a dictionary and other
			information associated with said dictionary.
@details	See @ref ion_iinq_source_t for more information.
*/
typedef struct iinq_source ion_iinq_source_t;

/**
@brief		This is a linked-list item used to manage dictionary objects
			during a query and especially while cleaning up a query.
@todo		There is definitely an opportunity to turn this linked list into
			an array, and should be considered to reduce memory requirements.
*/
typedef struct iinq_cleanup {
	/**> A pointer to a eference struct of an IonDB dictionary. */
	ion_iinq_source_t	*reference;
	/**> A pointer to the next item in the list. */
	struct iinq_cleanup *next;
	/**> A pointer to the previous item in the list. */
	struct iinq_cleanup *last;
} ion_iinq_cleanup_t;

/**
@brief		A structure containing objects and pointers to objects for IonDB
			dictionaries for the purposes of querying with IINQ.
@details	To make the IINQ query interface nice to use, we add pointers
			to the key and value data we load from the @ref dictionary object.
			We also track a pointer to the cleanup tracker used to clean this
			item up.

			We also keep things we will need to reference for this query here
			too, such as cursors, predicates, and IonDB records.
*/
struct iinq_source {
	/**> A dictionary handler instance for this source. */
	ion_dictionary_handler_t	handler;
	/**> A dictionary instance for this source. */
	ion_dictionary_t			dictionary;
	/**> A dictionary cursor predicate for this instance. It will be built as
		 an all-records predicate. */
	ion_predicate_t				predicate;
	/**> A pointer to the dictionary cursor used to iterate through this
		 source's dictionary. */
	ion_dict_cursor_t			*cursor;
	/**> A cursor status to track the validity and state of the cursor
		 for this source's dictionary. */
	ion_cursor_status_t			cursor_status;
	/**> A pointer referencing the key for the current record loaded from
		 this source's dictionary. */
	ion_key_t					key;
	/**> A pointer referencing the value for the current record loaded from
		 this source's dictionary. */
	ion_value_t					value;
	/**> An IonDB record that manages the key and value data loaded from
		 this source's dictionary. */
	ion_record_t				ion_record;
	/**> An IINQ cleanup object that will be tracked in a list for managing
		 cursor re-initialiation and such during queries and object destruction
		 after the query has completed. */
	ion_iinq_cleanup_t			cleanup;
};

/**
@brief		IINQ aggregate status codes.
*/
typedef enum {
	/**> An aggregate status denoting an uninitialized aggregate. */
	IINQ_AGGREGATE_UNINITIALIZED	= 0,
	/**> An aggregate status denoting an initialized aggregate. */
	IINQ_AGGREGATE_INITIALIZED		= 1
} iinq_aggregate_status_e;

/**
@brief		IINQ aggregate type codes.
*/
typedef enum {
	/**> An aggregate type denoting the aggregate has a signed integral
		 type (64 bits in size). */
	IINQ_AGGREGATE_TYPE_INT,
	/**> An aggregate type denoting the aggregate has an unsigned integral
		 type (64 bits in size). */
	IINQ_AGGREGATE_TYPE_UINT,
	/**> An aggregate type denoting the aggregate has a signed double
		 type (64 bits in size). */
	IINQ_AGGREGATE_TYPE_DOUBLE
} iinq_aggregate_type_e;

/**
@brief		A type used to store IINQ aggregate statuses with.
@details	This type exists so we can control the size of the type used
			to store these values in, instead of relying on the size of
			an enum.
*/
typedef uint8_t iinq_aggregate_status_t;

/**
@brief		A type used to store IINQ aggregate types with.
@details	This type exists so we can control the size of the type used
			to store these values in, instead of relying on the size of
			an enum.
*/
typedef uint8_t iinq_aggregate_type_t;

/**
@brief		A variable type union used to store any type of aggregate
			value in.
*/
typedef union {
	/**> A 64-bit signed integer value. */
	int64_t		i64;
	/**> A 64-bit unsigned integer value. */
	uint64_t	u64;
	/**> A 64-bit floating-point value (double). */
	double 		f64;
} iinq_aggregate_value_t;

/**
@brief		An IINQ aggregate object.
@details	This object is used for each declared aggregate for any IINQ query.
*/
typedef struct {
	/**> The type of the IINQ aggregate object. */
	iinq_aggregate_type_t	type;
	/**> The status of the IINQ aggregate object. */
	iinq_aggregate_status_t	status;
	/**> The value of the IINQ aggregate object. */
	iinq_aggregate_value_t	value;
} iinq_aggregate_t;

/**
@brief		A generic size type for the IINQ library.
*/
typedef size_t iinq_size_t;

/**
@brief		A type for storing the sorting direction used in ORDER BY clauses
			and such in IINQ queries.
*/
typedef int8_t iinq_order_direction_t;

/**
@brief		An object describing one part of an ordering clause (such as
			ORDER BY and GROUP BY).
*/
typedef struct {
	/**> A pointer to a stack-allocated-and-evaluated expression result. */
	void					*pointer;
	/**> The size of the exression pointed to by @ref pointer. */
	iinq_size_t				size;
	/**> The ordering direction of this ordering object (ASCENDING or
		 DESCENDING. */
	iinq_order_direction_t	direction;
} iinq_order_part_t;

/**
@brief		Comparator context for IonDB's sorting methods.
@details	IonDB provides sorting utilities for efficiently implementing
			complex query techniques, but requires some help for comparisons.
			This comes in the form of a user-defined context. This context will
			allow IINQ queries to compare ordering keys based on directions
			(this looks like ASCENDING and DESCENDING in traditional SQL
			queries).
*/
typedef struct {
	/**> The ordering parts for the clause we are comparing for. */
	iinq_order_part_t	*parts;
	/**> The number of parts to compare. */
	int					n;
} iinq_sort_context_t;

#define _IINQ_SORT_CONTEXT(name)	((iinq_sort_context_t){ name ## _order_parts , name ## _n })

/**
@param		schema_file_name
				A pointer to a character array describing the name of the schema
				file to open.
@param		key_type
				The type of key to store in this source and it's dictionary.
@param		key_size
				The size of the key to store in this source and it's dictionary.
@param		value_size
				The size of the value to store in this source and it's
				dictionary.
@return		An error describing the result of the call.
*/
ion_err_t
iinq_create_source(
	char						*schema_file_name,
	ion_key_type_t				key_type,
	ion_key_size_t				key_size,
	ion_value_size_t			value_size
);

/**
@param		schema_file_name
				A pointer to a character array describing the name of the schema
				file to open.
@param		dictionary
				A pointer to a dictionary object to open, initialize, and
				manipulate.
@param		handler
				A pointer to a pre-allocated handler object that will be
				initialized as a result of this function call.
@return		An error describing the result of the call.
*/
ion_err_t
iinq_open_source(
	char						*schema_file_name,
	ion_dictionary_t			*dictionary,
	ion_dictionary_handler_t	*handler
);

/**
@brief		Insert a key/value into a source (and it's underlying dictionary).
@param		schema_file_name
				A pointer to a character array describing the name of the schema
				file to open.
@param		key
				The key to insert.
@param		value
				The value to insert.
@return		A status describing the result of the call.
*/
ion_status_t
iinq_insert(
	char 		*schema_file_name,
	ion_key_t	key,
	ion_value_t value
);

/**
@brief		Update all values associated with a key in a given source
			(and it's underlying dictionary).
@param		schema_file_name
				A pointer to a character array describing the name of the schema
				file to open.
@param		key
				The key to update.
@param		value
				The value to update with.
@return		A status describing the result of the call.
*/
ion_status_t
iinq_update(
	char 		*schema_file_name,
	ion_key_t	key,
	ion_value_t value
);

/**
@brief		Delete all records associated with a key in a source (and it's
			associated dictionary).
@param		schema_file_name
				A pointer to a character array describing the name of the schema
				file to open.
@param		key
				The key for which we wish to delete all records associated.
@return		A status describing the result of the call.
*/
ion_status_t
iinq_delete(
	char 		*schema_file_name,
	ion_key_t	key
);

/**
@brief		Drop a source.
@param		schema_file_name
				A pointer to a character array describing the name of the schema
				file to open.
@return		An error describing the result of the call.
*/
ion_err_t
iinq_drop(
	char *schema_file_name
);

ion_comparison_e
iinq_sort_compare(
	void	*context,	// TODO: Turn this into a ion_sort_comparator_context_t.
	void	*a,
	void	*b
);

/* *** START code for IF_ELSE()()() Macro. *** */
/* Some helper macros. See http://jhnet.co.uk/articles/cpp_magic */
#define SECOND(a, b, ...) b

#define IS_PROBE(...) SECOND(__VA_ARGS__, 0, 0)
#define PROBE() ~, 1

#define CAT(a,b) a ## b

#define NOT(x) IS_PROBE(CAT(_NOT_, x))
#define _NOT_0 PROBE()

#define BOOL(x) NOT(NOT(x))

/**
@brief		The way to use this macro is to specify a condition (that can
			be evaluated at macro expansion) within a set of parentheses,
			followed by a statement wrapped in parentheses, followed by
			another statement wrapped in parentheses.
@details	Example:
 				IF_ELSE(SOME_MACRO_EQUAL_TO_ONE)(we_execute_this())(we_dont_execute_this());
*/
#define IF_ELSE(condition) _IF_ELSE(BOOL(condition))
#define _IF_ELSE(condition) CAT(_IF_, condition)

#define _IF_1(...) __VA_ARGS__ _IF_1_ELSE
#define _IF_0(...)             _IF_0_ELSE

#define _IF_1_ELSE(...)
#define _IF_0_ELSE(...) __VA_ARGS__
/* *** END code for IF_ELSE()()() Macro. *** */

/* *** START code for PP_NARG Macro. *** */
/**
@brief		Returns the number of arguments passed in (up to 63 arguments).
@details	This macro is pretty cool. It will correctly return the right
			number of parameters for blocks of code and a variety of other
			text parameters.
*/
#define PP_NARG(...) \
    PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
    PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
    _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
    _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
    _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
    _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
    _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
    _61,_62,_63,  N, ...) N
#define PP_RSEQ_N() \
    63,62,61,60,                   \
    59,58,57,56,55,54,53,52,51,50, \
    49,48,47,46,45,44,43,42,41,40, \
    39,38,37,36,35,34,33,32,31,30, \
    29,28,27,26,25,24,23,22,21,20, \
    19,18,17,16,15,14,13,12,11,10, \
     9, 8, 7, 6, 5, 4, 3, 2, 1, 0
/* *** START code for PP_NARG Macro. *** */

/**
@brief		Define a schema for a source.
*/
#define DEFINE_SCHEMA(source_name, struct_def) \
struct iinq_ ## source_name ## _schema struct_def

#define CREATE_DICTIONARY(schema_name, key_type, key_size, value_size) \
iinq_create_source(#schema_name ".inq", key_type, key_size, value_size)

#define INSERT(schema_name, key, value) \
iinq_insert(#schema_name ".inq", key, value)

#define UPDATE(schema_name, key, value) \
iinq_insert(#schema_name ".inq", key, value)

#define DELETE(schema_name, key) \
iinq_delete(#schema_name ".inq", key)

#define DROP(schema_name)\
iinq_drop(#schema_name ".inq")

#define SELECT_ALL \
ion_iinq_result_size_t result_loc	= 0; \
ion_iinq_cleanup_t *copyer			= first; \
while (NULL != copyer) { \
	memcpy(result.data+(result_loc), copyer->reference->key, copyer->reference->dictionary.instance->record.key_size); \
	result_loc += copyer->reference->dictionary.instance->record.key_size; \
	memcpy(result.data+(result_loc), copyer->reference->value, copyer->reference->dictionary.instance->record.value_size); \
	result_loc += copyer->reference->dictionary.instance->record.value_size; \
	copyer							= copyer->next; \
}

#define _FROM_SOURCE_SINGLE(source) \
	ion_iinq_source_t source; \
	source.cleanup.next			= NULL; \
	source.cleanup.last			= last; \
	source.cleanup.reference	= &source; \
	if (NULL == first) { \
		first					= &source.cleanup; \
	} \
	if (NULL != last) { \
		last->next				= &source.cleanup; \
	} \
	last						= &source.cleanup; \
	source.cleanup.next			= NULL; \
	source.dictionary.handler	= &source.handler; \
	error						= iinq_open_source(#source ".inq", &(source.dictionary), &(source.handler)); \
	if (err_ok != error) { \
		break; \
	} \
	source.key					= alloca(source.dictionary.instance->record.key_size); \
	source.value				= alloca(source.dictionary.instance->record.value_size); \
	source.ion_record.key		= source.key; \
	source.ion_record.value		= source.value; \
	result.num_bytes			+= source.dictionary.instance->record.key_size; \
	result.num_bytes			+= source.dictionary.instance->record.value_size; \
	error						= dictionary_build_predicate(&(source.predicate), predicate_all_records); \
	if (err_ok != error) { \
		break; \
	} \
	dictionary_find(&source.dictionary, &source.predicate, &source.cursor);

#define _FROM_WITH_SCHEMA_SINGLE(source) \
	struct iinq_ ## source ## _schema *source ## _tuple; \
	source ## _tuple = source.value;

#define _FROM_CHECK_CURSOR_SINGLE(source) \
	(cs_cursor_active == (source.cursor_status = source.cursor->next(source.cursor, &source.ion_record)) || cs_cursor_initialized == source.cursor_status)

#define _FROM_ADVANCE_CURSORS \
		if (NULL == ref_cursor) { \
			break; \
		} \
		last_cursor		= ref_cursor; \
		/* Keep going backwards through sources until we find one we can advance. If we re-initialize any cursors, reset ref_cursor to last. */ \
		while (NULL != ref_cursor && (cs_cursor_active != (ref_cursor->reference->cursor_status = ref_cursor->reference->cursor->next(ref_cursor->reference->cursor, &ref_cursor->reference->ion_record)) && cs_cursor_initialized != ref_cursor->reference->cursor_status)) { \
			ref_cursor->reference->cursor->destroy(&ref_cursor->reference->cursor); \
			dictionary_find(&ref_cursor->reference->dictionary, &ref_cursor->reference->predicate, &ref_cursor->reference->cursor); \
			if ((cs_cursor_active != (ref_cursor->reference->cursor_status = ref_cursor->reference->cursor->next(ref_cursor->reference->cursor, &ref_cursor->reference->ion_record)) && cs_cursor_initialized != ref_cursor->reference->cursor_status)) { \
				goto IINQ_QUERY_CLEANUP; \
			} \
			ref_cursor	= ref_cursor->last; \
		} \
		if (NULL == ref_cursor) { \
			break; \
		} \
		else if (last_cursor != ref_cursor) { \
			ref_cursor	= last; \
		}

/*
 * The last parameter, the variable arguments, is a black whole to swallow unused macro names.
 */
#define _FROM_SOURCE_GET_OVERRIDE(_1, _2, _3, _4, _5, _6, _7, _8, MACRO, ...) MACRO
/* Here we define a number of FROM macros to facilitate up to 8 sources. */
#define _FROM_SOURCE_1(_1) _FROM_SOURCE_SINGLE(_1)
#define _FROM_SOURCE_2(_1, _2) _FROM_SOURCE_1(_1) _FROM_SOURCE_1(_2)
#define _FROM_SOURCE_3(_1, _2, _3) _FROM_SOURCE_2(_1, _2) _FROM_SOURCE_1(_3)
#define _FROM_SOURCE_4(_1, _2, _3, _4) _FROM_SOURCE_3(_1, _2, _3) _FROM_SOURCE_1(_4)
#define _FROM_SOURCE_5(_1, _2, _3, _4, _5) _FROM_SOURCE_4(_1, _2, _3, _4) _FROM_SOURCE_1(_5)
#define _FROM_SOURCE_6(_1, _2, _3, _4, _5, _6) _FROM_SOURCE_5(_1, _2, _3, _4, _5) _FROM_SOURCE_1(_6)
#define _FROM_SOURCE_7(_1, _2, _3, _4, _5, _6, _7) _FROM_SOURCE_6(_1, _2, _3, _4, _5, _6) _FROM_SOURCE_1(_7)
#define _FROM_SOURCE_8(_1, _2, _3, _4, _5, _6, _7, _8) _FROM_SOURCE_7(_1, _2, _3, _4, _5, _6, _7) _FROM_SOURCE_1(_8)
/*
 * So this one is pretty ugly.
 *
 * We "slide" the correct macro based on the number of arguments. At the end, we add a comma so that we don't get a
 * compiler warning when only passing in only ONE argument into the variable argument list.
*/
#define _FROM_SOURCES(...) _FROM_SOURCE_GET_OVERRIDE(__VA_ARGS__, _FROM_SOURCE_8, _FROM_SOURCE_7, _FROM_SOURCE_6, _FROM_SOURCE_5, _FROM_SOURCE_4, _FROM_SOURCE_3, _FROM_SOURCE_2, _FROM_SOURCE_1, THEBLACKHOLE)(__VA_ARGS__)

/* Here we define a number of FROM macros to facilitate up to 8 sources, with schemas. */
#define _FROM_SOURCE_WITH_SCHEMA_1(_1) _FROM_SOURCE_SINGLE(_1) _FROM_WITH_SCHEMA_SINGLE(_1)
#define _FROM_SOURCE_WITH_SCHEMA_2(_1, _2) _FROM_SOURCE_WITH_SCHEMA_1(_1) _FROM_SOURCE_WITH_SCHEMA_1(_2)
#define _FROM_SOURCE_WITH_SCHEMA_3(_1, _2, _3) _FROM_SOURCE_WITH_SCHEMA_2(_1, _2) _FROM_SOURCE_WITH_SCHEMA_1(_3)
#define _FROM_SOURCE_WITH_SCHEMA_4(_1, _2, _3, _4) _FROM_SOURCE_WITH_SCHEMA_3(_1, _2, _3) _FROM_SOURCE_WITH_SCHEMA_1(_4)
#define _FROM_SOURCE_WITH_SCHEMA_5(_1, _2, _3, _4, _5) _FROM_SOURCE_WITH_SCHEMA_4(_1, _2, _3, _4) _FROM_SOURCE_WITH_SCHEMA_1(_5)
#define _FROM_SOURCE_WITH_SCHEMA_6(_1, _2, _3, _4, _5, _6) _FROM_SOURCE_WITH_SCHEMA_5(_1, _2, _3, _4, _5) _FROM_SOURCE_WITH_SCHEMA_1(_6)
#define _FROM_SOURCE_WITH_SCHEMA_7(_1, _2, _3, _4, _5, _6, _7) _FROM_SOURCE_WITH_SCHEMA_6(_1, _2, _3, _4, _5, _6) _FROM_SOURCE_WITH_SCHEMA_1(_7)
#define _FROM_SOURCE_WITH_SCHEMA_8(_1, _2, _3, _4, _5, _6, _7, _8) _FROM_SOURCE_WITH_SCHEMA_7(_1, _2, _3, _4, _5, _6, _7) _FROM_SOURCE_WITH_SCHEMA_1(_8)
/*
 * Like it's cousin above, this one is also ugly. We again leverage the sliding macro trick.
*/
#define _FROM_SOURCES_WITH_SCHEMA(...) _FROM_SOURCE_GET_OVERRIDE(__VA_ARGS__, _FROM_SOURCE_WITH_SCHEMA_8, _FROM_SOURCE_WITH_SCHEMA_7, _FROM_SOURCE_WITH_SCHEMA_6, _FROM_SOURCE_WITH_SCHEMA_5, _FROM_SOURCE_WITH_SCHEMA_4, _FROM_SOURCE_WITH_SCHEMA_3, _FROM_SOURCE_WITH_SCHEMA_2, _FROM_SOURCE_WITH_SCHEMA_1, THEBLACKHOLE)(__VA_ARGS__)

#define _FROM_CHECK_CURSOR(sources) \
	_FROM_CHECK_CURSOR_SINGLE(sources)

#define FROM(with_schemas, ...) \
	ion_iinq_cleanup_t	*first; \
	ion_iinq_cleanup_t	*last; \
	ion_iinq_cleanup_t	*ref_cursor; \
	ion_iinq_cleanup_t	*last_cursor; \
	first		= NULL; \
	last		= NULL; \
	ref_cursor	= NULL; \
	last_cursor	= NULL; \
	/*IF_ELSE((with_schema))(_FROM_SOURCES_WITH_SCHEMA(__VA_ARGS__))(_FROM_SOURCES(__VA_ARGS__));*/ \
	/*_FROM_SOURCES(__VA_ARGS__)*/; \
	IF_ELSE(with_schema)(_FROM_SOURCES_WITH_SCHEMA(__VA_ARGS__))(_FROM_SOURCES(__VA_ARGS__)); \
	result.data	= alloca(result.num_bytes); \
	ref_cursor	= first; \
	/* Initialize all cursors except the last one. */ \
	while (ref_cursor != last) { \
		if (NULL == ref_cursor || (cs_cursor_active != (ref_cursor->reference->cursor_status = ref_cursor->reference->cursor->next(ref_cursor->reference->cursor, &ref_cursor->reference->ion_record)) && cs_cursor_initialized != ref_cursor->reference->cursor_status)) { \
			break; \
		} \
		ref_cursor = ref_cursor->next; \
	} \
	ref_cursor	= last;

#define WHERE(condition) (condition)
#define HAVING(condition) (condition)

#define QUERY_SFW(select, from, where, limit, when, p) \
do { \
	ion_err_t			error; \
	ion_iinq_result_t	result; \
	result.num_bytes	= 0; \
	from/* This includes a loop declaration with some other stuff. */ \
	while (1) { \
		_FROM_ADVANCE_CURSORS \
		if (!where) { \
			continue; \
		} \
		select \
		(p)->execute(&result, (p)->state); \
	} \
	IINQ_QUERY_CLEANUP: \
	while (NULL != first) { \
		first->reference->cursor->destroy(&first->reference->cursor); \
		ion_close_dictionary(&first->reference->dictionary); \
		first			= first->next; \
	} \
} while (0);

#define QUERY(select, from, where, groupby, having, orderby, limit, when, p) \
do { \
	ion_err_t			error; \
	ion_iinq_result_t	result; \
	result.num_bytes	= 0; \
	from/* This includes a loop declaration with some other stuff. */ \
	while (1) { \
		_FROM_ADVANCE_CURSORS \
		if (!where) { \
			continue; \
		} \
		select \
		(p)->execute(&result, (p)->state); \
	} \
	IINQ_QUERY_CLEANUP: \
	while (NULL != first) { \
		first->reference->cursor->destroy(&first->reference->cursor); \
		ion_close_dictionary(&first->reference->dictionary); \
		first			= first->next; \
	} \
} while (0);

#define _AGGREGATES_INITIALIZE \
	for (i_agg = 0; i_agg < agg_n; i_agg++) { \
		aggregates[i_agg].status	= 0; /* TODO: Was .initialized */ \
    }

#define _AGGREGATES_SETUP(n) \
	agg_n	= (n);	\
	int i_agg	= 0;	\
	iinq_aggregate_t aggregates[agg_n]; \
	_AGGREGATES_INITIALIZE

/**
@brief		This is a helper macro for users. It gives the correct value
			of the i-th aggregate, based on the aggregates value type.
*/
#define AGGREGATE(i) (IINQ_AGGREGATE_TYPE_INT == aggregates[(i)].type ? aggregates[(i)].value.i64 : (IINQ_AGGREGATE_TYPE_UINT == aggregates[(i)].type ? aggregates[(i)].value.u64 : aggregates[(i)].value.f64))

/**
@brief		This is a helper macro, where we pass in which aggregate we are
			presently working with.
*/
#define _AGGREGATE_PRE_COMPUTE(n) \
	i_agg = n

#define MAX(expr) \
	if (0 == aggregates[i_agg].status || ((double)(expr)) > aggregates[i_agg].value.f64) { \
		aggregates[i_agg].value.f64 = ((double)(expr)); \
		aggregates[i_agg].status = 1; \
	}

#define MIN(expr) \
	if (0 == aggregates[i_agg].status || ((double)(expr)) < aggregates[i_agg].value.f64) { \
		aggregates[i_agg].value.f64 = ((double)(expr)); \
		aggregates[i_agg].status = 1; \
	}

/*
 * The last parameter, the variable arguments, is a black whole to swallow unused macro names.
 */
#define _AGGREGATES_GET_OVERRIDE(_1, _2, _3, _4, _5, _6, _7, _8, MACRO, ...) MACRO
#define _AGGREGATES_SINGLE(compute, n) _AGGREGATE_PRE_COMPUTE(n); compute
/* Here we define a number of macros to facilitate up to 8 total aggregate expressions. */
#define _AGGREGATES_1(_1) _AGGREGATES_SINGLE(_1, 0)
#define _AGGREGATES_2(_1, _2) _AGGREGATES_1(_1) _AGGREGATES_SINGLE(_2, 1)
#define _AGGREGATES_3(_1, _2, _3) _AGGREGATES_2(_1, _2) _AGGREGATES_SINGLE(_3, 2)
#define _AGGREGATES_4(_1, _2, _3, _4) _AGGREGATES_3(_1, _2, _3) _AGGREGATES_SINGLE(_4, 3)
#define _AGGREGATES_5(_1, _2, _3, _4, _5) _AGGREGATES_4(_1, _2, _3, _4) _AGGREGATES_SINGLE(_5, 4)
#define _AGGREGATES_6(_1, _2, _3, _4, _5, _6) _AGGREGATES_5(_1, _2, _3, _4, _5) _AGGREGATES_SINGLE(_6, 5)
#define _AGGREGATES_7(_1, _2, _3, _4, _5, _6, _7) _AGGREGATES_6(_1, _2, _3, _4, _5, _6) _AGGREGATES_SINGLE(_7, 6)
#define _AGGREGATES_8(_1, _2, _3, _4, _5, _6, _7, _8) _AGGREGATES_7(_1, _2, _3, _4, _5, _6, _7) _AGGREGATES_SINGLE(_8, 7)
/*
 * Like it's cousin above, this one is also ugly. We again leverage the sliding macro trick.
*/
#define _AGGREGATES(...) _AGGREGATES_GET_OVERRIDE(__VA_ARGS__, _AGGREGATES_8, _AGGREGATES_7, _AGGREGATES_6, _AGGREGATES_5, _AGGREGATES_4, _AGGREGATES_3, _AGGREGATES_2, _AGGREGATES_1, THEBLACKHOLE)(__VA_ARGS__)
#define AGGREGATES(...) \
	_AGGREGATES_SETUP(PP_NARG(__VA_ARGS__)); \
	goto IINQ_SKIP_COMPUTE_AGGREGATES; \
	IINQ_COMPUTE_AGGREGATES: ; \
	_AGGREGATES(__VA_ARGS__); \
	goto IINQ_DONE_COMPUTE_AGGREGATES; \
	IINQ_SKIP_COMPUTE_AGGREGATES: ;

#define _ORDERING_DECLARE(name) \
	int name ## _n				= 0;	\
	int i_ ## name				= 0;	\
	int total_ ## name ## _size	= 0; \
	iinq_order_part_t name ## _order_parts[(name ## _n)];

#define _ORDERING_SETUP(name, n) \
	name ## _n		= (n);	\
	i_ ## name		= 0;	\
	total_ ## name ## _size = 0;

#define _OPEN_ORDERING_FILE_WRITE(name, with_aggregates, record_size) \
	output_file			= fopen(#name, "wb"); \
	if (NULL == output_file) { \
		error			= err_file_open_error; \
		goto IINQ_QUERY_END; \
	} \
	write_page_remaining	= IINQ_PAGE_SIZE; \
	if ((int) write_page_remaining < (int) (total_ ## name ## _size + record_size IF_ELSE(with_aggregates)(+ (8*agg_n))())) { /* Record size is size of records, not including sort key. */ \
		/* In this case, there isn't enough space in a page to sort records. Fail. */ \
		error			= err_record_size_too_large; \
		goto IINQ_QUERY_END; \
	}

#define _OPEN_ORDERING_FILE_READ(name, with_aggregates, record_size) \
	input_file			= fopen(#name, "rb"); \
	if (NULL == input_file) { \
		error			= err_file_open_error; \
		goto IINQ_QUERY_END; \
	} \
	read_page_remaining	= IINQ_PAGE_SIZE; \
	/* The magic number 8 comes from the fact that all aggregate values are exactly 8 bytes big. */ \
	if ((int) read_page_remaining < (int) (total_ ## name ## _size + record_size IF_ELSE(with_aggregates)(+ (8*agg_n))())) { /* Record size is size of records, not including sort key. */ \
		/* In this case, there isn't enough space in a page to sort records. Fail. */ \
		error			= err_record_size_too_large; \
		goto IINQ_QUERY_END; \
	}

#define _CLOSE_ORDERING_FILE(f) \
	if (0 != fclose(f)) { \
		error			= err_file_close_error; \
		goto IINQ_QUERY_END; \
	}

#define _REMOVE_ORDERING_FILE(name) \
	if (0 != remove(#name)) { \
		error			= err_file_delete_error; \
		goto IINQ_QUERY_END; \
	}

#define _RENAME_ORDERING_FILE(old_name, new_name) \
	if (0 != rename(#old_name, #new_name)) { \
		error			= err_file_rename_error; \
		goto IINQ_QUERY_END; \
	}

#define _WRITE_ORDERING_RECORD(name, write_aggregates, record) \
	/* If the page runs out of room, fill the remaining space with zeroes. */ \
	/* Magic number 8 comes from fact that all aggregate values are 8 bytes in size. */ \
	if ((int) write_page_remaining < (int)(total_ ## name ## _size + record.num_bytes IF_ELSE(write_aggreates)(+ (8*agg_n))())) { /* Record size is size of records, not including sort key. */ \
		int		i = 0; \
		char	x = 0; \
		for (; i < write_page_remaining; i++) { \
			if (1 != fwrite(&x, 1, 1, output_file)) { \
				break; \
			} \
		} \
		write_page_remaining	= IINQ_PAGE_SIZE; \
	}; \
	/* Walk through each item in the order parts, write out data. */ \
	for (i_ ## name = 0; i_ ## name < name ## _n; i_ ## name ++) { \
		if (1 != fwrite(name ## _order_parts[ i_ ## name ].pointer, name ## _order_parts[ i_ ## name ].size, 1, output_file)) { \
			break; \
        } \
		else { \
			write_page_remaining	-= name ## _order_parts[ i_ ## name ].size; \
		} \
    } \
	/* If we require writing aggregates, do so. */ \
	IF_ELSE(write_aggregates)( \
		for (i_agg = 0; i_agg < agg_n; i_agg ++) { \
			if (1 != fwrite(&(uint64_t){AGGREGATE(i_agg)}, sizeof(AGGREGATE(i_agg)), 1, output_file)) { \
				break; \
			} \
			else { \
				write_page_remaining	-= sizeof(AGGREGATE(i_agg)); \
			} \
    	} \
	)() \
	if (1 != fwrite(record.data, record.num_bytes, 1, output_file)) { \
		break; \
	} \
	else { \
		write_page_remaining	-= record.num_bytes; \
	}

/*
 * We execute this once per ordering record (ORDERBY and GROUPBY clauses).
 *
 * execute_expr can be set to something if there needs to be something executed.
 */
#define _READ_ORDERING_RECORD(name, ordering_size, aggregate_data, key, record, execute_expr) \
	if ((int) read_page_remaining < (int)(ordering_size + record.num_bytes + ((NULL != aggregate_data) ? 8*agg_n : 0))) { /* Record size is size of records, not including sort key. */ \
		if (0 != fseek(input_file, read_page_remaining, SEEK_CUR)) { \
			break; \
		} \
	} \
	/* If the key is a non-null pointer. */ \
	if (NULL != key) {  \
		if (0 == ordering_size || 1 != fread((key), ordering_size, 1, input_file)) { \
			break; \
		} \
		else { \
			read_page_remaining -= ordering_size; \
        } \
	} \
	else { \
		if (0 != fseek(input_file, ordering_size, SEEK_CUR)) { \
			break; \
		} \
		else { \
			read_page_remaining -= ordering_size; \
        } \
	} \
	/* If the aggregate_data is a non-null pointer. */ \
	if (NULL != aggregate_data) {  \
		/* The magic number 8 again comes from the fact that all aggregate values are 8 bytes in size. */ \
		if (1 != fread((aggregate_data), 8*agg_n, 1, input_file)) { \
			break; \
		} \
		else { \
			read_page_remaining -= 8*agg_n; \
        } \
	} \
	if (1 != fread(&record, record.num_bytes, 1, input_file)) { \
		break; \
	} \
	read_page_remaining -= record.num_bytes; \
	/* Now record_data has the record. */ \
	execute_expr;

#define _ASCENDING_INDICATOR	 1
#define _DESCENDING_INDICATOR	-1

/*
 * This is a macro intended to be used with numerical and boolean expressions.
 */
#define _CREATE_MEMCPY_STACK_ADDRESS_FOR_NUMERICAL_EXPRESSION(expr) ( \
    8 == sizeof(expr) ? (void *)(&(uint64_t){(expr)}) : \
    ( \
        4 == sizeof(expr) ? (void *)(&(uint32_t){(expr)}) : \
        ( \
            2 == sizeof(expr) ? (void *)(&(uint16_t){(expr)}) : (void *)(&(uint8_t){(expr)}) \
        ) \
    ) \
)

/*
 * We need the ability to treat expressions resulting in
 */
#define ASCENDING(expr)		(expr, _CREATE_MEMCPY_STACK_ADDRESS_FOR_NUMERICAL_EXPRESSION(expr), sizeof((expr)), _ASCENDING_INDICATING)
#define ASC(expr)			ASCENDING(expr)
#define DESCENDING(expr)	(expr, _CREATE_MEMCPY_STACK_ADDRESS_FOR_NUMERICAL_EXPRESSION(expr), sizeof((expr)), _DESCENDING_INDICATING)
#define DESC(expr)			DESCENDING(expr)

#define _FIRST_MACRO_TUPLE4(_1, _2, _3, _4)		_1
#define _SECOND_MACRO_TUPLE4(_1, _2, _3, _4)	_2
#define _THIRD_MACRO_TUPLE4(_1, _2, _3, _4)		_3
#define _FOURTH_MACRO_TUPLE4(_1, _2, _3, _4)	_4

#define _SETUP_ORDERBY_SINGLE(t, n) \
	orderby_order_parts[(n)].direction	= _FOURTH_MACRO_TUPLE4 t; \
	orderby_order_parts[(n)].size		= _THIRD_MACRO_TUPLE4 t; \
	total_orderby_size					+= orderby_order_parts[(n)].size;

#define _PREPARE_ORDERING_KEY_ORDERBY_SINGLE(t, n) \
	orderby_order_parts[(n)].pointer	= _SECOND_MACRO_TUPLE4 t; \

#define ORDERBY(...) \
	_ORDERING_SETUP(orderby, PP_NARG(__VAR_ARGS__)) \
	/* Setup for each ordering part. */ \
	IINQ_ORDERBY_PREPARE: \
	do { \
		/* Setup code. */ \
		_PREPARE_ORDERING_KEYS(__VAR_ARGS__); \
		_WRITE_ORDERING_RECORD(orderby, recorddata); \
	} while(0); \
	goto IINQ_ORDERBY_AFTER; \
	IINQ_ORDERBY_SORT: \
	if (0 != fseek(output_file, 0, SEEK_SET)) { \
		goto IINQ_CLEANUP_QUERY; \
    } \
	input_file	= output_file; \
	output_file	= NULL; \
	/* Call Wade sort here. */ \
	IINQ_ORDERBY_AFTER: ; \

#define _GROUPBY_SETUP(n) \
	groupby_n	= (n);	\

#define _GROUPBY_SINGLE(_1) apple
#define _GROUPBY_1(_1) _GROUPBY_SINGLE(_1)
#define GROUPBY(...) \
	_GROUPBY_SETUP(PP_NARG(__VA_ARGS__)); \

#define MATERIALIZED_QUERY(select_clause, aggregate_exprs, from_clause, where_clause, groupby_clause, having_clause, orderby_clause, limit, when, p) \
do { \
	ion_err_t			error; \
	int					read_page_remaining		= IINQ_PAGE_SIZE; \
	int					write_page_remaining	= IINQ_PAGE_SIZE; \
	FILE				*input_file; \
	FILE				*output_file; \
	ion_iinq_result_t	result; \
    result.num_bytes							= 0; \
	int agg_n									= 0; \
	from_clause/* This includes a loop declaration with some other stuff. */ \
	_ORDERING_DECLARE(groupby) \
	_ORDERING_DECLARE(orderby) \
    aggregate_exprs \
    groupby_clause \
    orderby_clause \
	/* We wrap the FROM code in a do-while(0) to limit the lifetime of the source variables. */ \
    do { \
		if (agg_n > 0) { \
			/* Write out the aggregate records to disk for sorting. */ \
			_OPEN_ORDERING_FILE_WRITE(groupby, 0, result.num_bytes) \
		} \
		else if (groupby_n > 0) { \
			/* Error case where we have GROUPBY elements but no aggregates. */ \
			error	= err_illegal_state; \
			goto IINQ_QUERY_END; \
		} \
		else if (orderby_n > 0) { \
			_OPEN_ORDERING_FILE_WRITE(orderby, 0, result.num_bytes) \
		} \
		while (1) { \
			_FROM_ADVANCE_CURSORS \
            if (!where_clause) { \
                continue; \
            } \
			select_clause \
			/* If there are grouping/aggregate attributes. */ \
			if (agg_n > 0) { \
				/* Write out the groupby records to disk for sorting. */ \
				_WRITE_ORDERING_RECORD(groupby, 0, result) \
			} \
			else if (orderby_n > 0) { \
				_WRITE_ORDERING_RECORD(orderby, 0, result) \
			} \
			else { \
				/* Proceed with projection, no group by or order by. */ \
				(p)->execute(&result, (p)->state); \
			} \
        } \
		IINQ_QUERY_CLEANUP: \
		if (agg_n > 0 || orderby_n > 0) { \
            _CLOSE_ORDERING_FILE(output_file); \
        } \
		while (NULL != first) { \
			first->reference->cursor->destroy(&first->reference->cursor); \
			ion_close_dictionary(&first->reference->dictionary); \
			first			= first->next; \
		} \
    } while (0); \
	/* If we have both aggregates and a groupby elements, we must sort the file. */ \
	if (agg_n > 0 && groupby_n > 0) { \
		/* TODO: Wade sort output_file. */ \
		_OPEN_ORDERING_FILE_READ(groupby, 0, result.num_bytes); \
		int total_temp_size = total_groupby_size; \
		_OPEN_ORDERING_FILE_WRITE(temp, 0, result.num_bytes); \
		ion_external_sort_t	es; \
		iinq_sort_context_t context = _IINQ_SORT_CONTEXT(groupby); \
		if (err_ok != (error = ion_external_sort_init(&es, input_file, &context, iinq_sort_compare, result.num_bytes, result.num_bytes, IINQ_PAGE_SIZE, boolean_false, ION_FILE_SORT_FLASH_MINSORT))) { /* TODO: remove key_size */ \
			_CLOSE_ORDERING_FILE(input_file); \
			_CLOSE_ORDERING_FILE(output_file); \
			goto IINQ_QUERY_END; \
		} \
		uint16_t buffer_size = ion_external_sort_bytes_of_memory_required(&es, 0, boolean_true); \
		char buffer[buffer_size]; \
		if (err_ok != (error = ion_external_sort_dump_all(&es, output_file, buffer, buffer_size))) { \
			_CLOSE_ORDERING_FILE(input_file); \
			_CLOSE_ORDERING_FILE(output_file); \
			goto IINQ_QUERY_END; \
        } \
		_CLOSE_ORDERING_FILE(input_file); \
		_CLOSE_ORDERING_FILE(output_file); \
		_REMOVE_ORDERING_FILE(groupby); \
		_RENAME_ORDERING_FILE(temp, groupby); \
    } \
	/* Aggregates and GROUPBY handling. */ \
	if (agg_n > 0) { \
		_OPEN_ORDERING_FILE_READ(groupby, 0, result.num_bytes); \
		/* Note that if there is no orderby, then we simply will read these values off disk when we are done (no sort). */ \
		_OPEN_ORDERING_FILE_WRITE(orderby, 1, result.num_bytes); \
		ion_boolean_t	is_first		= boolean_true; \
		/* We need to track two keys. We need to be able to compare the last key seen to the next
		 * to know if the next key is equal (and is thus part of the same grouping attribute. */ \
		char		*old_key			= alloca(total_groupby_size);/*[total_groupby_size];*/ \
		char		*cur_key		= alloca(total_groupby_size);/*[total_groupby_size];*/ \
		/* While we have more records in the sorted group by file, read them, check if keys are the same. */ \
		read_page_remaining			= IINQ_PAGE_SIZE; \
		result.data					= alloca(result.num_bytes); \
		while (1) { \
			_READ_ORDERING_RECORD(groupby, total_groupby_size, NULL, cur_key, result, /* Empty on purpose. */) \
			/* TODO: Graeme, make sure you setup all necessary error codes in above and related, as well as handle them here. */ \
			if (total_groupby_size > 0 && !is_first && equal != iinq_sort_compare(&_IINQ_SORT_CONTEXT(groupby), cur_key, old_key)) { \
				_WRITE_ORDERING_RECORD(orderby, 1, result) \
				_AGGREGATES_INITIALIZE \
            } \
			goto IINQ_COMPUTE_AGGREGATES; \
			IINQ_DONE_COMPUTE_AGGREGATES:; \
			memcpy(old_key, cur_key, total_groupby_size); \
			is_first				= boolean_false; \
        } \
		_WRITE_ORDERING_RECORD(orderby, 1, result) \
		_CLOSE_ORDERING_FILE(output_file) \
		_CLOSE_ORDERING_FILE(input_file) \
    } \
	/* ORDERBY handling. Do this anytime we have ORDERBY or aggregates, since we abuse the use of orderby file to accomodate when we have aggregates but no orderby. */ \
	if (orderby_n > 0 || agg_n > 0) { \
		/* TODO: Think about making this the select + the aggregates at the end. We have the values, just process them together. Might be hard? */ \
		result.data			= alloca(result.num_bytes); \
		/* We can safely ALWAYS open with aggregates, because it doesn't increase the size if no aggregates exist (0*8 == 0). */ \
		/* TODO: Wade sort the orderby file. Using the cursor style. */ \
		_OPEN_ORDERING_FILE_READ(orderby, 1, result.num_bytes); \
		ion_external_sort_t	es; \
		iinq_sort_context_t context = _IINQ_SORT_CONTEXT(orderby); \
		if (err_ok != (error = ion_external_sort_init(&es, input_file, &context, iinq_sort_compare, result.num_bytes, result.num_bytes, IINQ_PAGE_SIZE, boolean_false, ION_FILE_SORT_FLASH_MINSORT))) { \
			_CLOSE_ORDERING_FILE(input_file); \
			goto IINQ_QUERY_END; \
		} \
		uint16_t buffer_size = ion_external_sort_bytes_of_memory_required(&es, 0, boolean_false); \
		char buffer[buffer_size]; \
		ion_external_sort_cursor_t cursor; \
		if (err_ok != (error = ion_external_sort_init_cursor(&es, &cursor, buffer, buffer_size))) { \
			_CLOSE_ORDERING_FILE(input_file); \
			goto IINQ_QUERY_END; \
        } \
		_CLOSE_ORDERING_FILE(input_file); \
		/* TODO: Store each record in result, then the following code SHOULD work: */ \
		/* TODO: We need to pass in the aggregates into the result, somehow. Godspeed. */ \
		if (err_ok != (error = cursor.next(&cursor, result.data))) { \
			_CLOSE_ORDERING_FILE(input_file); \
			goto IINQ_QUERY_END; \
		} \
		while (cs_cursor_active == cursor.status) { \
			(p)->execute(&result, (p)->state); \
			if (err_ok != (error = cursor.next(&cursor, result.data))) { \
				_CLOSE_ORDERING_FILE(input_file); \
				goto IINQ_QUERY_END; \
			} \
        } \
    } \
	\
	IINQ_QUERY_END: ; \
} while (0);

#if defined(__cplusplus)
}
#endif

#endif
/* *INDENT-ON* */
