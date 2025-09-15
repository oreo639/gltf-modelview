#include <stdio.h>
#include <string.h>

#include <hash_table.h>
#include <tests.h>

static bool str_equals(const void *str1, const void *str2) {
	return strcmp(str1, str2) == 0;
}

static uint32_t bad_hash(const void *str1) {
	(void)str1;
	return 11;
}

//void hash_table_print(struct hash_table *ht);

static void test_insert_regular(void) {
	char *table[] = {
		"Dave",
		"Jose",
		"Joseph",
		"Jane",
		"Daniel",
		"Mona",
		"Ron",
		"Jessica",
		"Jaden",
		"Fred",
		"Ronald",
		"Donald",
		"hello",
	};
	struct hash_table *ht = hash_table_create(1, hash_fnv1a32_str, str_equals, NULL, NULL);
	utest_assert_true(ht);

	for (size_t i=0; i < sizeof(table)/sizeof(table[0]); i++) {
		void *ent = hash_table_insert(ht, table[i], table[i], true);
		utest_assert(ent == table[i], "Insertion for %s returned unexpected result (%p:%p)", table[i], (void*)table[i], ent);
	}

	//hash_table_print(ht);
	for (size_t i = 0; i < sizeof(table)/sizeof(table[0]); ++i) {
		void *ent = NULL;
		utest_assert(hash_table_lookup(ht, table[i], &ent), "Lookup failed for %s", table[i]);
		utest_assert(ent == table[i], "Lookup for %s returned unexpected result (%p:%p).", table[i], (void*)table[i], ent);
	}

	{
		void *ent = NULL;
		utest_assert(hash_table_lookup(ht, "Jane", &ent), "Failed to lookup user Jane (what is the user that corresponds with table entry 3?)");
		utest_assert(ent == table[3], "Jane does not correspond with table entry 3 (what is the user that corresponds with table entry 3?)");
	}

	{
		utest_assert(hash_table_remove(ht, "Jane"), "Failed to remove user Jane");
		void *ent = NULL;
		utest_assert(!hash_table_lookup(ht, "Jane", &ent), "User Jane still exists in hash table after removal");
		utest_assert(ent == NULL, "Post-removal lookup for user Jane returned unexpected result (%p)", ent);
	}

	hash_table_destroy(ht);
}

static void test_insert_duplicate(void) {
	// Make sure duplicate insertions return the same entry
	char value1[] = "Dave";
	char value2[] = "Dave";
	struct hash_table *ht = hash_table_create(1, hash_fnv1a32_str, str_equals, NULL, NULL);
	utest_assert_true(ht);
	utest_assert_true((void*)value1 != (void*)value2);

	void *ent0 = hash_table_insert(ht, value1, value1, true);
	utest_assert_true(ent0 == value1);
	void *ent1 = hash_table_insert(ht, value2, value2, false);
	utest_assert_true(ent1 == value1);
	hash_table_destroy(ht);
}

static void test_insert_delete_lookup(void) {
	// Make sure lockups continue past deleted entries.
	struct hash_table *ht = hash_table_create(1, bad_hash, str_equals, NULL, NULL);
	utest_assert_true(ht);

	void *ent0 = hash_table_insert(ht, "Jake1", "Jake1", false);
	utest_assert_true(ent0);
	void *ent1 = hash_table_insert(ht, "Jake2", "Jake2", false);
	utest_assert_true(ent1);
	utest_assert_true(hash_table_remove(ht, "Jake1"));
	utest_assert_true(ent1 == hash_table_insert(ht, "Jake2", NULL, false));
	hash_table_destroy(ht);
}

int main(void) {
	utest_run(test_insert_regular,);
	utest_run(test_insert_duplicate,);
	utest_run(test_insert_delete_lookup,);
	return utest_end();
}
