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

This installation places the librest libraries in the /usr/local/lib directory