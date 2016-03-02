
#include "./../generic_dictionary_test.h"
#include "./../../../../dictionary/bpptree/bpptreehandler.h"

void
run_bpptreehandler_generic_test_set_1(
	planck_unit_test_t		*tc
)
{
	generic_test_t	test;
	
	init_generic_dictionary_test(
		&test,
		bpptree_init,
		key_type_numeric_signed,
		sizeof(int),
		sizeof(int),
		-1		/* Dictionary size, for now is unbounded. */
	);
	
	dictionary_test_init(&test, tc);
	
	dictionary_test_insert_get(
		&test,
		10000,
		tc
	);

	dictionary_test_insert_get_edge_cases(&test, tc);

	int to_delete[] = {7, -9, 32, 1000001};
	int i;
	
	for (i = 0; i < sizeof(to_delete)/sizeof(int); i++)
	{
		dictionary_test_delete(
			&test,
			(ion_key_t)(&(to_delete[i])),
			tc
		);
	}
	
	int update_key;
	int update_value;
	
	update_key	= 1;
	update_value	= -12;
	
	dictionary_test_update(
		&test,
		(ion_key_t)(&update_key),
		(ion_value_t)(&update_value),
		tc
	);
	
	update_key	= 1;
	update_value	= 12;
	
	dictionary_test_update(
		&test,
		(ion_key_t)(&update_key),
		(ion_value_t)(&update_value),
		tc
	);
	
	update_key	= 12;
	update_value	= 1;
	
	dictionary_test_update(
		&test,
		(ion_key_t)(&update_key),
		(ion_value_t)(&update_value),
		tc
	);

	dictionary_insert(&test.dictionary, IONIZE(5),  IONIZE(3));
	dictionary_insert(&test.dictionary, IONIZE(5),  IONIZE(5));
	dictionary_insert(&test.dictionary, IONIZE(5),  IONIZE(7));

	dictionary_insert(&test.dictionary, IONIZE(-5),  IONIZE(14));
	dictionary_insert(&test.dictionary, IONIZE(-7),  IONIZE(6));
	dictionary_insert(&test.dictionary, IONIZE(-10),  IONIZE(23));
	dictionary_insert(&test.dictionary, IONIZE(-205),  IONIZE(9));

	dictionary_test_equality(
	    &test,
	    IONIZE(5),
	    tc
    );

	dictionary_test_equality(
	    &test,
	    IONIZE(-10),
	    tc
    );

	dictionary_test_range(
	    &test,
	    IONIZE(5),
	    IONIZE(3777),
	    tc
    );

	dictionary_test_range(
	    &test,
	    IONIZE(-5),
	    IONIZE(3777),
	    tc
    );

	dictionary_test_all_records(
	    &test,
	    10006,
	    tc
    );

    dictionary_test_open_close(&test, tc);

	cleanup_generic_dictionary_test(&test);
}

planck_unit_suite_t*
bpptreehandler_get_suite(
)
{
	planck_unit_suite_t *suite = planck_unit_new_suite();

	planck_unit_add_to_suite(suite, run_bpptreehandler_generic_test_set_1);

	return suite;
}

void
run_all_tests_bpptreehandler(
)
{
	planck_unit_suite_t		*suite	= bpptreehandler_get_suite();
	
	planck_unit_run_suite(suite);
}
