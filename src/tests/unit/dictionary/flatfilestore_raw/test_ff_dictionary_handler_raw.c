#include <stdio.h>
#include <string.h>
#include "dicttypes.h"
#include "dictionary.h"
#include "flat_file_raw.h"
#include "ff_dictionary_handler_raw.h"
#include "CuTest.h"

/**
@brief		A helper function to build a test collection

@param      map_handler
@param      record
@param      size
@param      test_dictionary
*/
void
createFlatFileTestCollection(
	dictionary_handler_t	*map_handler,
	const record_info_t		*record,
	int						size,
	dictionary_t			*test_dictionary,
	key_type_t				key_type
)
{
	ff_file_t flat_file;

	ffdict_init(map_handler);
	dictionary_create(map_handler, test_dictionary, key_type, record->key_size, record->value_size, size);

	int32_t		i;
	ion_value_t str;

	str = (ion_value_t) malloc(sizeof(ion_value_t) * record->value_size);

	for (i = 0; i < size; i++)
	{
		sprintf((char *) str, "value: %02i", i);
		test_dictionary->handler->insert(test_dictionary, (ion_key_t) &i, str);
	}

	free(str);
}

/**
@brief		Validates the correct registration of function pointers
            for open address dictionary structure.

@param      tc
                CuTest
 */
void
test_flat_file_handler_function_registration(
	CuTest		*tc
)
{
	dictionary_handler_t map_handler;			/* create handler for hashmap */

	ffdict_init(&map_handler);					/* register handler for hashmap */

	/* check to ensure that function pointers are correctly registered */
	CuAssertTrue(tc, map_handler.insert == &ffdict_insert);
	CuAssertTrue(tc, map_handler.create_dictionary == &ffdict_create_dictionary);
	CuAssertTrue(tc, map_handler.update == &ffdict_update);
	CuAssertTrue(tc, map_handler.remove == &ffdict_delete);
	CuAssertTrue(tc, map_handler.delete_dictionary == &ffdict_delete_dictionary);
}

/**
@brief      Tests for creatation and deletion of open address hash.

@param      tc
                CuTest
 */
void
test_flat_file_handler_create_destroy(
	CuTest		*tc
)
{
	record_info_t record;

	/* this is required for initializing the hash map and should come from the dictionary */
	record.key_size		= 4;
	record.value_size	= 10;

	dictionary_handler_t map_handler;			/* create handler for hashmap */

	ffdict_init(&map_handler);					/* register handler for hashmap */

	/* collection handler for test collection */
	dictionary_t test_dictionary;

	/* register the appropriate handler for a given collection */
	dictionary_create(&map_handler, &test_dictionary, key_type_numeric_signed, record.key_size, record.value_size, 0);

	CuAssertIntEqual(tc, (((ff_file_t *) test_dictionary.instance)->super.record.key_size), record.key_size);
	CuAssertIntEqual(tc, (((ff_file_t *) test_dictionary.instance)->super.record.value_size), record.value_size);
	CuAssertTrue(tc, (((ff_file_t *) test_dictionary.instance)->write_concern) == wc_insert_unique);

	ff_file_t file;

	CuAssertIntEqual(tc, sd_raw_read(0, &(file.super), sizeof(file.super), SIZEOF(STATUS)), 0);
	CuAssertIntEqual(tc, record.key_size, file.super.record.key_size);
	CuAssertIntEqual(tc, record.value_size, file.super.record.value_size);
	CuAssertIntEqual(tc, key_type_numeric_signed, file.super.key_type);
	CuAssertTrue(tc, dictionary_compare_signed_value == file.super.compare);

	/* delete the dictionary */
	CuAssertTrue(tc, err_ok == test_dictionary.handler->delete_dictionary(&test_dictionary));
	CuAssertTrue(tc, test_dictionary.instance == NULL);
}

/**
@brief      Tests a simple insert into dictionary and simple query

@details	Tests a simple insert into dictionary and simple query with the
            write_concern set to insert only
@param      tc
                CuTest
 */
void
test_flat_file_handler_simple_insert(
	CuTest		*tc
)
{
	record_info_t record;

	/* this is required for initializing the hash map and should come from the dictionary */
	record.key_size		= 4;
	record.value_size	= 10;

	int32_t					i = 0;
	char					test_value[record.value_size];
	dictionary_handler_t	dict_handler;		/* create handler for hashmap */

	ffdict_init(&dict_handler);					/* register handler for hashmap */

	/* collection handler for test collection */
	dictionary_t test_dictionary;

	/* register the appropriate handler for a given collection */
	dictionary_create(&dict_handler, &test_dictionary, key_type_numeric_signed, record.key_size, record.value_size, 0);
	sprintf((char *) test_value, "value: %02i", i);
	CuAssertTrue(tc, err_ok == test_dictionary.handler->insert(&test_dictionary, (ion_key_t) &i, (ion_value_t) test_value));

	f_file_record_t *file_record;	/* this needs to be dynamically allocated */

	int record_size = SIZEOF(STATUS) + test_dictionary.instance->record.key_size + test_dictionary.instance->record.value_size;

	file_record = (f_file_record_t *) malloc(sizeof(char) * record_size);

	/* read the record_info back and check */
	CuAssertTrue(tc, 0 == sd_raw_read(1, file_record, record_size, 0));
	CuAssertTrue(tc, IN_USE == file_record->status);
	CuAssertTrue(tc, 0 == memcmp((char *) (file_record->data + test_dictionary.instance->record.key_size), test_value, test_dictionary.instance->record.value_size));
	CuAssertTrue(tc, i == *(int32_t *) (file_record->data));

	for (i = 1; i < 3; i++)
	{
		sprintf((char *) test_value, "value: %02i", i);
		CuAssertTrue(tc, err_ok == test_dictionary.handler->insert(&test_dictionary, (ion_key_t) &i, (ion_value_t) test_value));

		/* read the record_info back and check */
		CuAssertTrue(tc, 0 == sd_raw_read(1, file_record, record_size, record_size * i));

		/* check status */
		CuAssertTrue(tc, IN_USE == file_record->status);

		/* check value */
		CuAssertTrue(tc, 0 == memcmp((char *) (file_record->data + test_dictionary.instance->record.key_size), test_value, test_dictionary.instance->record.value_size));

		/* check key */
		CuAssertTrue(tc, i == *(int32_t *) (file_record->data));
	}

	/* delete the dictionary */
	CuAssertTrue(tc, err_ok == test_dictionary.handler->delete_dictionary(&test_dictionary));
	CuAssertTrue(tc, test_dictionary.instance == NULL);
	free(file_record);
}

void
test_flat_file_dictionary_cursor_equality(
	CuTest		*tc
)
{
	int				size;
	record_info_t	record;

	/* this is required for initializing the hash map and should come from the dictionary */
	record.key_size		= 4;
	record.value_size	= 10;
	size				= 10;

	dictionary_handler_t file_handler;				/* create handler for hashmap */

	dictionary_t test_dictionary;					/* collection handler for test collection */

	createFlatFileTestCollection(&file_handler, &record, size, &test_dictionary, key_type_numeric_signed);

	dict_cursor_t *cursor;						/* create a new cursor pointer */

	/* create a new predicate statement */
	predicate_t predicate;
	dictionary_build_predicate(&predicate, predicate_equality, IONIZE(1));

	/* test that the query runs on collection okay */
	CuAssertTrue(tc, err_ok == test_dictionary.handler->find(&test_dictionary, &predicate, &cursor));

	/* check the status of the cursor as it should be initialized */
	CuAssertTrue(tc, cs_cursor_initialized == cursor->status);

	/* destroy the cursor */
	cursor->destroy(&cursor);

	/* and check that cursor has been destroyed correctly */
	CuAssertTrue(tc, NULL == cursor);

	/* and destory the collection */
	test_dictionary.handler->delete_dictionary(&test_dictionary);
}

void
test_flat_file_dictionary_handler_query_with_results(
	CuTest		*tc
)
{
	int				size;
	record_info_t	record_info;

	/* this is required for initializing the hash map and should come from the dictionary */
	record_info.key_size	= 4;
	record_info.value_size	= 10;
	size					= 10;

	dictionary_handler_t map_handler;				/* create handler for hashmap */

	dictionary_t test_dictionary;					/* collection handler for test collection */

	createFlatFileTestCollection(&map_handler, &record_info, size, &test_dictionary, key_type_numeric_signed);

	dict_cursor_t *cursor;							/* create a new cursor pointer */

	/* create a new predicate statement */
	predicate_t predicate;
	dictionary_build_predicate(&predicate, predicate_equality, IONIZE(1));

	/* test that the query runs on collection okay */
	CuAssertTrue(tc, err_ok == test_dictionary.handler->find(&test_dictionary, &predicate, &cursor));

	/* check the status of the cursor as it should be initialized */
	CuAssertTrue(tc, cs_cursor_initialized == cursor->status);

	/* user must allocate memory before calling next() */
	ion_record_t record;

	record.value	= (ion_value_t) malloc(sizeof(ion_value_t) * record_info.value_size);
	record.key		= (ion_key_t) malloc(sizeof(ion_value_t) * record_info.key_size);
	CuAssertTrue(tc, cs_cursor_active == cursor->next(cursor, &record));

	/* check that value is correct that has been returned */
	ion_value_t str;

	str = (ion_value_t) malloc(sizeof(ion_value_t) * record_info.value_size);

	sprintf((char *) str, "value: %02i", *(int32_t *) predicate.statement.equality.equality_value);
	CuAssertTrue(tc, IS_EQUAL == memcmp(record.value, str, record_info.value_size));

	free(str);

	/* and as there is only 1 result, the next call should return empty */
	CuAssertTrue(tc, cs_end_of_results == cursor->next(cursor, &record));

	/* and as there is only 1 result, the next call should return empty */
	CuAssertTrue(tc, cs_end_of_results == cursor->next(cursor, &record));

	free(record.value);
	free(record.key);

	/* destory cursor for cleanup */
	cursor->destroy(&cursor);

	/* and destory the collection */
	test_dictionary.handler->delete_dictionary(&test_dictionary);
}

void
test_flat_file_dictionary_handler_query_no_results(
	CuTest		*tc
)
{
	int				size;
	record_info_t	record_info;

	/* this is required for initializing the hash map and should come from the dictionary */
	record_info.key_size	= 4;
	record_info.value_size	= 10;
	size					= 10;

	dictionary_handler_t map_handler;				/* create handler for hashmap */

	dictionary_t test_dictionary;					/* collection handler for test collection */

	createFlatFileTestCollection(&map_handler, &record_info, size, &test_dictionary, key_type_numeric_signed);

	dict_cursor_t *cursor;							/* create a new cursor pointer */

	/* create a new predicate statement */
	predicate_t predicate;
	dictionary_build_predicate(&predicate, predicate_equality, IONIZE(-1));

	/* test that the query runs on collection okay */
	CuAssertTrue(tc, err_ok == test_dictionary.handler->find(&test_dictionary, &predicate, &cursor));

	/* check the status of the cursor as it should be at the end of results as no values exist */
	CuAssertTrue(tc, cs_end_of_results == cursor->status);

	/* user must allocate memory before calling next() */
	ion_record_t record;

	record.key		= (ion_key_t) malloc(sizeof(ion_key_t) * record_info.value_size);
	record.value	= (ion_value_t) malloc(sizeof(ion_value_t) * record_info.value_size);

	CuAssertTrue(tc, cs_end_of_results == cursor->next(cursor, &record));
	
	free(record.key);
	free(record.value);

	/* destroy cursor for cleanup */
	cursor->destroy(&cursor);

	/* and destroy the collection */
	test_dictionary.handler->delete_dictionary(&test_dictionary);
}

void
test_flat_file_dictionary_predicate_equality(
	CuTest		*tc
)
{
	ion_key_t key_under_test;

	key_under_test = (ion_key_t) malloc(sizeof(int32_t));

	int				size;
	record_info_t	record_info;

	/* this is required for initializing the hash map and should come from the dictionary */
	record_info.key_size	= 4;
	record_info.value_size	= 10;
	size					= 10;

	dictionary_handler_t map_handler;				/* create handler for hashmap */

	dictionary_t test_dictionary;					/* collection handler for test collection */

	createFlatFileTestCollection(&map_handler, &record_info, size, &test_dictionary, key_type_numeric_signed);

	dict_cursor_t *cursor;							/* create a new cursor pointer */

	cursor			= (dict_cursor_t *) malloc(sizeof(dict_cursor_t));
	cursor->destroy = ffdict_destroy_cursor;

	/* create a new predicate statement */
	predicate_t predicate;
	dictionary_build_predicate(&predicate, predicate_equality, IONIZE(1));

	cursor->dictionary											= &test_dictionary;	/* register test dictionary */
	cursor->predicate											= &predicate;	/* register predicate */

	*((int32_t *) key_under_test) = 1;
	CuAssertTrue(tc, boolean_true == ffdict_test_predicate(cursor, key_under_test));

	*((int32_t *) key_under_test) = 2;
	CuAssertTrue(tc, boolean_false == ffdict_test_predicate(cursor, key_under_test));

	*((int32_t *) key_under_test) = -1;
	CuAssertTrue(tc, boolean_false == ffdict_test_predicate(cursor, key_under_test));

	free(key_under_test);

	/* free up the correct predicate */
	free(cursor);

	test_dictionary.handler->delete_dictionary(&test_dictionary);
}

void
test_flat_file_dictionary_predicate_range_signed(
	CuTest		*tc
)
{
	ion_key_t key_under_test;

	key_under_test = (ion_key_t) malloc(sizeof(int32_t));

	int				size;
	record_info_t	record_info;

	/* this is required for initializing the hash map and should come from the dictionary */
	record_info.key_size	= 4;
	record_info.value_size	= 10;
	size					= 10;

	dictionary_handler_t map_handler;				/* create handler for hashmap */

	dictionary_t test_dictionary;					/* collection handler for test collection */

	createFlatFileTestCollection(&map_handler, &record_info, size, &test_dictionary, key_type_numeric_signed);

	dict_cursor_t *cursor;							/* create a new cursor pointer */

	cursor			= (dict_cursor_t *) malloc(sizeof(dict_cursor_t));
	cursor->destroy = ffdict_destroy_cursor;

	/* create a new predicate statement */
	predicate_t predicate;
	dictionary_build_predicate(&predicate, predicate_range, IONIZE(-1), IONIZE(1));

	cursor->dictionary				= &test_dictionary;		/* register test dictionary */
	cursor->predicate				= &predicate;			/* register predicate */
	*((int32_t *) key_under_test)	= 0;

	CuAssertTrue(tc, boolean_true == ffdict_test_predicate(cursor, key_under_test));
	*((int32_t *) key_under_test) = -1;

	CuAssertTrue(tc, boolean_true == ffdict_test_predicate(cursor, key_under_test));
	*((int32_t *) key_under_test) = 1;

	CuAssertTrue(tc, boolean_true == ffdict_test_predicate(cursor, key_under_test));
	*((int32_t *) key_under_test) = 2;

	CuAssertTrue(tc, boolean_false == ffdict_test_predicate(cursor, key_under_test));
	*((int32_t *) key_under_test) = -2;

	CuAssertTrue(tc, boolean_false == ffdict_test_predicate(cursor, key_under_test));
	free(key_under_test);

	free(cursor);

	test_dictionary.handler->delete_dictionary(&test_dictionary);
}

void
test_flat_file_dictionary_predicate_range_unsigned(
	CuTest		*tc
)
{
	ion_key_t key_under_test;

	key_under_test = (ion_key_t) malloc(sizeof(unsigned int));

	int				size;
	record_info_t	record_info;

	/* this is required for initializing the hash map and should come from the dictionary */
	record_info.key_size	= 4;
	record_info.value_size	= 10;
	size					= 10;

	dictionary_handler_t map_handler;				/* create handler for hashmap */

	dictionary_t test_dictionary;					/* collection handler for test collection */

	createFlatFileTestCollection(&map_handler, &record_info, size, &test_dictionary, key_type_numeric_unsigned);

	dict_cursor_t *cursor;							/* create a new cursor pointer */

	cursor			= (dict_cursor_t *) malloc(sizeof(dict_cursor_t));
	cursor->destroy = ffdict_destroy_cursor;

	/* create a new predicate statement */
	predicate_t predicate;
	dictionary_build_predicate(&predicate, predicate_range, IONIZE(0), IONIZE(2));

	cursor->dictionary					= &test_dictionary;	/* register test dictionary */
	cursor->predicate					= &predicate;		/* register predicate */

	memcpy(key_under_test, (ion_key_t) &(unsigned int) {0}, sizeof(unsigned int));
	CuCuAssertTrue(tc, tc, boolean_true == ffdict_test_predicate(cursor, key_under_test));

	memcpy(key_under_test, (ion_key_t) &(unsigned int) {1}, sizeof(unsigned int));
	CuCuAssertTrue(tc, tc, boolean_true == ffdict_test_predicate(cursor, key_under_test));

	memcpy(key_under_test, (ion_key_t) &(unsigned int) {2}, sizeof(unsigned int));
	CuCuAssertTrue(tc, tc, boolean_true == ffdict_test_predicate(cursor, key_under_test));

	memcpy(key_under_test, (ion_key_t) &(unsigned int) {3}, sizeof(unsigned int));
	CuCuAssertTrue(tc, tc, boolean_false == ffdict_test_predicate(cursor, key_under_test));

	memcpy(key_under_test, (ion_key_t) &(unsigned int) {4}, sizeof(unsigned int));
	CuCuAssertTrue(tc, tc, boolean_false == ffdict_test_predicate(cursor, key_under_test));

	free(key_under_test);

	test_dictionary.handler->delete_dictionary(&test_dictionary);
}

void
test_flat_file_dictionary_cursor_range(
	CuTest		*tc
)
{
	int				size;
	record_info_t	record_info;

	/* this is required for initializing the hash map and should come from the dictionary */
	record_info.key_size	= 4;
	record_info.value_size	= 10;
	size					= 10;

	dictionary_handler_t map_handler;				/* create handler for hashmap */

	dictionary_t test_dictionary;					/* collection handler for test collection */

	createFlatFileTestCollection(&map_handler, &record_info, size, &test_dictionary, key_type_numeric_signed);

	dict_cursor_t *cursor;						/* create a new cursor pointer */

	/* create a new predicate statement */
	predicate_t predicate;
	dictionary_build_predicate(&predicate, predicate_range, IONIZE(1), IONIZE(5));

	/* test that the query runs on collection okay */
	CuCuAssertTrue(tc, tc, err_ok == test_dictionary.handler->find(&test_dictionary, &predicate, &cursor));

	/* check the status of the cursor as it should be initialized */
	CuCuAssertTrue(tc, tc, cs_cursor_initialized == cursor->status);

	/* user must allocate memory before calling next() */
	ion_record_t record;

	record.key		= (ion_key_t) malloc(record_info.key_size);
	record.value	= (ion_value_t) malloc(record_info.value_size);

	int			result_count = 0;
	status_t	cursor_status;

	while (cs_cursor_active == (cursor_status = cursor->next(cursor, &record)))
	{
		CuCuAssertTrue(tc, tc, cs_cursor_active == cursor_status);

		/* check that value is correct that has been returned */
		ion_value_t str;

		str = (ion_value_t) malloc(record_info.value_size);

		sprintf((char *) str, "value : %i ", (*(int *) predicate.statement.range.geq_value) + result_count);
		CuCuAssertTrue(tc, tc, IS_EQUAL == memcmp(record.value, str, record_info.value_size));
		
		result_count++;

		free(str);
	}

	CuCuAssertTrue(tc, tc, 5 == result_count);

	/* and as there is only 1 result, the next call should return empty */
	CuCuAssertTrue(tc, tc, cs_end_of_results == cursor->next(cursor, &record));

	/* free up record */
	free(record.key);
	free(record.value);

	/* destroy the cursor */
	cursor->destroy(&cursor);

	/* and check that cursor has been destroyed correctly */
	CuCuAssertTrue(tc, tc, NULL == cursor);

	/* and destory the collection */
	test_dictionary.handler->delete_dictionary(&test_dictionary);
}

CuSuite *
flat_file_handler_getsuite(
)
{
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, test_flat_file_handler_function_registration);
	SUITE_ADD_TEST(suite, test_flat_file_handler_create_destroy);
	SUITE_ADD_TEST(suite, test_flat_file_handler_simple_insert);

	/** @tdodo need simple query */
	SUITE_ADD_TEST(suite, test_flat_file_dictionary_predicate_equality);
	SUITE_ADD_TEST(suite, test_flat_file_dictionary_predicate_range_signed);
	SUITE_ADD_TEST(suite, test_flat_file_dictionary_predicate_range_unsigned);
	SUITE_ADD_TEST(suite, test_flat_file_dictionary_cursor_equality);
	SUITE_ADD_TEST(suite, test_flat_file_dictionary_handler_query_with_results);
	SUITE_ADD_TEST(suite, test_flat_file_dictionary_handler_query_no_results);
	SUITE_ADD_TEST(suite, test_flat_file_dictionary_cursor_range);

	return suite;
}

void
runalltests_flat_file_handler(
)
{
	int error = 0;

	if (error = sd_raw_init(4))
	{
		Serial.print(F("SD initialization failed. Error code: "));
		Serial.println(error);
	}

	CuString	*output = CuStringNew();
	CuSuite		*suite	= flat_file_handler_getsuite();

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
	CuSuiteDelete(suite);
	CuStringDelete(output);
}