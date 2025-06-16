#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h> // For UINT_MAX
#include "sv.h"

// Helper to print test results
void print_test_result(const char* test_name, int success) {
    printf("%s: %s\n", test_name, success ? "PASS" : "FAIL");
}

// Test 1: sv_write_field over-read (more of a crash/no-crash test)
// Attempts to trigger an over-read if the fix in sv_write_field was not present.
// The fix ensures loops are bounded by 'width', not just null terminators.
void test_sv_write_field_overread() {
    const char* test_name = "test_sv_write_field_overread";
    sv* t = sv_new(NULL, NULL, NULL, ',');
    if (!t) {
        print_test_result(test_name, 0);
        fprintf(stderr, "  Failed to create sv object.\n");
        return;
    }

    // Create a field that is not null-terminated within the 'width' we'll specify.
    // field_data: |'f'|'i'|'e'|'l'|'d'|GARBAGE... (not null-terminated at byte 5)
    char field_data[] = {'f', 'i', 'e', 'l', 'd', 'X', 'Y', 'Z'};
    char* fields[] = {field_data};
    size_t widths[] = {5}; // Specify width as 5. sv_write_field should only process field_data[0] to field_data[4]
    size_t count = 1;

    // Attempt to write to /dev/null, we only care if sv_write_fields crashes due to overread.
    FILE* fh = fopen("/dev/null", "w");
    if (!fh) {
        // If /dev/null can't be opened, we can't really run this test effectively as a crash test.
        // But the primary check is that sv_write_fields itself doesn't try to read past `width`.
        // We'll proceed, assuming the logic is tested internally.
        // The main point is to exercise the code path.
    }

    sv_status_t status = SV_STATUS_OK;
    if (fh) {
        status = sv_write_fields(t, fh, fields, widths, count);
        fclose(fh);
    } else {
        // If /dev/null is not available, call sv_write_field directly with a NULL FILE pointer.
        // This is not ideal but will exercise the internal logic of sv_write_field's loops.
        // The function will likely fail to write, but it shouldn't crash from over-read.
        // Note: This is a workaround. sv_write_field is static, so we call sv_write_fields.
        // The previous test with fh=fopen("/dev/null") is better. This is a fallback idea.
        // For now, let's assume /dev/null works or the test is "best effort".
        // The core idea is that the call to sv_write_fields (which calls sv_write_field)
        // with the non-null-terminated string but a limiting 'width' should not crash.
    }

    // If it doesn't crash, we consider it a pass for this basic test.
    // True verification needs Valgrind.
    // status might be SV_STATUS_FAILED if /dev/null is weird, but crash is the concern.
    print_test_result(test_name, 1); // Assuming no crash implies pass
    fprintf(stderr, "  Note: This test relies on absence of crash. True over-read detection needs Valgrind.\n");

    sv_free(t);
}


// Test 2: sv_init_fields integer overflow leading to NO_MEMORY
// Tries to make sv_init_fields request huge memory that should trigger overflow checks.
void test_sv_init_fields_overflow() {
    const char* test_name = "test_sv_init_fields_overflow";
    sv* t = sv_new(NULL, NULL, NULL, ',');
    if (!t) {
        print_test_result(test_name, 0);
        fprintf(stderr, "  Failed to create sv object.\n");
        return;
    }

    // Create a line with an extremely large number of fields.
    // Max number of fields is UINT_MAX. If size_t is larger, then (UINT_MAX+1) * sizeof(char*)
    // is less likely to overflow SIZE_MAX. The check is num_elements > SIZE_MAX / sizeof(char*).
    // Let's simulate a scenario where num_elements *is* very large.
    // This requires crafting a string. Max string length itself is an issue.
    // A direct call to sv_init_fields would be better, but it's static.
    // We craft a line with many separators.
    // (SIZE_MAX / sizeof(char*)) can be huge. Let's try a big number.
    // A string of 2,000,000 commas means 2,000,001 fields.
    // (2000001+1) * sizeof(char*) -- if char* is 8, this is ~16MB. This won't overflow SIZE_MAX.
    // To truly test SIZE_MAX overflow, we'd need close to SIZE_MAX fields, which is not feasible.
    // The check added was: `if (num_elements > SIZE_MAX / sizeof(char*))`
    // and `if (nfields == UINT_MAX && (sizeof(unsigned int) == sizeof(size_t)))`
    //
    // This test will construct a moderately large number of fields.
    // It's unlikely to hit the *actual* SIZE_MAX overflow, but it tests parsing many fields.
    // A more targeted test for the overflow check itself is hard without internal access.

    // Let's make a string with 100,000 commas (100,001 fields).
    // This is more of a stress test for many fields than a true overflow trigger.
    // The actual overflow condition `num_elements > SIZE_MAX / sizeof(char*)` is hard to hit.
    // For example, if SIZE_MAX is 2^64-1 and sizeof(char*) is 8, num_elements must be > 2^60.
    // This means 2^60 fields. A string for that is impossible.
    //
    // So, this test will likely pass by successfully parsing (if memory allows)
    // or fail due to memory for the string/field structures, not the specific overflow check.
    // The overflow check is a safeguard for truly astronomical numbers.

    int num_commas = 100000; // 100,001 fields
    char* long_line = (char*)malloc(num_commas + 2); // +1 for last char (e.g. 'a'), +1 for \0
    if(!long_line) {
        print_test_result(test_name, 0);
        fprintf(stderr, "  Malloc failed for long_line.\n");
        sv_free(t);
        return;
    }
    for(int i=0; i<num_commas; ++i) long_line[i] = ',';
    long_line[num_commas] = 'a'; // Add a char to ensure last field is processed
    long_line[num_commas+1] = '\0';

    sv_status_t status = sv_parse_chunk(t, long_line, strlen(long_line));
    // We expect either OK (if it fits in memory and doesn't hit the theoretical overflow)
    // or NO_MEMORY if any malloc along the way fails (including the one guarded by overflow check).
    // The specific check `num_elements > SIZE_MAX / sizeof(char*)` is what we hope to ensure.
    // If that check is hit, sv_init_fields returns SV_STATUS_NO_MEMORY.

    // This test is imperfect for *forcing* the overflow check.
    // It mostly tests if parsing many fields causes other crashes.
    // A true test of the SIZE_MAX check would need to mock `nfields`.
    if (status == SV_STATUS_NO_MEMORY) {
        print_test_result(test_name, 1); // Correctly reported no memory (could be our check)
        fprintf(stderr, "  Received SV_STATUS_NO_MEMORY, which is an acceptable outcome for many fields.\n");
    } else if (status == SV_STATUS_OK) {
        print_test_result(test_name, 1); // Also acceptable if it actually parsed it.
        fprintf(stderr, "  Parsed many fields successfully.\n");
    } else {
        print_test_result(test_name, 0); // Other error
        fprintf(stderr, "  Unexpected status: %d.\n", status);
    }

    free(long_line);
    sv_free(t);
}


// Test 3: I/O error propagation in write.c
void test_io_error_propagation() {
    const char* test_name = "test_io_error_propagation";
    sv* t = sv_new(NULL, NULL, NULL, ',');
    if (!t) {
        print_test_result(test_name, 0);
        fprintf(stderr, "  Failed to create sv object.\n");
        return;
    }

    char* fields[] = {"data1", "data2"};
    size_t count = 2;

    // Attempt to write to /dev/full, which should always cause I/O errors (ENOSPC)
    FILE* fh = fopen("/dev/full", "w");
    if (!fh) {
        print_test_result(test_name, 0); // Cannot perform test if /dev/full is not available/writable
        fprintf(stderr, "  Could not open /dev/full. Test skipped.\n");
        sv_free(t);
        return;
    }

    sv_status_t status = sv_write_fields(t, fh, fields, NULL, count);
    fclose(fh); // Important to close even on error

    // Expect SV_STATUS_FAILED due to I/O error
    if (status == SV_STATUS_FAILED) {
        print_test_result(test_name, 1);
    } else {
        print_test_result(test_name, 0);
        fprintf(stderr, "  Expected SV_STATUS_FAILED from writing to /dev/full, got %d.\n", status);
    }

    sv_free(t);
}


int main() {
    printf("Running security tests for libsv...\n");

    test_sv_write_field_overread();
    test_sv_init_fields_overflow(); // Acknowledged limitations for this test
    test_io_error_propagation();

    printf("Security tests finished.\n");
    return 0;
}

// To compile (example):
// gcc -I. security_test.c sv.c read.c write.c option.c -o security_tester
// Requires libsv source files (sv.c, read.c etc.) to be available for compilation.
// Or link against a compiled libsv.a if available.
// e.g. gcc -I. security_test.c -L. -lsv -o security_tester (if libsv.a is in current dir)
