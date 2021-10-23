#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <purple.h>
#include "jabber.h"

#include "libomemo.h"

#include "../src/lurch_addr.h"
#include "../src/lurch_crypto.h"

/**
 * The mock asserts the function was called, checks the addresses name member via 'name', and returns a mocked int.
 */
int __wrap_axc_message_encrypt_and_serialize(axc_buf * msg_p, const axc_address * recipient_addr_p, axc_context * ctx_p, axc_buf ** ciphertext_pp) {
    int ret_val;

    function_called();
    const char * name = recipient_addr_p->name;
    check_expected(name);

    // the buffer is freed by the caller, so alloc here to prevent segfault
    axc_buf * mock_buf = axc_buf_create(NULL, 0);
    *ciphertext_pp = mock_buf;

    ret_val = mock_type(int);
    return ret_val;
}

/**
 * The mock asserts the function was called, checks addr_p, and returns a mocked int.
 */
int __wrap_axc_session_exists_initiated(const axc_address * addr_p, axc_context * ctx_p) {
    int ret_val;

    function_called();
    check_expected_ptr(addr_p);

    ret_val = mock_type(int);
    return ret_val;
}

/**
 * When the list of recipients is empty, nothing happens.
 */
static void test_lurch_crypto_encrypt_msg_for_addrs_empty_list(void ** state) {
    (void) state;

    assert_return_code(lurch_crypto_encrypt_msg_for_addrs(NULL, NULL, NULL), 0);
}

/**
 * When no session exists, nothing happens.
 */
static void test_lurch_crypto_encrypt_msg_for_addrs_no_sessions(void ** state) {
    (void) state;

    lurch_addr test_addr = {
        .jid = "wanted@jabber.id",
        .device_id = 1234
    };

    expect_function_call(__wrap_axc_session_exists_initiated);
    expect_string(__wrap_axc_session_exists_initiated, addr_p, &test_addr);
    will_return(__wrap_axc_session_exists_initiated, 0);

    assert_return_code(lurch_crypto_encrypt_msg_for_addrs(NULL, g_list_append(NULL, &test_addr), NULL), 0);
}

/**
 * On error, passes on the status code.
 */
static void test_lurch_crypto_encrypt_msg_for_addrs_err(void ** state) {
    (void) state;

    lurch_addr test_addr = {
        .jid = "wanted@jabber.id",
        .device_id = 1234
    };

    const int wanted_errcode = -1337;
    expect_function_call(__wrap_axc_session_exists_initiated);
    expect_string(__wrap_axc_session_exists_initiated, addr_p, &test_addr);
    will_return(__wrap_axc_session_exists_initiated, wanted_errcode);

    assert_int_equal(lurch_crypto_encrypt_msg_for_addrs(NULL, g_list_append(NULL, &test_addr), NULL), wanted_errcode);
}

/**
 * Encrypts the OMEMO key with the recipients Signal session and adds a recipient element to the omemo message.
 */
static void test_lurch_crypto_encrypt_msg_for_addrs_happy(void ** state) {
    (void) state;

    lurch_addr test_addr = {
        .jid = "wanted@jabber.id",
        .device_id = 1234
    };

    expect_function_call(__wrap_axc_session_exists_initiated);
    expect_string(__wrap_axc_session_exists_initiated, addr_p, &test_addr);
    will_return(__wrap_axc_session_exists_initiated, 1);

    expect_function_call(__wrap_axc_message_encrypt_and_serialize);
    expect_string(__wrap_axc_message_encrypt_and_serialize, name, test_addr.jid);
    will_return(__wrap_axc_message_encrypt_and_serialize, 0);

    omemo_message * om_msg_p;
    assert_int_equal(omemo_message_create(789, &crypto, &om_msg_p), 0);

    assert_return_code(lurch_crypto_encrypt_msg_for_addrs(om_msg_p, g_list_append(NULL, &test_addr), NULL), 0);

    uint8_t * key_buf_p = NULL;
    size_t key_buf_size = 234;
    assert_int_equal(omemo_message_get_encrypted_key(om_msg_p, test_addr.device_id, &key_buf_p, &key_buf_size), 0);
    assert_non_null(key_buf_p);
    assert_int_equal(key_buf_size, 0); // the mock enctyption returns a buffer of length 0
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_lurch_crypto_encrypt_msg_for_addrs_empty_list),
        cmocka_unit_test(test_lurch_crypto_encrypt_msg_for_addrs_no_sessions),
        cmocka_unit_test(test_lurch_crypto_encrypt_msg_for_addrs_err),
        cmocka_unit_test(test_lurch_crypto_encrypt_msg_for_addrs_happy)
    };

    return cmocka_run_group_tests_name("lurch_crypto", tests, NULL, NULL);
}
