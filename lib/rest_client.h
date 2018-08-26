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

/**
 * @file rest_client.h
 * @brief This module contains a generic REST client that is built upon
 * libcurl to easily make RESTful calls with resuable content filter chains.
 * @defgroup REST_API REST Client
 * @brief Easy to use REST client based on libcurl.
 * @{
 */

#ifndef REST_CLIENT_H_
#define REST_CLIENT_H_

#include <curl/curl.h>
#ifdef _PTHREADS
#include <pthread.h>
#endif

#include "object.h"

/**
 * Compile-time constant for the maximum number of HTTP headers to be passed
 * in a single RestRequest.
 */
#define MAX_HEADERS 64
/**
 * Compile-time constant for the maximum size of an error message.
 */
#define ERROR_MESSAGE_SIZE 255

// Some standard HTTP headers
/** MIME type of the object, e.g. image/jpeg */
#define HTTP_HEADER_CONTENT_TYPE "Content-Type"
/** Size of the response in bytes */
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length"
/** Request only part of a resource */
#define HTTP_HEADER_RANGE "Range"
#define HTTP_HEADER_CONTENT_RANGE "Content-Range"
/** Defines the Content-Types that will be accepted in a response */
#define HTTP_HEADER_ACCEPT "Accept"
/** Date the request or response was sent */
#define HTTP_HEADER_DATE "Date"
/** Used to define the location of a response */
#define HTTP_HEADER_LOCATION "Location"

/**
 * Allowed methods for REST operations.
 */
enum http_method {
	HTTP_POST,
	HTTP_GET,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_HEAD,
	HTTP_OPTIONS,
	HTTP_PATCH
};

/** Class name for RestResponse */
#define CLASS_REST_RESPONSE "RestResponse"


/**
 * This is a standard response from REST operations.  Do not modify this object
 * directly; instead use the RestResponse_xxx methods.
 */
typedef struct {
	/** Parent class's fields */
	Object parent;
	/**
	 * HTTP code of the last operation.  If zero, see curl_error for low-level
	 * transport errors (e.g. could not resolve host).  Note that you should
	 * always also check curl_error to make sure it's zero even when http-code
	 * indicates success since it's possible that the HTTP request succeeded
	 * but libcurl was unable to complete the transfer (e.g. disk full,
	 * out of memory, buffer full, or connection closed before content
	 * was completely received).
	 */
	int http_code;
	/**
	 * HTTP status line of the last operation.  This is especially useful for
	 * getting error information in situations where there is no response body
	 * like a DELETE or PUT.
	 */
	char http_status[ERROR_MESSAGE_SIZE];
	/**
	 * Error code from libcurl.  On success, this should be zero.  When it is
	 * nonzero, you can test with CURLE_XXX macros or check curl_error_message
	 * for more information.
	 */
	int curl_error;
	/**
	 * Contains a textual error message from libcurl
	 */
	char curl_error_message[CURL_ERROR_SIZE];
	/** Response headers parsed from the HTTP response. */
	char *response_headers[MAX_HEADERS];
	/** Number of response headers present in the response_headers array */
	int response_header_count;
	/**
	 * Content type of the response.  On operations that have no response
	 * body (e.g. DELETE and PUT), this will be NULL.
	 */
	char *content_type;
	/**
	 * Bytes in the response.  When writing into memory, this will be the
	 * actual number of bytes present in the body member.  When writing to a
	 * file, this will be the number of bytes written to the file.
	 */
	int64_t content_length;
	/**
	 * The buffer containing the response body.
	 */
	char *body;
	/**
	 * If use_buffer is nonzero, this is the maximum number of bytes that
	 * can be written into the body buffer.  Used when the user passes in a
	 * static buffer to fill with the response.
	 */
    size_t buffer_size;
    /**
     * If nonzero, the user has provided us with a static buffer to fill with
     * the response.  The buffer_size member indicates the size of this buffer.
     * This also flags the destructor to not free the buffer since we did not
     * allocate it.
     */
    int use_buffer;
    /**
     * If not-NULL, the response body will be written to this file handle
     * instead of memory.
     */
	FILE *file_body;
	/**
	 * When using file_body, we keep track of the offset just before the HTTP
	 * operation in case we need to rewind.
	 */
	off_t file_body_start_pos;
} RestResponse;

/**
 * Initializes a RestResponse object.
 * @param self the RestResponse object to initialize.
 * @return the RestResponse object (same as self)
 */
RestResponse *RestResponse_init(RestResponse *self);
/**
 * Destroys a RestResponse object.
 * @param self the RestResponse object to destroy.
 */
void RestResponse_destroy(RestResponse *self);
/**
 * Adds an HTTP response header to the object.
 * @param self the RestResponse to modify.
 * @param header the HTTP response header to add to the object.
 */
void RestResponse_add_header(RestResponse *self, const char *header);

/**
 * Gets an existing HTTP header from the response.  If the response does not
 * contain the header in question, NULL is returned.  Note that if the response
 * contains the same header more than once, only the first instance will be
 * returned.
 * @param self the RestResponse to search of the header.
 * @param header_name the name (case-insensitive) of the header to retrieve.
 * @return the header, generally in the format of "name: value".  Do not modify
 * this value.
 */
const char *RestResponse_get_header(RestResponse *self, const char *header_name);

/**
 * Similar to RestResponse_get_header, but returns only the value portion of
 * the header (everything past the first colon).
 * @param self the RestResponse to search of the header.
 * @param header_name the name (case-insensitive) of the header to retrieve.
 * @return the header, generally in the format of "name: value".  Do not modify
 * this value.
 */
const char *RestResponse_get_header_value(RestResponse *self,
        const char *header_name);


/**
 * This function can be used to preconfigure a static response buffer to
 * capture the response.  This can be used to improve memory efficiency
 * especially when downloading files by reusing the same buffer mutliple
 * times.  This buffer will be set on the body member of the RestResponse
 * object and will NOT be freed by the destructor.
 * @param self the RestResponse to modify.
 * @param buffer the buffer to use for the response.
 * @param buffer_size the size of the buffer.
 */
void RestResponse_use_buffer(RestResponse *self, char *buffer,
                             size_t buffer_size);
/**
 * This function can be used to configure the RestResponse to store the response
 * body in a file.  The file pointer will not be closed or reset at the end of
 * the operation.
 * @param self the RestResponse to modify.
 * @param f the file pointer to use to store the response.
 */
void RestResponse_use_file(RestResponse *self, FILE *f);

/**
 * Defines a RestRequestBody that is an optional component to a RestRequest.
 */
typedef struct {
	/** Type of content, e.g. "text/plain" or "image/jpeg" */
	const char *content_type;
	/** Number of bytes to write */
	int64_t data_size;
	/** Bytes written to the request stream */
	int64_t bytes_written;
	/** Bytes remaining to write to the request stream */
	int64_t bytes_remaining;
	/** Memory buffer containing the request (NULL if using file_body) */
	const char *body;
	/** File pointiner containing the request data (NULL if using body) */
	FILE *file_body;
	/** Optional pointer to a function to filter a file_body */
	void *filter;
} RestRequestBody;

/** Class name for RestRequest */
#define CLASS_REST_REQUEST "RestRequest"

/**
 * Defines the RestRequest object.  This is the base object containing a REST
 * request's content.  Can be subclassed to proivde extra parameters used by
 * RestFilter objects when processing a request.
 */
typedef struct {
	/** Parent class's fields */
	Object parent;
	/** The HTTP operation for the request (e.g. GET or PUT) */
	enum http_method method;
	/** The URI for the request (e.g. /service/version) */
	char *uri;
	/**
	 * If nonzero, the URI is already properly encoded and should not be
	 * escaped again.
	 */
	int uri_encoded;
	/** HTTP request headers, (e.g. x-emc-date) */
	char *headers[MAX_HEADERS];
	/** Number of request headers */
	int header_count;
	/**
	 * Optional body for the request.  Will be NULL for requests that do not
	 * contain a body (e.g. GET, HEAD, and DELETE requests).
	 */
	RestRequestBody *request_body;
} RestRequest;

/**
 * Handler callback used to filter file data as it's read.  Useful for
 * examining the data stream, modifying bytes, tracking progress,
 * or calculating a checksum.
 * @param data the data read from the file
 * @param data_size number of bytes in data.
 * @return 1 to continue processing, 0 to abort the http request.
 */
typedef int (*rest_file_data_filter)(RestRequest *request, char *data,
        size_t data_size);

/**
 * Initializes a new RestRequest object.
 * @param self the RestRequest object to initialize.
 * @param uri the URI for the request (e.g. /service/version).
 * @param method the HTTP method for the request (e.g. GET or PUT).
 * @return the RestRequest object (same as self).
 */
RestRequest *RestRequest_init(RestRequest *self, const char *uri,
		enum http_method method);
/**
 * Destroys a RestRequest.
 * @param self the RestRequest to destroy.
 */
void RestRequest_destroy(RestRequest *self);
/**
 * Sets the RestRequest's body to an in-memory byte array.
 * @param self the RestRequest to configure.
 * @param data the byte array containing the body data.
 * @param data_size the number of bytes to read from the byte array.
 * @param content_type the content type (MIME type) of the data, e.g.
 * "text/plain" or "image/jpeg".
 */
void RestRequest_set_array_body(RestRequest *self, const char *data,
        int64_t data_size, const char *content_type);
/**
 * Sets the RestRequest's body to the contents of a file pointer.
 * @param self the RestRequest to configure.
 * @param data the file pointer to read the body data from.
 * @param data_size the number of bytes to read from the file pointer.
 * @param content_type the content type (MIME type) of the data, e.g.
 * "text/plain" or "image/jpeg".
 */
void RestRequest_set_file_body(RestRequest *self, FILE *data, int64_t data_size,
		const char *content_type);
/**
 * Adds an HTTP header to the request.
 * @param self the RestRequest to configure.
 * @param header the HTTP header to add.  The header should be in the format,
 * "name: value", e.g. "Accept: application/json"
 */
void RestRequest_add_header(RestRequest *self, const char *header);

/**
 * Gets an existing HTTP header from the request.  If the request does not
 * contain the header in question, NULL is returned.  Note that if the request
 * contains the same header more than once, only the first instance will be
 * returned.
 * @param self the RestRequest to search of the header.
 * @param header_name the name (case-insensitive) of the header to retrieve.
 * @return the header, generally in the format of "name: value".  Do not modify
 * this value.
 */
const char *RestRequest_get_header(RestRequest *self, const char *header_name);

/**
 * Similar to RestRequest_get_header, but returns only the value portion of
 * the header (everything past the first colon).
 * @param self the RestRequest to search of the header.
 * @param header_name the name (case-insensitive) of the header to retrieve.
 * @return the header, generally in the format of "name: value".  Do not modify
 * this value.
 */
const char *RestRequest_get_header_value(RestRequest *self,
        const char *header_name);

/**
 * Sets the file filter for a request.  Only valid if the request has a body
 * and that body is reading from a file.
 * @param self the RestRequest to modify.
 * @param filter the rest_file_data_filter to call when reading data.  Set to
 * NULL to turn off.
 */
void
RestRequest_set_file_filter(RestRequest *self, rest_file_data_filter filter);

/** Class name for RestClient */
#define CLASS_REST_CLIENT "RestClient"

/**
 * Configuration for a RestClient.  This includes connection information like
 * host and port, proxy information, and other internal state information.  It
 * is not recommended to modify this struct directly; instead use the
 * RestClient_xxx methods.  This object is designed to be thread-safe when
 * compiled with libpthreads and utilizes libcurl's shared-state methods
 * (curl_shared) to share connections, cookies, and SSL handles between threads.
 */
typedef struct {
	/** Parent class's fields */
	Object parent;
	/**
	 * Host name or IP of REST server.  You can prefix this with http:// or
	 * https:// to force SSL on or off.
	 */
	char *host;
	/** Port number, generally 80 or 443 */
	int port;
	/** Host name or IP of proxy (NULL disables) */
	char *proxy_host;
	/** Use -1 for default */
	int proxy_port;
	/** User to authenticate with proxy (NULL disables) */
	char *proxy_user;
	/** Password for proxy user */
	char *proxy_pass;
	/** Internal data, do not modify */
	void *internal;
} RestClient;


/**
 * Initializes a RestClient object.
 * @param self pointer to the RestClient.
 * @param host the host name or IP of the REST server.  You can prefix this
 * with http:// or https:// to force SSL on or off.
 * @param port the port of the REST server.  Generally, this is 80 for HTTP or
 * 443 for HTTPS.
 * @return the pointer to the RestClient (same as self)
 */
RestClient *RestClient_init(RestClient *self, const char *host, int port);

/**
 * Destroys a RestClient object and its shared-state information.
 * @param self pointer to the RestClient to destroy.
 */
void RestClient_destroy(RestClient *self);

/**
 * Configures a RestClient to use a proxy.
 * @param self the RestClient to configure.
 * @param proxy_host the host name or IP address of the proxy server.  Set to
 * NULL to disable proxying.
 * @param proxy_port the port number of the proxy.  Use -1 to use the default
 * port of 1080.
 * @param proxy_user the username to authenticate the proxy.  Set to NULL to
 * disable proxy authentication.
 * @param proxy_pass if proxy_user is specified, the password for the proxy.
 * If null, no password will be used.
 */
void RestClient_set_proxy(RestClient *self, const char *proxy_host,
		int proxy_port, const char *proxy_user, const char *proxy_pass);

/**
 * Handler callback to perform some sort of configuration on a cURL handle before
 * it's executed (e.g. set custom headers, verbose logging, etc).  Note that a
 * handler may be called by multiple threads concurrently so it should be
 * thread-safe.
 * @param rest the pointer to the rest configuration object.
 * @param handle the handle to the current cURL connection object.
 * @return nonzero to abort processing the request.
 */
typedef int (*rest_curl_config_handler)(RestClient *rest, CURL *handle);


/**
 * Internal private state for RestClient.
 */
typedef struct {
	/** Shared-state information for CURL */
	CURLSH *curl_shared;
#ifdef _PTHREADS
	/** Mutex used by CURL for accessing the shared-state object */
	pthread_mutex_t curl_lock;
#endif
	/**
	 * Array of functions implementing rest_curl_config_handler to configure
	 * the low-level CURL request object.
	 */
	rest_curl_config_handler *handlers;
	/**
	 * Number of CURL config handler functions in the array.
	 */
	int curl_config_handler_count;
} RestPrivate;

/**
 * This is a CURL configuration function that enables verbose logging of the
 * request and response via CURL.  See RestClient_add_curl_config_handler().
 * @param rest the RestClient executing the request.
 * @param handle the CURL handle
 */
int rest_verbose_config(RestClient *rest, CURL *handle);
/**
 * This is a CURL configuration function that will check the RestClient to
 * see if a proxy is set and configure the underlying CURL handle to use that
 * proxy.  This handler is added to the RestClient by default.
 * @param rest the RestClient executing the request.
 * @param handle the CURL handle
 */
int rest_proxy_config(RestClient *rest, CURL *handle);

/**
 * This is a CURL configuration function that will disable SSL certificate
 * validation, including host validation.  THIS IS INSECURE and can be easily
 * exploited to implement a man-in-the-middle attack.  Therefore, this should
 * only be used for testing SSL with a self-signed certificate.  See
 * RestClient_add_curl_config_handler().
 * @param rest the RestClient executing the request.
 * @param handle the CURL handle
 */
int rest_disable_ssl_cert_check(RestClient *rest, CURL *handle);

/**
 * This is an internal CURL configuration function that configures the
 * underlying CURL handle to use shared-state information like Cookies,
 * SSL state, and Connection objects.  This handler is added to the
 * RestClient by default.
 * @param rest the RestClient executing the request.
 * @param handle the CURL handle
 */
int rest_curl_shared_config(RestClient *rest, CURL *handle);

/**
 * A linked list of filter functions to apply to the request.
 */
typedef struct RestFilterTag {
	/** A function pointer implementing rest_http_filter */
	void *func;
	/**
	 * The next RestFilter to execute.  If NULL, this is the last filter
	 * in the chain.
	 */
	struct RestFilterTag *next;
} RestFilter;

/**
 * An HTTP request filter. Filters do operations such as logging, retrying,
 * authenticating, and parsing responses.  Each filter is responsible for
 * checkin self->next for the next filter and passing the request down the
 * chain.
 * @param self the currently executing filter
 * @param rest the REST endpoint configuration
 * @param request the REST request object
 * @param response the REST response object
 */
typedef void (*rest_http_filter)(RestFilter *self, RestClient *rest,
		RestRequest *request, RestResponse *response);

/**
 * Executes a REST request.
 * @param self the RestClient used to execute the request.
 * @param filters the linked list of RestFilter objects to filter the request
 * and the response.
 * @param request the request to execute.
 * @param response the object to capture the response.
 */
void RestClient_execute_request(RestClient *self, RestFilter *filters,
		RestRequest *request, RestResponse *response);

/**
 * Adds a curl config handler to the client.  Handlers are executed in the order
 * they are added.
 */
void RestClient_add_curl_config_handler(RestClient *boss,
		rest_curl_config_handler handler);

/**
 * Adds a filter to the head of the filter chain.  Filters are executed in the
 * order they are added.  Requests flow "through" them so the last filter
 * executed during the request phase will be the first to be run during
 * the response as the stack unwinds.
 * @param start the first filter in the chain or NULL if the chain doesn't
 * exist yet.
 * @param next the filter to add to the chain
 * @return the first filter in the chain.  If the chain parameter was NULL,
 * this is a new chain.
 */
RestFilter *RestFilter_add(RestFilter *start, rest_http_filter next);

/**
 * Frees a filter chain.
 * @param chain the head of the RestFilter chain.
 */
void RestFilter_free(RestFilter *chain);

/**
 * This RestFilter function sets the Content-Type and Content-Length headers
 * on the request and parses them from the response stream (if not already
 * set).
 * @param self the RestFilter that's executing.
 * @param rest the RestClient processing the request.
 * @param request the REST request object.
 * @param response the object receiving the REST response.
 */
void RestFilter_set_content_headers(RestFilter *self, RestClient *rest,
		RestRequest *request, RestResponse *response);

/**
 * This is usually the last filter in the chain and executes the HTTP request
 * using cURL.
 * @param self the RestFilter that's executing.
 * @param rest the RestClient processing the request.
 * @param request the REST request object.
 * @param response the object receiving the REST response.
 */
void RestFilter_execute_curl_request(RestFilter *self, RestClient *rest,
		RestRequest *request, RestResponse *response);

/**
 * @}
 */
#endif /* REST_CLIENT_H_ */
