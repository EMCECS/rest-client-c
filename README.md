This API is required by the atmo-client -c which allows C developers to easily connect to an Atmos based Storage cloud, performing the REST requests and responses.

---
# Requirements
* libcurl 7.20 or greater

__Ubuntu:__

        sudo apt-get install libcurl.x86_64 libcurl4-openssl-dev 
        
__CentOS/RHEL:__

        sudo yum install libcurl-devel.x86_64 libcurl.x86_64 

---
# Building 
This package uses autotools, so the standard GNU build procedure can be used:

    ./configure
    make
    sudo make install

This installation places the librest libraries in the /usr/local/lib directory where atmos-client-c will check for this dependency.

---
# Using
## Important Note
Since this client is based on libcurl, it's imperative that you call

```
        curl_global_init(CURL_GLOBAL_DEFAULT);
```

Before executing any requests and

```
	curl_global_cleanup();
```

Before exiting your application.  Otherwise, you will cause memory leaks.

## Executing Requests
To execute requests, you'll first create a RestClient configured for the endpoint (host/port) you want to connect to.  From there, you'll execute multiple requests using the client.  RestClient objects are thread-safe when the library is compiled with pthread support (default).  When using multiple threads, libcurl will maintain a connection pool as long as you're using the same RestClient instance.

Each request is executed using a 'chain' of RestFilter functions.  At a minimum, you'll need to include the `RestFilter_execute_curl_request` function in your chain to execute the request.  The chain is a linked list and handlers are added to the _front_ of the chain.  Therefore, you should add `RestFilter_execute_curl_request` first so it gets executed last.  Requests flow from the first handler to the last, and then back up to the first.  This gives each handler a chance to modify the request before it executes and a chance to examine the response before the client application sees it.  See the atmos-client-c project for examples of using multiple handlers (near the top of `lib/atmos_client.c`).

A bare-bones HTTP request looks like this:

```
	// Load the root of the server.
	RestClient c;
	RestRequest req;
	RestResponse res;
	RestFilter* chain = NULL;
    
	RestClient_init(&c, "www.google.com", 80);
	RestRequest_init(&req, "/", HTTP_GET);
	RestResponse_init(&res);
    
	chain = RestFilter_add(chain, &RestFilter_execute_curl_request);
	RestClient_execute_request(&c, chain, &req, &res);

        // Process your response here.
    
	RestFilter_free(chain);
	RestResponse_destroy(&res);
	RestRequest_destroy(&req);
	RestClient_destroy(&c);
```

Note that as stated above, if you're executing multiple requests to the same server you'd only initialize the RestClient once.  For more examples, see the tests/ subdirectory and the emcvipr/atmos-client-c project on GitHub.

