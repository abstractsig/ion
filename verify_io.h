/*
 *
 * Verify Io
 *
 * A unit test library for testing io
 *
 * LICENSE
 * See end of file for license terms.
 *
 * USAGE
 * Include this file in whatever places need to refer to it.
 * In one C/C++ define IMPLEMENT_VERIFY_IO_CORE.
 *
 */
#ifndef verify_io_core_H_
#define verify_io_core_H_
#include <io_core.h>

typedef struct test_runner V_runner_t;
typedef void (*V_test_t) (V_runner_t*);

typedef enum {
	VERIFY_UNIT_CONTINUE,
	VERIFY_UNIT_SKIP,
} verify_unit_setup_t;

typedef verify_unit_setup_t (*V_test_setup_t) (V_runner_t*);

typedef struct _v_unit_test {
	const char* name;
	const char* description;
	V_test_t const *tests;
	V_test_setup_t setup;
	V_test_t teardown;
	struct _v_unit_test* next_unit;
} V_unit_test_t;

typedef void (*unit_test_t) (V_unit_test_t*);

struct test_runner {
	uint32_t test_result;
	uint32_t total_tests;
	uint32_t total_failed;
	uint32_t unit_count;
	uint32_t skipped_unit_count;
	const char *current_test_name;
	io_t *io;
	memory_info_t minfo[2];
	vref_t user_value;
};

#define TEST_BEGIN(test_name) void test_name (V_runner_t *vrunner) {\
	vrunner->current_test_name = #test_name;

#define TEST_END 	\
}

#define TEST_IO				(vrunner->io)
#define TEST_MEMORY_INFO	(vrunner->minfo)
#define TEST_USER_VALUE		(vrunner->user_value)

#define VERIFY(pred,desc) V_record_test_result(vrunner,pred,""#pred"",desc,__FILE__, __LINE__)
#define UNIT_SETUP(name) static verify_unit_setup_t name (V_runner_t *vrunner)
#define UNIT_TEARDOWN(name) static void name (V_runner_t *vrunner)

unsigned int	V_record_test_result(V_runner_t*,uint32_t,const char*,const char*,const char* file,unsigned int line);
void			V_run_unit_tests(V_runner_t*,unit_test_t const*);
uint32_t		print_unit_test_report (V_runner_t*);


#ifdef IMPLEMENT_VERIFY_IO_CORE
//-----------------------------------------------------------------------------
//
// implementtaion of verify
//
//-----------------------------------------------------------------------------
#include <io_device.h>

#define vprintf(r,fmt,...) io_printf(r->io,fmt,##__VA_ARGS__)

unsigned int
V_record_test_result (
	V_runner_t *runner,
	uint32_t result,
	const char* predicate,
	const char* description,
	const char* file,
	unsigned int line
) {
	runner->test_result = (runner->test_result && !!(result));
	runner->total_tests++;

	if(!(result)) {
		runner->total_failed++;	// NB: This line used as a breakpoint, do not move
	}

	return result;
}

void
V_init_unit(V_unit_test_t *new_unit) {
	new_unit->name = "";
	new_unit->description = "";
	new_unit->tests = NULL;
	new_unit->setup = NULL;
	new_unit->teardown = NULL;
	new_unit->next_unit = NULL;
}

void
V_run_rest(V_runner_t *runner,V_unit_test_t *unit) {
	V_test_t const *test;

	test = unit->tests;
	if (unit->setup != NULL) {
		runner->current_test_name = unit->name;
		switch (unit->setup(runner)) {
			case VERIFY_UNIT_SKIP:
				runner->skipped_unit_count++;
				return;

			case VERIFY_UNIT_CONTINUE:
			break;
		}
	}

	while((*test) != NULL) {
		runner->unit_count ++;
		(*test)(runner);
		test++;
	}

	if (unit->teardown != NULL) {
		runner->current_test_name = unit->name;
		unit->teardown(runner);
	}
}

void
V_start_tests(V_runner_t *runner) {
	runner->test_result = 1;
	runner->total_tests = 0;
	runner->total_failed = 0;
	runner->unit_count = 0;
	runner->skipped_unit_count = 0;
	runner->current_test_name = NULL;
}

void
V_run_unit_tests(V_runner_t *runner,unit_test_t const * unit_tests) {
	static V_unit_test_t unit;
	uint32_t i = 0;

	while(unit_tests[i] != 0) {
		V_init_unit(&unit);
		unit_tests[i++](&unit);
		V_run_rest(runner,&unit);
	}
}

uint32_t
print_unit_test_report (V_runner_t *runner) {
	vprintf (runner,"\n");
	if (runner->total_failed == 0) {
		vprintf (
			runner,"%-*s%-*scompleted with %d test%s",
			DBP_FIELD1,DEVICE_NAME,
			DBP_FIELD2,"self test",
			runner->total_tests,
			(runner->total_tests == 1) ? "":"s"
		);
		if (runner->skipped_unit_count > 0) {
			vprintf (
				runner,"%-*s(%u unit%s skipped)",
				DBP_FIELD1+DBP_FIELD2,"",
				runner->skipped_unit_count,plural(runner->skipped_unit_count)
			);
		}
		vprintf (runner," OK\n");
	} else {
		vprintf (
			runner,"%-*s%-*sfailed, %d tests of %d\n",
			DBP_FIELD1,"self test",
			DBP_FIELD2,DEVICE_NAME,
			runner->total_failed,
			runner->total_tests
		);
	}
	return runner->total_failed == 0;
}
#ifdef IMPLEMENT_VERIFY_IO_MATH

TEST_BEGIN(test_io_is_prime_1) {
	VERIFY (!is_u32_integer_prime (0),NULL);
	VERIFY (!is_u32_integer_prime (1),NULL);
	VERIFY (is_u32_integer_prime (2),NULL);

	uint32_t p[] = {163,277,409,547};
	bool ok = true;
	for (int i = 0; i < SIZEOF(p) && ok; i++) {
		ok &= is_u32_integer_prime (p[i]);
	}
	VERIFY (ok,"all prime");
}
TEST_END

TEST_BEGIN(test_io_next_prime_1) {
	uint32_t p[][2] = {{0,2},{1,2},{7,11},{163,167},{20,23},{50,53}};
	bool ok = true;
	for (int i = 0; i < SIZEOF(p); i++) {
		ok &= (next_prime_u32_integer (p[i][0]) == p[i][1]);
	}
	VERIFY (ok,"all next primes ok");
}
TEST_END

TEST_BEGIN(test_read_le_uint32_1) {
	uint8_t p[8] = {0x42,0,0,0};
	int64_t v = read_le_uint32 (p);
	VERIFY (v = 42,NULL);
}
TEST_END

TEST_BEGIN(test_read_le_int64_1) {
	uint8_t p[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,};
	int64_t v = read_le_int64 (p);
	VERIFY (v = -1,NULL);
}
TEST_END

TEST_BEGIN(test_read_float64_1) {
	uint8_t p[8] = {205,204,204,204,204,12,69,64};
	float64_t v = read_le_float64 (p);	
	VERIFY (io_math_compare_float64_eq (v,42.1),NULL);
}
TEST_END

TEST_BEGIN(test_number_gcd) {
	uint32_t numbers[4];
	
	VERIFY(gcd_uint32(48,18) == 6,NULL);
	VERIFY(gcd_uint32(10000000,1000000) == 1000000,NULL);
	VERIFY(gcd_uint32(17,10) == 1,NULL);
	VERIFY(gcd_uint32(1000,1000) == 1000,NULL);
	
	numbers[0] = 48;
	numbers[1] = 18;
	VERIFY(gcd_uint32_reduce (numbers,2) == 6,NULL);

	numbers[0] = 48;
	numbers[1] = 18;
	numbers[2] = 96;
	VERIFY(gcd_uint32_reduce (numbers,3) == 6,NULL);

}
TEST_END

UNIT_SETUP(setup_io_math_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_byte_memory_get_info (bm,TEST_MEMORY_INFO);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_math_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end;
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO->used_bytes,NULL);
}

void
io_math_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_is_prime_1,
		test_io_next_prime_1,
		test_read_le_uint32_1,
		test_read_le_int64_1,
		test_read_float64_1,
		test_number_gcd,
		0
	};
	unit->name = "io math";
	unit->description = "io math unit test";
	unit->tests = tests;
	unit->setup = setup_io_math_unit_test;
	unit->teardown = teardown_io_math_unit_test;
}
# define IO_MATH_UNIT_TESTS \
	io_math_unit_test,\
	/**/
#else
# define IO_MATH_UNIT_TESTS
#endif /* IMPLEMENT_VERIFY_IO_MATH */

# ifdef IMPLEMENT_VERIFY_IO_CORE_VALUES

TEST_BEGIN(test_io_text_encoding_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);

	io_encoding_t *encoding = mk_io_text_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
		uint8_t byte;
		
		reference_io_encoding (encoding);
		VERIFY (is_io_text_encoding (encoding),NULL);
		VERIFY (is_io_binary_encoding (encoding),NULL);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		VERIFY (io_encoding_printf (encoding,"%s","abc") == 3,NULL);
		VERIFY (io_encoding_length (encoding) == 3,NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 3,NULL);
		VERIFY (memcmp ("abc",b,3) == 0,NULL);
		byte = 0;
		VERIFY (io_encoding_pop_last_byte (encoding,&byte) && byte == 'c',NULL);
		io_encoding_reset (encoding);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		io_encoding_append_byte (encoding,0);

		unreference_io_encoding(encoding);
	}

	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_x70_encoding_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);

	io_encoding_t *encoding = mk_io_x70_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
	
		reference_io_encoding (encoding);

		VERIFY (is_io_x70_encoding (encoding),NULL);
		VERIFY (is_io_binary_encoding (encoding),NULL);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		VERIFY (io_encoding_printf (encoding,"%s","abc") == 3,NULL);
		VERIFY (io_encoding_length (encoding) == 3,NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 3,NULL);
		VERIFY (memcmp ("abc",b,3) == 0,NULL);

		io_encoding_reset (encoding);
		VERIFY (io_encoding_length (encoding) == 0,NULL);
		
		unreference_io_encoding(encoding);
	}

	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_packet_encoding_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	io_encoding_t *encoding = mk_io_twi_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
		io_twi_transfer_t *h;

		reference_io_encoding (encoding);

		VERIFY (is_io_twi_encoding (encoding),NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 0,NULL);

		h = io_encoding_get_get_rw_header (encoding);
		if (VERIFY (h != NULL,NULL)) {
			h->cmd.bus_address = 0x42;
			h->cmd.tx_length = 1;
			h->cmd.rx_length = 0;
		}

		unreference_io_encoding(encoding);
	}

	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_constant_values_1) {

	VERIFY(vref_get_containing_memory (cr_NIL) == NULL,NULL);

	VERIFY(vref_is_valid(cr_UNIV),NULL);
	VERIFY (io_value_is_equal (cr_UNIV,cr_UNIV),NULL);

	VERIFY(vref_is_valid(cr_NIL),NULL);
	VERIFY (io_typesafe_ro_cast(cr_NIL,cr_NIL) != NULL,NULL);
	VERIFY (io_value_is_equal (cr_NIL,cr_NIL),NULL);

	VERIFY (io_value_is_more (cr_UNIV,cr_NIL),NULL);
	VERIFY (io_value_is_less (cr_NIL,cr_UNIV),NULL);

	VERIFY(vref_is_valid(cr_BINARY),NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_BINARY) != NULL,NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_UNIV) != NULL,NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_NIL) == NULL,NULL);

	VERIFY(vref_is_valid(cr_CONSTANT_BINARY),NULL);
	VERIFY (io_typesafe_ro_cast(cr_CONSTANT_BINARY,cr_BINARY) != NULL,NULL);

	VERIFY(vref_is_valid(cr_SYMBOL),NULL);
	VERIFY (io_typesafe_ro_cast(cr_SYMBOL,cr_CONSTANT_BINARY) != NULL,NULL);
	VERIFY (arg_match(cr_SYMBOL)(cr_SYMBOL),NULL);

	VERIFY(vref_is_valid(cr_IO_EXCEPTION),NULL);

	VERIFY(vref_is_valid(cr_RESULT),NULL);
	VERIFY(vref_is_valid(cr_RESULT_CONTINUE),NULL);

	extern EVENT_DATA io_value_implementation_t nil_value_implementation;
	extern EVENT_DATA io_value_implementation_t i64_number_value_implementation;
	extern EVENT_DATA io_value_implementation_t f64_number_value_implementation;
	extern EVENT_DATA io_value_implementation_t binary_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_text_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_cons_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_list_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_map_slot_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_map_value_implementation;
	io_value_implementation_t const* expect[] = {
		&univ_value_implementation,
		&nil_value_implementation,
		&io_vector_value_implementation,
		&i64_number_value_implementation,
		&f64_number_value_implementation,
		&binary_value_implementation,
		&io_text_value_implementation,
		&io_cons_value_implementation,
		&io_list_value_implementation,
		&io_map_slot_value_implementation,
		&io_map_value_implementation,
	};
	bool ok = true;
	for (int i = 0; i < SIZEOF(expect) && ok; i++) {
		ok = expect[i] == io_get_value_implementation (TEST_IO,expect[i]->name,strlen(expect[i]->name));
	}
	VERIFY(ok,"value implementations match");

}
TEST_END

TEST_BEGIN(test_io_constant_values_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_encoding_t *encoding;
	const uint8_t *b,*e;

	encoding = mk_io_text_encoding (bm);

	VERIFY (io_encoding_printf (encoding,"%v",cr_NIL) == 2,NULL);
	io_encoding_get_content (encoding,&b,&e);
	VERIFY ((e - b) == 2,NULL);
	VERIFY (memcmp ("()",b,2) == 0,NULL);

	io_encoding_reset (encoding);
	VERIFY (io_encoding_printf (encoding,"%v",cr_UNIV) == 1,NULL);
	io_encoding_get_content (encoding,&b,&e);
	VERIFY ((e - b) == 1,NULL);
	VERIFY (memcmp (".",b,1) == 0,NULL);
	
	io_encoding_free (encoding);
}
TEST_END

TEST_BEGIN(test_io_int64_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_int64_value (vm,42));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_I64_NUMBER);
		io_value_int64_encoding_t enc = def_int64_encoding(24);
		vref_t r_decoded;

		VERIFY (v != NULL,NULL);
		VERIFY (io_typesafe_ro_cast (r_value,cr_NUMBER) != NULL,NULL);
		VERIFY(vref_get_containing_memory (r_value) == vm,NULL);

		VERIFY (
				io_value_encode (r_value,(io_encoding_t*) &enc)
			&&	io_value_int64_encoding_encoded_value(&enc) == 42,
			NULL
		);

		r_decoded = io_encoding_decode_to_io_value (
			(io_encoding_t*) &enc,io_integer_to_integer_value_decoder,vm
		);
		if (VERIFY (vref_is_valid (r_decoded),NULL)) {
			vref_t r_small = mk_io_int64_value (vm,-2);
			
			int64_t i64_value;

			reference_value (r_decoded);

			v = io_typesafe_ro_cast (r_decoded,cr_I64_NUMBER);
			if (VERIFY (v != NULL,NULL)) {
				io_value_int64_encoding_t dc = def_int64_encoding(24);
				VERIFY (
						io_value_encode (r_value,(io_encoding_t*) &dc)
					&&	io_value_int64_encoding_encoded_value(&enc) == 42,
					NULL
				);
			}

			VERIFY (io_value_get_as_int64(r_decoded,&i64_value) && i64_value == 42,NULL);

			VERIFY (io_value_is_equal (r_decoded,r_value),NULL);
			VERIFY (io_value_is_more (r_decoded,r_small),NULL);
			VERIFY (io_value_is_less (r_small,r_value),NULL);
			VERIFY (io_value_not_equal (cr_NIL,r_value),NULL);
			
			unreference_value (r_decoded);
		}
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_int64_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end,vm_begin,vm_end;
	io_encoding_t *encoding;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);
	io_byte_memory_get_info (bm,&bm_begin);

	r_value = reference_value (mk_io_int64_value (vm,42));

	encoding = mk_io_text_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;

		VERIFY (io_encoding_printf (encoding,"%v",r_value) == 2,NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 2,NULL);
		VERIFY (memcmp ("42",b,2) == 0,NULL);

		io_encoding_free(encoding);
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_int64_value_3) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_encoding_t *encoding;
	vref_t r_value;

	io_byte_memory_get_info (bm,&bm_begin);

	r_value = reference_value (mk_io_int64_value (vm,42));

	encoding = mk_io_x70_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
	
		uint8_t expect[] = {
			X70_UINT_VALUE_BYTE,3,
			'i','6','4',
			42,0,0,0,0,0,0,0
		};
		
		VERIFY (io_value_encode (r_value,encoding),NULL);

		io_encoding_get_content (encoding,&b,&e);
		if (
			VERIFY (
				(
						(e - b) == sizeof(expect) 
					&& memcmp (expect,b,sizeof(expect)) == 0
				),
				NULL
			)
		) {
			vref_t r_decoded = io_encoding_decode_to_io_value (
				encoding,io_x70_decoder,vm
			);
		
			if (
				VERIFY (
						vref_is_valid(r_decoded) 
					&&	io_typesafe_ro_cast (r_decoded,cr_I64_NUMBER),
					NULL
				)
			) {
				int64_t i64_value;
				VERIFY (
						io_value_get_as_int64 (r_decoded,&i64_value)
					&&	i64_value == 42,
					NULL
				);
			}
		}
		
		io_encoding_free(encoding);
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_float64_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_float64_value (vm,42.1));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_float64_encoding_t enc = def_float64_encoding(0.0);
		io_value_t const *v;
		float64_t f64;

		v = io_typesafe_ro_cast (r_value,cr_F64_NUMBER);
		VERIFY (v != NULL,NULL);
		VERIFY (io_typesafe_ro_cast (r_value,cr_NUMBER) != NULL,NULL);

		VERIFY (
				io_value_encode (r_value,(io_encoding_t*) &enc)
			&&	io_math_compare_float64_eq (io_value_float64_encoding_encoded_value(&enc),42.1),
			NULL
		);

		VERIFY (
				io_value_get_as_float64 (r_value,&f64)
			&&	io_math_compare_float64_eq (f64,42.1),
			NULL
		);

		unreference_value (r_value);
	}

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_float64_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_t r_value;

	r_value = reference_value (mk_io_float64_value (vm,42.1));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_encoding_t *encoding = mk_io_text_encoding (io_get_byte_memory (TEST_IO));
		const uint8_t *b,*e;

		VERIFY (io_encoding_printf (encoding,"%v",r_value) == 9,NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 9,NULL);
		VERIFY (memcmp ("42.100000",b,2) == 0,NULL);

		unreference_value (r_value);
		io_encoding_free(encoding);
	}

	io_do_gc (TEST_IO,-1);
}
TEST_END

TEST_BEGIN(test_io_float64_value_3) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_encoding_t *encoding;
	vref_t r_value;

	io_byte_memory_get_info (bm,&bm_begin);

	r_value = reference_value (mk_io_float64_value (vm,42.1));

	encoding = mk_io_x70_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
	
		uint8_t expect[] = {
			X70_UINT_VALUE_BYTE,3,
			'f','6','4',
			205,204,204,204,204,12,69,64
		};
		
		VERIFY (io_value_encode (r_value,encoding),NULL);

		io_encoding_get_content (encoding,&b,&e);
		if (
			VERIFY (
				(
						(e - b) == sizeof(expect) 
					&& memcmp (expect,b,sizeof(expect)) == 0
				),
				NULL
			)
		) {
			vref_t r_decoded = io_encoding_decode_to_io_value (
				encoding,io_x70_decoder,vm
			);
		
			if (
				VERIFY (
						vref_is_valid(r_decoded) 
					&&	io_typesafe_ro_cast (r_decoded,cr_F64_NUMBER),
					NULL
				)
			) {
				vref_t r_small = mk_io_float64_value (vm,12.1);
				float64_t f64_value;
				VERIFY (
						io_value_get_as_float64 (r_decoded,&f64_value)
					&&	io_math_compare_float64_eq (f64_value,42.1),
					NULL
				);
				VERIFY (io_value_is_equal (r_decoded,r_value),NULL);
				VERIFY (io_value_is_less (r_small,r_value),NULL);
				VERIFY (io_value_is_more (r_value,r_small),NULL);
			}
		}

		io_encoding_free(encoding);
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_binary_value_with_const_bytes_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_binary_value_with_const_bytes (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_CONSTANT_BINARY);

		if (VERIFY (v != NULL,NULL)) {
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding = mk_io_text_encoding (bm);

			if (VERIFY (io_value_encode (r_value,encoding),NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_content (encoding,&b,&e);
				VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
			}

			io_encoding_free(encoding);
		}
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_binary_value_dynamic_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_binary_value (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_BINARY);

		if (VERIFY (v != NULL,NULL)) {
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding = mk_io_text_encoding (bm);

			if (VERIFY (io_value_encode (r_value,encoding),NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_content (encoding,&b,&e);
				VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
			}

			io_encoding_free(encoding);
		}
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_text_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_text_value (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_TEXT);

		if (VERIFY (v != NULL,NULL)) {
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding = mk_io_text_encoding (bm);

			if (VERIFY (io_value_encode (r_value,encoding),NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_content (encoding,&b,&e);
				VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
			}

			io_encoding_free(encoding);
		}
		unreference_value (r_value);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_text_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_text_value (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
		io_encoding_t *encoding = mk_io_x70_encoding (bm);

		if (VERIFY (io_value_encode (r_value,encoding),NULL)) {

			const uint8_t *b,*e;
		
			uint8_t expect[] = {
				X70_UINT_VALUE_BYTE,4,
				't','e','x','t',
				X70_UINT_VALUE_BYTE,3,
				'a','b','c'
			};

			io_encoding_get_content (encoding,&b,&e);

			if (
				VERIFY (
					(
							(e - b) == sizeof(expect)
						&& memcmp (expect,b,sizeof(expect)) == 0
					),
					NULL
				)
			) {
				vref_t r_decoded = io_encoding_decode_to_io_value (
					encoding,io_x70_decoder,vm
				);
			
				if (
					VERIFY (
							vref_is_valid(r_decoded) 
						&&	io_typesafe_ro_cast (r_decoded,cr_TEXT),
						NULL
					)
				) {
					io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
					io_encoding_t *e1 = mk_io_text_encoding (bm);
					vref_t r_long = mk_io_text_value (vm,(uint8_t const*)"abcd",4);
					
					if (VERIFY (io_value_encode (r_decoded,e1),NULL)) {
						const uint8_t *b,*e;
						io_encoding_get_content (e1,&b,&e);
						VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
					}

					VERIFY (io_value_is_equal (r_decoded,r_value),NULL);
					VERIFY (io_value_is_less (r_decoded,r_long),NULL);
					VERIFY (io_value_is_more (r_long,r_value),NULL);
					
					io_encoding_free(e1);
				}
			}
		
			io_encoding_free(encoding);
		}
		unreference_value (r_value);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_stack_vector_value_1) {
	declare_stack_vector (r_vect,cr_NIL);
	uint32_t arity;
	vref_t const *args;

	VERIFY (io_vector_value_get_arity (r_vect,&arity) && arity == 1,NULL);
	VERIFY (io_vector_value_get_values (r_vect,&arity,&args) && vref_is_nil(args[0]),NULL);
}
TEST_END

TEST_BEGIN(test_vector_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_vector_value (vm,0,NULL));
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		uint32_t arity = 42;
		VERIFY (io_vector_value_get_arity (r_value,&arity) && arity == 0,NULL);
		unreference_value (r_value);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_vector_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;
	
	io_value_memory_get_info (vm,&vm_begin);
	
	{
		vref_t r_int = mk_io_int64_value (vm,42);
		vref_t args[] = {cr_NIL,r_int};
		vref_t const *values;
		
		r_value = reference_value (mk_io_vector_value (vm,SIZEOF(args),args));
		if (VERIFY(vref_is_valid(r_value),NULL)) {
			uint32_t arity = 42;
			VERIFY (io_vector_value_get_arity (r_value,&arity) && arity == 2,NULL);

			if (VERIFY (io_vector_value_get_values (r_value,&arity,&values),NULL)) {
				VERIFY (vref_is_nil (values[0]),NULL);
				VERIFY (vref_is_equal_to (values[1],r_int),NULL);
			}
			
			unreference_value (r_value);
		}
	}
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_cons_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value,r_car;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_value = mk_io_cons_value (vm,cr_NIL,cr_NIL,cr_NIL);
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		vref_t r_part;
		
		VERIFY (io_typesafe_ro_cast (r_value,cr_CONS) != NULL,NULL);

		r_part = cr_UNIV;
		VERIFY (io_cons_value_get_car (r_value,&r_part) && vref_is_nil(r_part),NULL);
		r_part = cr_UNIV;
		VERIFY (io_cons_value_get_cdr (r_value,&r_part) && vref_is_nil(r_part),NULL);

	}

	r_car =  mk_io_int64_value (vm,-2);
	r_value = mk_io_cons_value (vm,r_car,cr_NIL,cr_NIL);
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		vref_t r_part;
		VERIFY (io_typesafe_ro_cast (r_value,cr_CONS) != NULL,NULL);
		
		VERIFY (io_value_is_equal (r_value,r_value),NULL);
		r_part = cr_UNIV;
		VERIFY (io_cons_value_get_car (r_value,&r_part) && vref_is_equal_to(r_part,r_car),NULL);
		r_part = cr_UNIV;
		VERIFY (io_cons_value_get_cdr (r_value,&r_part) && vref_is_nil(r_part),NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_list_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_value = mk_io_list_value (vm);
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		vref_t r_pop;
		
		VERIFY (io_typesafe_ro_cast (r_value,cr_LIST) != NULL,NULL);
		VERIFY (vref_get_containing_memory (r_value) == vm,NULL);
		
		VERIFY (io_list_value_count (r_value) == 0,NULL);
		
		{
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding;
			encoding = mk_io_text_encoding (bm);
			if (VERIFY (io_encoding_printf (encoding,"%v",r_value) == 2,NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_content (encoding,&b,&e);
				VERIFY (memcmp(b,"()",2) == 0,NULL);
			}
			
			io_encoding_free(encoding);
		}
		
		io_list_value_append_value (r_value,cr_NIL);
		VERIFY (io_list_value_count (r_value) == 1,NULL);
		r_pop = cr_LIST;
		VERIFY (io_list_pop_last (r_value,&r_pop) && vref_is_nil (r_pop),NULL);
		VERIFY (io_list_value_count (r_value) == 0,NULL);

		io_list_value_append_value (r_value,cr_NIL);
		VERIFY (io_list_value_count (r_value) == 1,NULL);
		r_pop = cr_LIST;
		VERIFY (io_list_pop_first (r_value,&r_pop) && vref_is_nil (r_pop),NULL);
		VERIFY (io_list_value_count (r_value) == 0,NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_list_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_list;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_list = mk_io_list_value (vm);

	io_list_value_append_value (r_list,mk_io_int64_value (vm,42));

	{
		io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
		io_encoding_t *encoding;
		encoding = mk_io_text_encoding (bm);
		if (VERIFY (io_encoding_printf (encoding,"%v",r_list) == 4,NULL)) {
			const uint8_t *b,*e;
			io_encoding_get_content (encoding,&b,&e);
			VERIFY (memcmp(b,"(42)",4) == 0,NULL);
		}
		
		io_encoding_free(encoding);
	}
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

static bool
test_io_map_value_count_cb (vref_t r_slot,void *result) {
	(*((int*) result))++;
	return true;
}

TEST_BEGIN(test_map_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_map;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_map = mk_io_map_value (vm,3);
	if (VERIFY(vref_is_valid(r_map),NULL)) {
		vref_t r_mapped;
		int result;
		
		VERIFY (io_typesafe_ro_cast (r_map,cr_MAP) != NULL,NULL);
		VERIFY (vref_get_containing_memory (r_map) == vm,NULL);

		result = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&result);
		VERIFY(result == 0,NULL);

		VERIFY (io_map_value_map (r_map,cr_NIL,cr_NIL),NULL);
		VERIFY (
			(
					io_map_value_get_mapping (r_map,cr_NIL,&r_mapped) 
				&& vref_is_nil(r_mapped)
			),
			NULL
		);
		result = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&result);
		VERIFY(result == 1,NULL);
		
		r_mapped = io_map_value_unmap (r_map,cr_NIL);
		VERIFY (vref_is_nil (r_mapped),NULL);

		result = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&result);
		VERIFY(result == 0,NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

/*
				int64_t i64_value;
				VERIFY (
						io_value_get_as_int64 (r_decoded,&i64_value)
					&&	i64_value == 42,
					NULL
				);

*/
static bool
test_io_map_value_3_cb (vref_t r_slot,void *result) {
	int64_t v;
	if (io_value_get_as_int64 (io_map_slot_value_get_key(r_slot),&v)) {
		int **a = result;
		*(*a) = v;
		(*a)++;
	}
	return true;
}

TEST_BEGIN(test_map_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_map;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_map = mk_io_map_value (vm,3);
	if (VERIFY(vref_is_valid(r_map),NULL)) {
		vref_t r_key[8];
		bool ok;
		int count;
		
		ok = true;
		for (int i = SIZEOF(r_key) - 1; i >= 0; i--) {
			r_key[i] = mk_io_int64_value (vm,i);
			ok &= io_map_value_map (r_map,r_key[i],cr_NIL);
		}
		VERIFY(ok,"all new slots");

		count = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&count);
		VERIFY(count == SIZEOF(r_key),NULL);

		int rr[SIZEOF(r_key)] = {0},*c = rr;
		io_map_value_iterate (r_map,test_io_map_value_3_cb,&c);

		ok = true;
		for (int i = 0; i < SIZEOF(r_key); i++) {
			ok &= (rr[i] == i);
		}
		VERIFY (ok,"increasing order");

		ok = true;
		for (int i = SIZEOF(r_key) - 1; i >= 0; i--) {
			vref_t r_removed = io_map_value_unmap (r_map,r_key[i]);
			ok &= vref_is_nil(r_removed);
		}
		VERIFY(ok,"all slots unmaped");

		count = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&count);
		VERIFY(count == 0,NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

UNIT_SETUP(setup_io_core_values_unit_test) {
	io_byte_memory_get_info (io_get_byte_memory (TEST_IO),TEST_MEMORY_INFO);
	io_value_memory_get_info (io_get_short_term_value_memory (TEST_IO),TEST_MEMORY_INFO + 1);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_core_values_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end,vm_end;

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (io_get_short_term_value_memory (TEST_IO),&vm_end);
	VERIFY (vm_end.used_bytes == TEST_MEMORY_INFO[1].used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO[0].used_bytes,NULL);
}

void
io_core_values_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_text_encoding_1,
		test_io_x70_encoding_1,
		test_io_packet_encoding_1,
		test_io_constant_values_1,
		test_io_constant_values_2,
		test_io_int64_value_1,
		test_io_int64_value_2,
		test_io_int64_value_3,
		test_io_float64_value_1,
		test_io_float64_value_2,
		test_io_float64_value_3,
		test_io_binary_value_with_const_bytes_1,
		test_io_binary_value_dynamic_1,
		test_io_text_value_1,
		test_io_text_value_2,
		test_stack_vector_value_1,
		test_vector_value_1,
		test_vector_value_2,
		test_cons_value_1,
		test_list_value_1,
		test_list_value_2,
		test_map_value_1,
		test_map_value_2,
		0
	};
	unit->name = "io_values";
	unit->description = "io core values unit test";
	unit->tests = tests;
	unit->setup = setup_io_core_values_unit_test;
	unit->teardown = teardown_io_core_values_unit_test;
}
#define IO_CORE_VALUES_UNIT_TEST	\
		io_core_values_unit_test,
		/**/
#else
#define IO_CORE_VALUES_UNIT_TEST
# endif /* IMPLEMENT_VERIFY_IO_CORE_VALUES */

TEST_BEGIN(test_io_memories_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *mem,*bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	VERIFY (bm != NULL,NULL);
	VERIFY (io_value_memory_get_io(vm) == TEST_IO,NULL);

	io_byte_memory_get_info (bm,&bm_begin);

	mem = mk_io_byte_memory (TEST_IO,32,UMM_BLOCK_SIZE_8);
	if (VERIFY (mem != NULL,NULL)) {
		memory_info_t begin,end;
		void *data;
		
		io_byte_memory_get_info (mem,&begin);
		VERIFY (begin.used_bytes == 0,NULL);
		
		data = io_byte_memory_allocate (mem,12);
		VERIFY (data != NULL,NULL);
	
		io_byte_memory_free (mem,data);
	
		io_byte_memory_get_info (mem,&end);
		VERIFY (end.used_bytes == begin.used_bytes,NULL);
		
		free_io_byte_memory (mem);
	}
	
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_memories_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	io_byte_memory_get_info (bm,&bm_begin);
	
	io_value_memory_t *vm = mk_umm_io_value_memory (
		TEST_IO,1024,INVALID_MEMORY_ID
	);
	
	if (VERIFY (vm != NULL,NULL)) {
		memory_info_t begin,end;
		vref_t r_value;
		bool ok;
		
		io_value_memory_get_info (vm,&begin);
		VERIFY (begin.used_bytes == 0,NULL);

		ok = true;
		for (int i = 0; i < 8; i++) {
			r_value = mk_io_int64_value (vm,42);
			ok &= vref_is_valid(r_value);
		}
		
		VERIFY (ok,"values created");
		io_value_memory_do_gc (vm,-1);
		io_value_memory_get_info (vm,&end);
		VERIFY (end.used_bytes == 0,NULL);

		ok = true;
		for (int i = 0; i < 16; i++) {
			r_value = mk_io_int64_value (vm,42);
			ok &= vref_is_valid(r_value);
		}
		
		VERIFY (ok,"values created");
		io_value_memory_do_gc (vm,-1);
		io_value_memory_get_info (vm,&end);
		VERIFY (end.used_bytes == 0,NULL);
		
		umm_value_memory_free_memory (vm);
	}
	
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_byte_pipe_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	io_byte_memory_get_info (bm,&bm_begin);

	io_byte_pipe_t *pipe = mk_io_byte_pipe (bm,8);
	if (VERIFY (pipe != NULL,NULL)) {
		uint8_t data = 0;
		VERIFY (!io_byte_pipe_is_readable (pipe),NULL);
		VERIFY (io_byte_pipe_put_byte (pipe,42),NULL);
		VERIFY (io_byte_pipe_is_readable (pipe),NULL);
		VERIFY (io_byte_pipe_is_writeable (pipe),NULL);
		VERIFY (io_byte_pipe_get_byte (pipe,&data) && data == 42,NULL);
		VERIFY (!io_byte_pipe_is_readable (pipe),NULL);
		VERIFY (io_byte_pipe_is_writeable (pipe),NULL);
		
		VERIFY (is_io_byte_pipe ((io_pipe_t*) (pipe)),NULL);
		
		free_io_byte_pipe (pipe,bm);
	}
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_encoding_pipe_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	io_byte_memory_get_info (bm,&bm_begin);

	io_encoding_pipe_t *pipe = mk_io_encoding_pipe (bm,4);
	if (VERIFY (pipe != NULL,NULL)) {
		io_encoding_t *data = NULL, *encoding;
		VERIFY (!io_encoding_pipe_is_readable (pipe),NULL);
		encoding = mk_io_text_encoding (bm);
		VERIFY (io_encoding_pipe_put_encoding (pipe,encoding),NULL);
		VERIFY (io_encoding_pipe_is_readable (pipe),NULL);
		VERIFY (io_encoding_pipe_is_writeable (pipe),NULL);
		VERIFY (io_encoding_pipe_peek (pipe,&data) && data == encoding,NULL);
		io_encoding_pipe_pop_encoding (pipe);
		
		VERIFY (!io_encoding_pipe_is_readable (pipe),NULL);
		VERIFY (io_encoding_pipe_is_writeable (pipe),NULL);

		VERIFY (is_io_encoding_pipe ((io_pipe_t*) (pipe)),NULL);

		free_io_encoding_pipe (pipe,bm);
	}
	
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_value_pipe_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	memory_info_t vm_begin,vm_end;
	
	io_value_memory_get_info (vm,&vm_begin);
	io_byte_memory_get_info (bm,&bm_begin);

	io_value_pipe_t *pipe = mk_io_value_pipe (bm,3);
	if (VERIFY (pipe != NULL,NULL)) {
		vref_t values[] = {
			mk_io_int64_value (vm,42),
			mk_io_int64_value (vm,43),
			mk_io_int64_value (vm,44),
		};
		vref_t r_value;
		
		VERIFY (!io_value_pipe_is_readable (pipe),NULL);
		VERIFY (io_value_pipe_put_value (pipe,values[0]),NULL);
		VERIFY (io_value_pipe_is_readable (pipe),NULL);

		VERIFY (io_value_pipe_put_value (pipe,values[1]),NULL);

		VERIFY (!io_value_pipe_put_value (pipe,values[2]),NULL);
		
		VERIFY (io_value_pipe_get_value (pipe,&r_value),NULL);
		VERIFY (vref_is_equal_to (r_value,values[0]),NULL);
		VERIFY (io_value_pipe_is_readable (pipe),NULL);
		VERIFY (io_value_pipe_get_value (pipe,&r_value),NULL);
		VERIFY (vref_is_equal_to (r_value,values[1]),NULL);
		VERIFY (!io_value_pipe_is_readable (pipe),NULL);
		VERIFY (!io_value_pipe_get_value (pipe,&r_value),NULL);
		
		free_io_value_pipe (pipe,bm);
	}

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_tls_sha256_1) {
	io_sha256_context_t ctx;
	uint8_t output[32];
	uint8_t expect[32] = {
		0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,
		0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
		0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,
		0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55
	};

	io_sha256_start (TEST_IO,&ctx);
	io_sha256_update (TEST_IO,&ctx,NULL,0);
	io_sha256_finish (TEST_IO,&ctx,output);
	VERIFY (memcmp (output,expect,32) == 0,NULL);
}
TEST_END

/*
 *-----------------------------------------------------------------------------
 *
 * test_io_tls_sha256_2 --
 *
 *-----------------------------------------------------------------------------
 */
TEST_BEGIN(test_io_tls_sha256_2) {
	io_sha256_context_t ctx;
	uint8_t data[] = {0x81,0xa7,0x23,0xd9,0x66};
	uint8_t expect[SHA256_SIZE] = {
		0x75,0x16,0xfb,0x8b,0xb1,0x13,0x50,0xdf,
		0x2b,0xf3,0x86,0xbc,0x3c,0x33,0xbd,0x0f,
		0x52,0xcb,0x4c,0x67,0xc6,0xe4,0x74,0x5e,
		0x04,0x88,0xe6,0x2c,0x2a,0xea,0x26,0x05
	};
	uint8_t output[32];

	io_sha256_start (TEST_IO,&ctx);
	io_sha256_update (TEST_IO,&ctx,data,sizeof(data));
	io_sha256_finish (TEST_IO,&ctx,output);
	
	VERIFY (memcmp (output,expect,SHA256_SIZE) == 0,NULL);
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_vref_bucket_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		VERIFY (!vref_hash_table_contains (hash,cr_NIL),NULL);
		free_vref_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_vref_bucket_hash_table (bm,7);
	VERIFY (vref_hash_table_insert (hash,cr_NIL),NULL);
	VERIFY (vref_hash_table_contains (hash,cr_NIL),NULL);
	VERIFY (vref_hash_table_remove (hash,cr_NIL),NULL);
	VERIFY (!vref_hash_table_contains (hash,cr_NIL),NULL);

	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END
			
TEST_BEGIN(test_vref_bucket_hash_table_3) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t bmbegin,bmend;
	memory_info_t vmbegin,vmend;
	vref_t r_a;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);
	
	hash = mk_vref_bucket_hash_table (bm,7);

	r_a = mk_io_int64_value (vm,42);
	VERIFY (vref_hash_table_insert (hash,r_a),NULL);
	VERIFY (vref_hash_table_contains (hash,r_a),NULL);

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes > vmbegin.used_bytes,NULL);	
	
	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_4) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t bmbegin,bmend;
	memory_info_t vmbegin,vmend;
	vref_t r_value[16];
	bool ok;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);
	
	hash = mk_vref_bucket_hash_table (bm,30);

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		r_value[i] = mk_io_int64_value (vm,i);
		ok &= vref_hash_table_insert (hash,r_value[i]);
	}
	VERIFY (ok,"values all added");
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes > vmbegin.used_bytes,NULL);	

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		ok &= vref_hash_table_contains (hash,r_value[i]);
	}
	VERIFY (ok,"values all present");
	
	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_5) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t bmbegin,bmend;
	memory_info_t vmbegin,vmend;
	vref_t r_value[16];
	bool ok;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);
	
	hash = mk_vref_bucket_hash_table (bm,30);

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		r_value[i] = mk_io_int64_value (vm,i);
		ok &= vref_hash_table_insert (hash,r_value[i]);
	}
	VERIFY (ok,"values all added");
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes > vmbegin.used_bytes,NULL);	

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		ok &= vref_hash_table_remove (hash,r_value[i]);
	}
	VERIFY (ok,"values all removed");

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		ok &= !vref_hash_table_contains (hash,r_value[i]);
	}
	VERIFY (ok,"values all gone");
	
	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_string_hash_table_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	string_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_string_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		const char* key = "abc";
		char data[] = {5,6,0,7};
		string_hash_table_mapping_t map;
		
		VERIFY (!string_hash_table_map (hash,key,strlen(key),&map),NULL);
		VERIFY (string_hash_table_insert (hash,key,strlen(key),def_hash_mapping_i32(42)),NULL);
		map.i32 = 0;
		VERIFY (string_hash_table_map (hash,key,strlen(key),&map) && map.i32 == 42,NULL);

		VERIFY (string_hash_table_insert (hash,data,sizeof(data),def_hash_mapping_i32(9)),NULL);
		map.i32 = 0;
		VERIFY (string_hash_table_map (hash,data,sizeof(data),&map) && map.i32 == 9,NULL);

		VERIFY (!string_hash_table_insert (hash,data,sizeof(data),def_hash_mapping_i32(22)),NULL);
		map.i32 = 0;
		VERIFY (string_hash_table_map (hash,data,sizeof(data),&map) && map.i32 == 22,NULL);

		free_string_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_string_hash_table_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	string_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_string_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		uint32_t len = hash->table_size;
		bool ok = true;
		
		for (int i = 0; i < 120 && ok; i++) {
			char c = 'a' + i;
			ok &= string_hash_table_insert (hash,&c,1,def_hash_mapping_i32(i));
		}
		VERIFY (ok,"all inserted");
		// table grew
		VERIFY (hash->table_size > len,NULL);

		ok = true;
		for (int i = 0; i < 48 && ok; i++) {
			string_hash_table_mapping_t v;
			char c = 'a' + i;
			ok &= string_hash_table_map (hash,&c,1,&v) && v.i32 == i;
		}
		VERIFY (ok,"all mapped");
		
		free_string_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_string_hash_table_3) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	string_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_string_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		string_hash_table_mapping_t map;
		
		VERIFY (string_hash_table_insert (hash,NULL,0,def_hash_mapping_i32(0)),NULL);

		map.i32 = -1;
		VERIFY (string_hash_table_map (hash,NULL,0,&map) && map.i32 == 0,NULL);

		free_string_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

int
test_io_pq_sort_1_compare (void const *a,void const *b) {
	int c = ((int) a) - ((int) b);
	return (c > 0) ? 1 : (c < 0) ? -1 : 0;
}

TEST_BEGIN(test_io_pq_sort_1) {
	int data[] = {5,1,4,2};
	int expect[] = {1,2,4,5};
	bool ok;
	
	pq_sort ((void**) data,SIZEOF(data),test_io_pq_sort_1_compare);
	
	ok = true;
	for (int i = 0; i < SIZEOF(data) && ok; i++) {
		ok &= expect[i] == data[i];
	}
	VERIFY(ok,"sorted");
}
TEST_END

TEST_BEGIN(test_io_pq_sort_2) {
	int a[] = {12,43,13,5,8,10,11,9,20,17};
	int r[] = {5,8,9,10,11,12,13,17,20,43};
	int i,ok = 1;
	
	pq_sort ((void**) a,SIZEOF(a),test_io_pq_sort_1_compare);
	for (i = 0; i < SIZEOF(a); i++){
		ok &= (a[i] == r[i]);
	}

	VERIFY(ok,"all sorted");	
}
TEST_END

static bool
test_constrained_hash_purge_entry_helper (
	vref_t a,vref_t b,void *u
) {
	uint32_t *prune_count = (uint32_t*) u;
	*prune_count += 1;	
	return true;
}

static void
test_constrained_hash_begin_purge_helper (void *u) {
}

TEST_BEGIN(test_io_constrained_hash_table_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_constrained_hash_t *hash;
	memory_info_t begin,end;
	uint32_t size = 17;
	
	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		NULL
	);
	
	if (VERIFY (hash != NULL,NULL)) {
		
		free_io_constrained_hash (bm,hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_constrained_hash_table_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend,vmbegin,vmend;
	io_constrained_hash_t *cht;
	uint32_t size = 17;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);

	cht = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		NULL
	);
	
	if (VERIFY (cht != NULL,NULL)) {
		vref_t r_last = cr_NIL;
		uint32_t i,c;
		
		VERIFY(cht_get_table_size(cht) >= size,"table size");
		size = cht_get_table_size(cht);
		
		for (i = 0, c = 0; i < size; i++) {
			io_constrained_hash_entry_t *e = &cht_entry_at_index(cht,i);
			e->value = mk_io_int64_value (vm,i);
			r_last = e->value;
			c += e->info.free;
		}
		VERIFY(c == size,"table initialised");
		VERIFY(cht_count(cht) == 0,"empty");

		cht_entry_at_index(cht,size - 1).info.free = 0;
		cht_sort(cht);
		VERIFY (
			vref_is_equal_to (
				cht_ordered_entry_at_index(cht,0)->value,r_last
			),
			"order by age"
		);

		VERIFY(!cht_has_key(cht,cr_NIL),NULL);
		
		free_io_constrained_hash (bm,cht);
	}
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_constrained_hash_table_3) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend,vmbegin,vmend;
	io_constrained_hash_t *cht;
	uint32_t size = 17;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);

	cht = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		NULL
	);
	
	if (VERIFY (cht != NULL,NULL)) {
		vref_t r_key,r_value;
		
		r_key = mk_io_int64_value (vm,0);
		r_value = mk_io_int64_value (vm,1);
		
		cht_set_value(cht,r_key,r_value);
		VERIFY (cht_count(cht) == 1,NULL);
		VERIFY (cht_has_key(cht,r_key),NULL);
		VERIFY (vref_is_equal_to (cht_get_value(cht,r_key),r_value),NULL);

		VERIFY (cht_unset (cht,r_key),NULL);
		VERIFY (!cht_has_key(cht,r_key),NULL);

		VERIFY (!cht_has_key(cht,cr_NIL),NULL);

		free_io_constrained_hash (bm,cht);
	}
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_constrained_hash_table_4) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend,vmbegin,vmend;
	io_constrained_hash_t *cht;
	uint32_t size = 17;
	uint32_t prune_count = 0;
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);

	cht = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		&prune_count
	);
	
	if (VERIFY (cht != NULL,NULL)) {
		int i;
		for(i = 0; i < cht_get_entry_limit (cht); i++) {
			cht_set_value (
				cht,mk_io_int64_value (vm,i),cr_NIL
			);
		}

		VERIFY(prune_count == 0,"no pruning yet");
		cht_set_value (
			cht,mk_io_int64_value (vm,i),cr_NIL
		);

		VERIFY(prune_count == 2,"entries pruned");

		free_io_constrained_hash (bm,cht);
	}
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

UNIT_SETUP(setup_io_containers_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_containers_unit_test) {
}

static void
io_containers_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_memories_1,
		test_io_memories_2,
		test_io_byte_pipe_1,
		test_io_encoding_pipe_1,
		test_io_value_pipe_1,
		test_io_tls_sha256_1,
		test_io_tls_sha256_2,
		test_vref_bucket_hash_table_1,
		test_vref_bucket_hash_table_2,
		test_vref_bucket_hash_table_3,
		test_vref_bucket_hash_table_4,
		test_vref_bucket_hash_table_5,
		test_string_hash_table_1,
		test_string_hash_table_2,
		test_string_hash_table_3,
		test_io_pq_sort_1,
		test_io_constrained_hash_table_1,
		test_io_constrained_hash_table_2,
		test_io_constrained_hash_table_3,
		test_io_constrained_hash_table_4,
		0
	};
	unit->name = "io containers";
	unit->description = "io containers unit test";
	unit->tests = tests;
	unit->setup = setup_io_containers_unit_test;
	unit->teardown = teardown_io_containers_unit_test;
}

#ifdef IMPLEMENT_VERIFY_IO_CORE_SOCKETS



TEST_BEGIN(test_io_address_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	io_address_t a;
	
	io_byte_memory_get_info (bm,&bmbegin);

	a = io_any_address();
	VERIFY (is_any_io_address (a),NULL);
	
	{
		uint8_t t = 42;
		a = mk_io_address(TEST_IO,1,&t);
		VERIFY (io_u8_address_value(a) == 42,NULL);
		VERIFY (compare_io_addresses (a,io_any_address()) == 1,NULL);
		VERIFY (compare_io_addresses (io_any_address(),a) == -1,NULL);
		free_io_address (TEST_IO,a);
	}
	
	{
		uint16_t t = 4296;
		a = mk_io_address(TEST_IO,2,(uint8_t const*) &t);
		VERIFY (io_u16_address_value(a) == t,NULL);
		VERIFY (compare_io_addresses (a,io_any_address()) == 1,NULL);
		VERIFY (compare_io_addresses (io_any_address(),a) == -1,NULL);
		free_io_address (TEST_IO,a);
	}

	{
		uint32_t t = 0x8000000;
		a = mk_io_address(TEST_IO,4,(uint8_t const*) &t);
		VERIFY (io_u32_address_value(a) == t,NULL);
		VERIFY (compare_io_addresses (a,io_any_address()) == 1,NULL);
		VERIFY (compare_io_addresses (io_any_address(),a) == -1,NULL);
		free_io_address (TEST_IO,a);
	}

	{
		uint8_t t[] = {1,0,0,0,0};
		a = mk_io_address (TEST_IO,sizeof(t),t);
		VERIFY (compare_io_addresses (a,def_io_u8_address(1)) == 0,NULL);
		VERIFY (compare_io_addresses (def_io_u8_address(1),a) == 0,NULL);
		VERIFY (compare_io_addresses (def_io_u8_address(2),a) == 1,NULL);
		VERIFY (compare_io_addresses (a,def_io_u8_address(2)) == -1,NULL);
		free_io_address (TEST_IO,a);
	}

	{
		uint8_t d1[] = {1,0,0,1,0,2};
		uint8_t d2[] = {1,0,0,1,0,2};
		uint8_t d3[] = {1,0,0,0,0,2};
		io_address_t a1 = mk_io_address (TEST_IO,sizeof(d1),d1);
		io_address_t a2 = mk_io_address (TEST_IO,sizeof(d2),d2);
		io_address_t a3 = mk_io_address (TEST_IO,sizeof(d3),d3);

		VERIFY (compare_io_addresses (a1,a2) == 0,NULL);
		VERIFY (compare_io_addresses (a1,a3) == 1,NULL);
		VERIFY (compare_io_addresses (a3,a2) == -1,NULL);

		VERIFY (compare_io_addresses (a3,io_any_address()) == 1,NULL);

		free_io_address (TEST_IO,a1);
		free_io_address (TEST_IO,a2);
		free_io_address (TEST_IO,a3);
	}

	VERIFY (compare_io_addresses (io_any_address(),io_any_address()) == 0,NULL);
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

UNIT_SETUP(setup_io_sockets_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_sockets_unit_test) {
}

static void
io_sockets_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_address_1,
		0
	};
	unit->name = "io sockets";
	unit->description = "io sockets unit test";
	unit->tests = tests;
	unit->setup = setup_io_sockets_unit_test;
	unit->teardown = teardown_io_sockets_unit_test;
}



#define IO_CORE_SOCKETS_UNIT_TEST io_sockets_unit_test,
#else
#define IO_CORE_SOCKETS_UNIT_TEST
#endif /* IMPLEMENT_VERIFY_IO_CORE_SOCKETS */

void
run_ut_io (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_containers_unit_test,
		IO_CORE_VALUES_UNIT_TEST
		IO_CORE_SOCKETS_UNIT_TEST
		IO_MATH_UNIT_TESTS
		IO_CPU_UNIT_TESTS
		IO_DEVICE_UNIT_TESTS
		0
	};
	V_run_unit_tests(runner,test_set);
}

void
verify_io (V_runner_t *runner) {
	run_ut_io (runner);
}

#endif /* IMPLEMENT_VERIFY_IO_CORE */
#endif /* verify_io_core_H_ */
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Gregor Bruce
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
