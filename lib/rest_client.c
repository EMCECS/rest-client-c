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
#include <inttypes.h>
#include <ctype.h>

#include "config.h"
#include "rest_client.h"

#define CONNECT_TIMEOUT 200
#define MAX_HEADER_SIZE 1024

void lock_function(CURL *handle, curl_lock_data data,
		curl_lock_access access, void *userptr) {
#ifdef _PTHREADS
	RestPrivate *private = (RestPrivate*)userptr;
	pthread_mutex_lock(&private->curl_lock);
#endif
}

void unlock_function(CURL *handle, curl_lock_data data, void *userptr) {
#ifdef _PTHREADS
	RestPrivate *private = (RestPrivate*)userptr;
	pthread_mutex_unlock(&private->curl_lock);
#endif
}

static const char *get_header_value(const char *header);

RestClient *RestClient_init(RestClient *self, const char *host, int port) {
	// Super init
	Object_init_with_class_name((Object*)self, CLASS_REST_CLIENT);

	memset(((void*)self)+sizeof(Object), 0, sizeof(RestClient) - sizeof(Object));

	self->host = strdup(host);
	self->port = port;

	// Init internal
	self->internal = malloc(sizeof(RestPrivate));
	memset(self->internal, 0, sizeof(RestPrivate));

	// Initialize cURL shared state.
	RestPrivate *private = self->internal;
	private->curl_shared = curl_share_init();
	curl_share_setopt(private->curl_shared, CURLSHOPT_LOCKFUNC, lock_function);
	curl_share_setopt(private->curl_shared, CURLSHOPT_UNLOCKFUNC, unlock_function);

#ifdef _PTHREADS
	pthread_mutex_init(&private->curl_lock, NULL);
#endif

	curl_share_setopt(private->curl_shared, CURLSHOPT_USERDATA, private);
	curl_share_setopt(private->curl_shared, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
	curl_share_setopt(private->curl_shared, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
	curl_share_setopt(private->curl_shared, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);

	// Add default handlers
	RestClient_add_curl_config_handler(self, rest_proxy_config);
	RestClient_add_curl_config_handler(self, rest_curl_shared_config);

	return self;
}

void RestClient_destroy(RestClient *self) {
	if(self->host) {
		free(self->host);
		self->host = NULL;
	}
	self->port = 0;

	if(self->internal) {
		RestPrivate *private = self->internal;
		if(private->curl_shared) {
			curl_share_cleanup(private->curl_shared);
			private->curl_shared = NULL;
		}
#ifdef _PTHREADS
		pthread_mutex_destroy(&private->curl_lock);
#endif
        if(private->handlers) {
            free(private->handlers);
        }
		free(private);
		self->internal = NULL;
	}

	if(self->proxy_host) {
		free(self->proxy_host);
		self->proxy_user = NULL;
	}
	if(self->proxy_user) {
		free(self->proxy_user);
		self->proxy_user = NULL;
	}
	if(self->proxy_pass) {
		free(self->proxy_pass);
		self->proxy_pass = NULL;
	}


	Object_destroy((Object*) self);
}

void RestClient_add_curl_config_handler(RestClient *self, rest_curl_config_handler handler) {
	RestPrivate *priv = self->internal;
	priv->handlers = realloc(priv->handlers,
			(priv->curl_config_handler_count+1) * sizeof(rest_curl_config_handler));
	priv->handlers[priv->curl_config_handler_count++] = handler;
}

void RestClient_set_proxy(RestClient *self, const char *proxy_host,
		int proxy_port, const char *proxy_user, const char *proxy_pass) {
	if(self->proxy_host) {
		free(self->proxy_host);
		self->proxy_user = NULL;
	}
	if(self->proxy_user) {
		free(self->proxy_user);
		self->proxy_user = NULL;
	}
	if(self->proxy_pass) {
		free(self->proxy_pass);
		self->proxy_pass = NULL;
	}

	if(proxy_host) {
		self->proxy_host = strdup(proxy_host);
	}
	self->proxy_port = proxy_port;
	if(proxy_user) {
		self->proxy_user = strdup(proxy_user);
	}
	if(proxy_pass) {
		self->proxy_pass = strdup(proxy_pass);
	}
}


// Standard handlers
int rest_curl_shared_config(RestClient *rest, CURL *handle) {
	RestPrivate *priv = rest->internal;
	if(priv->curl_shared) {
		curl_easy_setopt(handle, CURLOPT_SHARE, priv->curl_shared);
	}
	return 0;
}

int rest_verbose_config(RestClient *rest, CURL *handle) {
	curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);

	return 0;
}

int rest_proxy_config(RestClient *rest, CURL *handle) {
	// Proxy support
	if(rest->proxy_host != NULL) {
		curl_easy_setopt(handle, CURLOPT_PROXY, rest->proxy_host);
		if(rest->proxy_port != -1) {
			curl_easy_setopt(handle, CURLOPT_PROXYPORT, rest->proxy_port);
		}
		if(rest->proxy_user != NULL) {
#if LIBCURL_VERSION_NUM > 0x071300
			curl_easy_setopt(handle, CURLOPT_PROXYUSERNAME, rest->proxy_user);
			if(rest->proxy_pass != NULL) {
				curl_easy_setopt(handle, CURLOPT_PROXYPASSWORD, rest->proxy_pass);
			}
#else
			// Old way; doesn't accept users with ":" in it (e.g. sip:user@host)
			char auth[HEADER_MAX];
			sprintf(auth, "%s:%s", rest->proxy_user,
					rest->proxy_pass?rest->proxy_pass:"");
			curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, auth);
#endif
		}
	}
	return 0;
}

int rest_disable_ssl_cert_check(RestClient *rest, CURL *handle) {
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);

	return 0;
}

size_t readfunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if(!stream) {
    	return 0;
    }

    RestRequest *req = (RestRequest*)stream;
	RestRequestBody *ud = req->request_body;
	if(ud->bytes_remaining > 0) {
	    if(ud->bytes_remaining >= size*nmemb) {
	      if(ud->bytes_written == 0) {
	        memcpy(ptr, ud->body, size*nmemb);
	      } else {
	        memcpy(ptr, ud->body+ud->bytes_written, size*nmemb);
	      }
		ud->bytes_written+=size*nmemb;
		ud->bytes_remaining -=size*nmemb;
		return size*nmemb;
	    } else {
	      unsigned int datasize = (unsigned int)ud->bytes_remaining;
	      memcpy(ptr, ud->body+ud->bytes_written, datasize);
	      ud->bytes_written+=datasize;
	      ud->bytes_remaining=0;
	      return datasize;
	    }
	}
	return 0;
}

/**
 * The normal curl file readfunc reads the entire file, even though the curlopt
 * FILESIZE was set to the actual number of bytes we want.  This version will
 * stop when we reach the requested number of bytes.
 */
size_t readfunc_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t c;
    if(!stream) {
        return 0;
    }

    RestRequest *req = (RestRequest*)stream;
    RestRequestBody *ud = req->request_body;
    if(ud->bytes_remaining > 0) {
        if(ud->bytes_remaining >= size*nmemb) {
          c = fread(ptr, size, nmemb, ud->file_body);
          if(ud->filter) {
              if(!((rest_file_data_filter)ud->filter)(req, ptr, c)) {
                  return 0;
              }
          }
          ud->bytes_written += c;
          ud->bytes_remaining -= c;
          return c;
        } else {
          unsigned int datasize = (unsigned int)ud->bytes_remaining;
          c = fread(ptr, 1, datasize, ud->file_body);
          if(ud->filter) {
              if(!((rest_file_data_filter)ud->filter)(req, ptr, c)) {
                  return 0;
              }
          }
          ud->bytes_written += c;
          ud->bytes_remaining -= c;
          return c;
        }
    }
    return 0;
}


size_t writefunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
	RestResponse *ws = (RestResponse*)stream;

    void *new_response = NULL;
    unsigned long long data_offset = ws->content_length;

    size_t mem_required = size*nmemb;
    ws->content_length += mem_required;
    
    if(ws->use_buffer) {
        // Use the buffer provided
        if(ws->content_length > ws->buffer_size) {
            // Content length too large!
            // TODO: Logging
            return 0;
        }
    } else {
        /* Add an extra byte so we can null terminate it */
        new_response = realloc(ws->body,ws->content_length+1);
        if(new_response) {
          ws->body = (char*)new_response;
        } else {
          return 0; // Error
        }
    }

    if(data_offset) {
      memcpy(ws->body+data_offset,ptr, mem_required);
    } else {
      memcpy(ws->body,ptr, mem_required);
    }

    if(!ws->use_buffer
       || (ws->use_buffer && ws->content_length < ws->buffer_size)) {
        /* Null terminate the body in case it's a string*/
        /* Since the data is one longer there is room, but */
        /* shouldn't affect non-string data as long as the */
        /* body_size element is used when copying*/
        ws->body[ws->content_length] = 0;
    }
    return mem_required;
}

size_t headerfunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
    RestResponse *ws = (RestResponse*)stream;
    size_t mem_required = size*nmemb-2; // - 2 for the /r/n at the end of each header

    if(ws->response_header_count >= MAX_HEADERS) {
    	fprintf(stderr, "MAX_HEADERS reached parsing response.");
    	return 0; // Error
    }

    ws->response_headers[ws->response_header_count] = (char*)malloc(mem_required+1);//+1 for \0
    memcpy(ws->response_headers[ws->response_header_count], ptr, mem_required);
    ws->response_headers[ws->response_header_count][mem_required] = '\0';
    ws->response_header_count++;

    return size*nmemb;
}

void RestClient_execute_request(RestClient *self, RestFilter *filters,
		RestRequest *request, RestResponse *response) {

	if(!filters) {
		fprintf(stderr, "RestClient_execute_request called with no filters.");
		abort();
	} else {
		// Simply invoke the head of the filter chain.
		((rest_http_filter)filters->func)(filters, self, request, response);
	}
}

RestFilter *RestFilter_add(RestFilter *start, rest_http_filter next) {
	RestFilter *newfilter = calloc(sizeof(RestFilter), 1);

	newfilter->func = next;
	if(start) {
		newfilter->next = start;
	}

	return newfilter;
}

void RestFilter_free(RestFilter *start) {
	if(start->next) {
		RestFilter_free(start->next);
	}
	free(start);
}

/**
 * Sets the Content-Type and Content-Length headers
 */
void RestFilter_set_content_headers(RestFilter *self, RestClient *rest,
		RestRequest *request, RestResponse *response) {
	char headerbuf[MAX_HEADER_SIZE];

	if(request->request_body) {
	    /** Curl should set this itself */
		/* snprintf(headerbuf, MAX_HEADER_SIZE, "%s: %" PRId64,
		        HTTP_HEADER_CONTENT_LENGTH,
				(long long)request->request_body->data_size);
		RestRequest_add_header(request, headerbuf); */
		snprintf(headerbuf, MAX_HEADER_SIZE, "%s: %s", HTTP_HEADER_CONTENT_TYPE,
				request->request_body->content_type);
        RestRequest_add_header(request, headerbuf);
	} else if(request->method == HTTP_POST || request->method == HTTP_PUT || request->method == HTTP_PATCH) {
		// Zero-length body
		RestRequest_add_header(request, HTTP_HEADER_CONTENT_LENGTH ":0");
	}

	// Pass to the next filter
	if(self->next) {
		((rest_http_filter)self->next->func)(self->next, rest, request, response);
	}

	// Parse content-type from response
	int count = response->response_header_count;
	int i;
	for(i=0; i<count; i++) {
		char *header = response->response_headers[i];
		if(header && strstr(header, HTTP_HEADER_CONTENT_TYPE) == header) {
			char *content_type = header + strlen(HTTP_HEADER_CONTENT_TYPE);
			content_type++; // skip ":"

			// Might be leading spaces
			while(*content_type == ' ') {
				content_type++;
			}

            if(response->content_type) {
                // If one was already set, free it.
                free(response->content_type);
            }
			response->content_type = strdup(content_type);
			break;
		}
	}
}



void RestFilter_execute_curl_request(RestFilter *self, RestClient *rest,
		RestRequest *request, RestResponse *response) {
	CURL *curl = curl_easy_init();
    struct curl_slist *chunk = NULL;
    char *encoded_uri;
    char *endpoint_url;
    long http_code;
    size_t endpoint_size;
    size_t i,j;

    RestPrivate *priv = rest->internal;

	/* Encode the URI */
	encoded_uri = (char*)malloc(strlen(request->uri)*3+1); /* Worst case if every char was encoded */
	memset(encoded_uri, 0, strlen(request->uri)*3+1);

	if(request->uri_encoded) {
	    // URI is already encoded.
	    strcpy(encoded_uri, request->uri);
	} else {
        for(i=0,j=0; i<strlen(request->uri); i++) {
            if(request->uri[i] == '/') {
                encoded_uri[j++] = request->uri[i];
            } else if(request->uri[i] == '?') {
                /* Do the rest */
                strcat(encoded_uri, request->uri+i);
                break;
            } else {
                /* Encode the data */
                char *encoded;
                encoded = curl_easy_escape(curl, request->uri+i, 1);
                strcat(encoded_uri, encoded);
                curl_free(encoded);
                j = strlen(encoded_uri);
            }
        }
	}


	endpoint_size = strlen(rest->host)+strlen(encoded_uri) +1;
	endpoint_url = (char*)malloc(endpoint_size);

	snprintf(endpoint_url, endpoint_size, "%s%s", rest->host, encoded_uri);

	/* Done with encoded version */
	free(encoded_uri);

	curl_easy_setopt(curl, CURLOPT_URL, endpoint_url);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 0);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writefunc);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &headerfunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_WRITEHEADER, response);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, response->curl_error_message);

	switch(request->method) {

	case HTTP_POST:
	    curl_easy_setopt(curl, CURLOPT_POST, 1l);
	    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)0L);
	    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);

	    break;
	case HTTP_PUT:
	    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	    break;
	case HTTP_DELETE:
	    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	    break;
	case HTTP_HEAD:
	  curl_easy_setopt(curl, CURLOPT_NOBODY, 1l);
	    break;
	case HTTP_GET:

	  break;
	case HTTP_OPTIONS:
	  break;
	case HTTP_PATCH:
	  curl_easy_setopt(curl, CURLOPT_POST, 1L);
	  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
	  break;
	}


	if(request->request_body) {
	    if(request->method == HTTP_PUT) {
	        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
	                (curl_off_t)request->request_body->data_size);
	    } else {
	        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
	                (curl_off_t)request->request_body->data_size);
	    }

	  /* If a stream handle is used, use the file readfunc */
	  if(request->request_body->file_body && (request->method == HTTP_POST || request->method == HTTP_PUT || request->method == HTTP_PATCH)) {
		  curl_easy_setopt(curl, CURLOPT_READDATA, request);
          request->request_body->bytes_remaining = request->request_body->data_size;
		  curl_easy_setopt(curl, CURLOPT_READFUNCTION, readfunc_file);

	  } else {
		  curl_easy_setopt(curl, CURLOPT_READDATA, request);
		  request->request_body->bytes_remaining = request->request_body->data_size;
		  curl_easy_setopt(curl, CURLOPT_READFUNCTION, readfunc);
	  }
	}

	if(response->file_body) {
		/* Write to the stream */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, response->file_body);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        
        // Get the current offset so we know how many bytes were written
        response->file_body_start_pos = ftello(response->file_body);
	}


	// Set headers -- be sure to include Content-Length and Content-Type
	// if applicable.
	for(i=0;i<request->header_count; i++) {
	    if(request->headers[i] != NULL) {
		chunk = curl_slist_append(chunk, request->headers[i]);
	    } else {
		; //a null header thats been added to the array is proabbly a bug.
	    }
	}

	// Remove some headers
	chunk = curl_slist_append(chunk, "Expect:");
	chunk = curl_slist_append(chunk, "Transfer-Encoding:");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	// Call the handlers
	for(i=0; i<priv->curl_config_handler_count; i++) {
		if(priv->handlers[i](rest, curl)) {
			// Handler requested abort
			response->http_code = 0;
			response->curl_error = CURLE_ABORTED_BY_CALLBACK;
			sprintf(response->curl_error_message,
					"Request aborted by request handler");
			curl_easy_cleanup(curl);
			curl_slist_free_all(chunk);
			free(endpoint_url);
			return;
		}
	}

	// Execute the request
	response->curl_error = curl_easy_perform(curl);

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	response->http_code = (int)http_code;
	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &response->content_type);

	/* dup it so we can free later */
	if(response->content_type) {
		response->content_type = strdup(response->content_type);
	} else {
		response->content_type = NULL;
	}

    // First header should be the HTTP status line, e.g.
    // HTTP/1.1 200 OK
    // Capture the status message ("OK") since curl doesn't seem to do
    // this for us.
    if(response->response_header_count > 0) {

        int space = 0;
        char *line = response->response_headers[0];
        while(*line && space < 2) {
            if(*line == ' ') {
                space++;
            }
            line++;
        }
        if(space == 2) {
            // correctly found 2 spaces.
            strncpy(response->http_status, line, ERROR_MESSAGE_SIZE);
        }
    }

    
    // If we read a file, record the number of bytes
    if(response->file_body) {
        // Do this by getting the current ptr.
        off_t current_offset = ftello(response->file_body);
        response->content_length = current_offset - response->file_body_start_pos;
    }


	curl_easy_cleanup(curl);
	curl_slist_free_all(chunk);
	free(endpoint_url);
}


RestResponse *RestResponse_init(RestResponse *self) {
	Object_init_with_class_name((Object*)self, CLASS_REST_RESPONSE);

	memset(((void*)self)+sizeof(Object), 0, sizeof(RestResponse) - sizeof(Object));

	return self;
}

void RestResponse_destroy(RestResponse *self) {
	int i;

    // Free the body only if we allocated it.
	if(self->body && !self->use_buffer) {
		free(self->body);
	}
	for(i=0; i<self->response_header_count; i++) {
		if(self->response_headers[i]) {
			free(self->response_headers[i]);
		}
	}
	if(self->content_type) {
		free(self->content_type);
	}

	// Clear all our fields.
	memset(((void*)self)+sizeof(Object), 0, sizeof(RestResponse) - sizeof(Object));

	Object_destroy((Object*)self);
}

void RestResponse_use_buffer(RestResponse *self, char *buffer,
                             size_t buffer_size) {
    self->body = buffer;
    self->buffer_size = buffer_size;
    self->use_buffer = 1;
    self->file_body = NULL;
    
}
void RestResponse_use_file(RestResponse *self, FILE *f) {
    self->body = NULL;
    self->use_buffer = 0;
    self->file_body = f;
}


RestRequest *RestRequest_init(RestRequest *self, const char *uri, enum http_method method) {
	Object_init_with_class_name((Object*)self, CLASS_REST_REQUEST);
	OBJECT_ZERO(self, RestRequest, Object);

	self->uri = strdup(uri);
	self->method = method;

	return self;
}

void RestRequest_destroy(RestRequest *self) {
	int i;

	// Free the body if set
	if(self->request_body) {
		free(self->request_body);
		self->request_body = 0;
	}

	// Free the headers if set
	for(i = 0; i<self->header_count; i++) {
		if(self->headers[i]) {
			free(self->headers[i]);
			self->headers[i] = NULL;
		}
	}

	self->header_count = 0;
	free(self->uri);
	self->uri = NULL;
	self->method = 0;


	Object_destroy((Object*)self);
}

void RestRequest_set_array_body(RestRequest *self, const char *data,
        int64_t data_size, const char *content_type) {
	self->request_body = calloc(sizeof(RestRequestBody), 1);

	self->request_body->body = data;
	self->request_body->data_size = data_size;
	self->request_body->content_type = content_type;
}

void RestRequest_set_file_body(RestRequest *self, FILE *data, int64_t data_size,
		const char *content_type) {
	self->request_body = calloc(sizeof(RestRequestBody), 1);

	self->request_body->file_body = data;
	self->request_body->data_size = data_size;
	self->request_body->content_type = content_type;
}

void RestRequest_add_header(RestRequest *self, const char *header) {
	// We strdup the header so we can free it in the destructor.
	self->headers[self->header_count++] = strdup(header);
}

const char *RestRequest_strcsw(const char *haystack, const char *needle) {
    const char *start = haystack;

    while(*haystack && *needle) {
        char h = tolower(*haystack);
        char n = tolower(*needle);

        if(h == n) {
            haystack++;
            needle++;
        } else {
            // not equal.
            return NULL;
        }
    }

    if(*needle) {
        // didn't finish
        return NULL;
    } else {
        // Reached end of needle, found it
        return start;
    }
}

const char *RestRequest_strcasestr(const char *haystack, const char *needle) {
    size_t diff = strlen(haystack) - strlen(needle);

    // Stop when what's left of the haystack is smaller than the needle.
    const char *end = haystack + diff+1;

    while(*haystack && haystack < end) {
        const char *x = RestRequest_strcsw(haystack, needle);
        if(x) {
            return x;
        }
        haystack++;
    }

    return NULL;
}

const char *RestRequest_get_header(RestRequest *self, const char *header_name) {
    int i;
    const char *header;

    for(i=0; i<self->header_count; i++) {
        header = RestRequest_strcasestr(self->headers[i], header_name);
        if(header != NULL && header == self->headers[i]) {
            // Just to make sure, the next character should be a colon.
            if(header[strlen(header_name)] != ':') {
                continue;
            }

            return self->headers[i];
        }
    }
    return NULL;
}

static const char *get_header_value(const char *header) {
    // Find the colon
    header = strstr(header, ":");

    if(!header) {
        return NULL;
    }

    // Skip the colon
    header++;

    // Trim any leading whitespace.
    while(*header == ' ') {
        header++;
    }

    return header;
}

const char *RestRequest_get_header_value(RestRequest *self,
        const char *header_name) {
    const char *header = RestRequest_get_header(self, header_name);

    if(!header) {
        return NULL;
    }

    return get_header_value(header);
}

const char *RestResponse_get_header(RestResponse *self, const char *header_name) {
    int i;
    const char *header;

    for(i=0; i<self->response_header_count; i++) {
        if((header = RestRequest_strcasestr(self->response_headers[i], header_name)) != NULL &&
                header == self->response_headers[i]) {
            // Just to make sure, the next character should be a colon.
            if(header[strlen(header_name)] != ':') {
                continue;
            }

            return self->response_headers[i];
        }
    }
    return NULL;
}

void
RestRequest_set_file_filter(RestRequest *self, rest_file_data_filter filter) {
    if(!self->request_body) {
        return;
    }

    self->request_body->filter = filter;
}


const char *RestResponse_get_header_value(RestResponse *self,
        const char *header_name) {
    const char *header = RestResponse_get_header(self, header_name);

    if(!header) {
        return NULL;
    }

    return get_header_value(header);
}

void
RestResponse_add_header(RestResponse *self, const char *header) {
    self->response_headers[self->response_header_count++] = strdup(header);
}


