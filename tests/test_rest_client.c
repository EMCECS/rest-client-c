/*

 Copyright (c) 2012, EMC Corporation

 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of the EMC Corporation nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "seatest.h"
#include "test.h"
#include "test_rest_client.h"
#include "rest_client.h"

#define TEST_HOST "http://www.google.com"
#define TEST_PORT 80

void test_rest_client_init() {
	RestClient c;

	RestClient_init(&c, TEST_HOST, TEST_PORT);

	assert_string_equal(CLASS_REST_CLIENT, Object_get_class_name((Object*)&c));

	RestClient_destroy(&c);
}

void test_rest_client_destroy() {
	RestClient c;

	RestClient_init(&c, TEST_HOST, TEST_PORT);

	assert_string_equal(CLASS_REST_CLIENT, Object_get_class_name((Object*)&c));

	RestClient_destroy(&c);
	assert_string_equal(CLASS_DESTROYED, Object_get_class_name((Object*)&c));
}

void test_rest_client_execute() {
	// Load the root of the server.
	RestClient c;
	RestRequest req;
	RestResponse res;
	RestFilter* chain = NULL;
    
    
	RestClient_init(&c, "www.google.com", 443);
	RestRequest_init(&req, "/", HTTP_GET);
	RestResponse_init(&res);
    
	chain = RestFilter_add(chain, &RestFilter_execute_curl_request);
	RestClient_execute_request(&c, chain, &req, &res);
	RestFilter_free(chain);
    
	assert_int_equal(0, res.curl_error);
	assert_int_equal(200, res.http_code);
    
	RestResponse_destroy(&res);
	RestRequest_destroy(&req);
	RestClient_destroy(&c);
}

#ifdef _PTHREADS
#define NUM_THREADS 100

typedef struct {
	RestClient *c;
	int status;
} TestData;

void *exec_request(void *private) {
	TestData *data = (TestData*)private;
	RestClient *c = data->c;
        RestRequest req;
        RestResponse res;
        RestFilter* chain = NULL;

        RestRequest_init(&req, "/", HTTP_GET);
        RestResponse_init(&res);

        chain = RestFilter_add(chain, &RestFilter_execute_curl_request);
        RestClient_execute_request(c, chain, &req, &res);
        RestFilter_free(chain);

        assert_int_equal(0, res.curl_error);
        assert_int_equal(200, res.http_code);

	data->status = res.http_code;

        RestResponse_destroy(&res);
        RestRequest_destroy(&req);
	pthread_exit(private);
}

void test_rest_client_threads() {
	pthread_t thread[NUM_THREADS];
	pthread_attr_t attr;
	int rc;
	long t;
	RestClient c;
	TestData *data;

        RestClient_init(&c, "www.google.com", 80);

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(t=0; t<NUM_THREADS; t++) {
		data = malloc(sizeof(TestData));
		data->c = &c;
		data->status = 0;
		printf("Main: creating thread %ld\n", t);
		rc = pthread_create(&thread[t], &attr, exec_request, (void *)data);  
		assert_int_equal(0, rc);	
	}

	/* Free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);
	for(t=0; t<NUM_THREADS; t++) {
		rc = pthread_join(thread[t], (void*)&data);
		assert_int_equal(200, data->status);
		printf("Main: completed join with thread %ld having a status of %d\n",t, data->status);
		free(data);
	}

	RestClient_destroy(&c);
}
#endif

void test_rest_client_execute_with_buffer() {
	// Load the root of the server. Use a predefined buffer to fill.
    size_t buffer_size = 1024*1024;
    char *buffer = NULL;
	RestClient c;
	RestRequest req;
	RestResponse res;
	RestFilter* chain = NULL;
    
    
	RestClient_init(&c, "www.google.com", 80);
	RestRequest_init(&req, "/", HTTP_GET);
	RestResponse_init(&res);
    
    // Init the buffer.
    buffer = malloc(buffer_size);
    RestResponse_use_buffer(&res, buffer, buffer_size);
    
	chain = RestFilter_add(chain, &RestFilter_execute_curl_request);
	RestClient_execute_request(&c, chain, &req, &res);
	RestFilter_free(chain);
    
	assert_int_equal(0, res.curl_error);
	assert_int_equal(200, res.http_code);
    
	RestResponse_destroy(&res);
	RestRequest_destroy(&req);
	RestClient_destroy(&c);
    free(buffer);
}

void test_rest_client_execute_with_too_small_buffer() {
	// Load the root of the server. Use a predefined buffer to fill.
    // This buffer will be too small, so the request should fail!
    size_t buffer_size = 1;
    char *buffer = NULL;
	RestClient c;
	RestRequest req;
	RestResponse res;
	RestFilter* chain = NULL;
    
    
	RestClient_init(&c, "www.google.com", 80);
	RestRequest_init(&req, "/", HTTP_GET);
	RestResponse_init(&res);
    
    // Init the buffer.
    buffer = malloc(buffer_size);
    RestResponse_use_buffer(&res, buffer, buffer_size);
    
	chain = RestFilter_add(chain, &RestFilter_execute_curl_request);
	RestClient_execute_request(&c, chain, &req, &res);
	RestFilter_free(chain);
    
	assert_int_equal(CURLE_WRITE_ERROR, res.curl_error);
	assert_int_equal(200, res.http_code);
    
	RestResponse_destroy(&res);
	RestRequest_destroy(&req);
	RestClient_destroy(&c);
    free(buffer);
}


#define TEST_HEADER "THIS IS A HEADER"
#define TEST_CONTENT_TYPE "text/plain"

void test_rest_request() {
	RestRequest req;

	RestRequest_init(&req, "/", HTTP_GET);
	assert_string_equal(CLASS_REST_REQUEST, Object_get_class_name((Object*)&req));

	RestRequest_add_header(&req, TEST_HEADER);
	assert_int_equal(1, req.header_count);
	assert_string_equal(TEST_HEADER, req.headers[0]);

	RestRequest_set_array_body(&req, TEST_HEADER, strlen(TEST_HEADER), TEST_CONTENT_TYPE);
	assert_string_equal(TEST_HEADER, req.request_body->body);
	assert_int_equal(strlen(TEST_HEADER), (int)req.request_body->data_size);
	assert_string_equal(TEST_CONTENT_TYPE, req.request_body->content_type);

	RestRequest_destroy(&req);

	assert_true(req.request_body == NULL);
	assert_true(req.header_count == 0);
	assert_true(req.uri == NULL);
}

void test_rest_client_suite() {
	test_fixture_start();
	curl_global_init(CURL_GLOBAL_DEFAULT);


	start_test_msg("test_rest_client_init");
	run_test(test_rest_client_init);
	start_test_msg("test_rest_client_destroy");
	run_test(test_rest_client_destroy);
	start_test_msg("test_rest_client_execute");
	run_test(test_rest_client_execute);
	start_test_msg("test_rest_client_execute_with_buffer");
	run_test(test_rest_client_execute_with_buffer);
	start_test_msg("test_rest_client_execute_with_too_small_buffer");
	run_test(test_rest_client_execute_with_too_small_buffer);
#ifdef _PTHREADS
	start_test_msg("test_rest_client_threads");
	run_test(test_rest_client_threads);
#endif
    
	curl_global_cleanup();
	test_fixture_end();

}
