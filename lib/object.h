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
 * @file object.h
 * @brief This module contains the base object class definition and its methods
 * @defgroup Object_API Object Core
 * @brief Core Object functionality for the REST APIs.  This
 * API defines a core Object-based pattern for use in the other APIs.
 * @{
 */


#ifndef OBJECT_H_
#define OBJECT_H_

/** The class name for the base Object class */
#define CLASS_OBJECT "Object"
/**
 * The class name for destroyed objects.  You can check any object's class
 * name and if it's set to this, it has already been destroyed.
 */
#define CLASS_DESTROYED "<<Destroyed>>"

/**
 * The base Object class structure contains the textual name of the class;
 * useful for debugging.
 */
typedef struct {
	/** The name of the class (or CLASS_DESTROYED if destroyed) */
	const char *class_name;
} Object;

/**
 * Initializes a new object.  The object class name will be set to "Object".
 * @param self pointer to the object to initialize.
 * @return pointer to the new object (same as self).  Useful for assignment
 * during initialization.
 */
Object *Object_init(Object *self);

/**
 * Initializes a new object with the given class name.  This is used by
 * subclasses when calling the super constructor.
 * @param self pointer to the object to initialize.
 * @param class_name the name of the class getting initialized.
 * @return pointer to the new object (same as self).
 */
Object *Object_init_with_class_name(Object *self, const char *class_name);

/**
 * Destroys a object.  Sets the name of the class to "<<Destroyed>>" for 
 * debugging purposes.
 * @param self pointer to the object to destroy.
 */
void Object_destroy(Object *self);

/**
 * Gets the name of the object's class.
 * @param self pointer to the object to get the class name from.
 * @return the name of the class.  This is generally a static string and should
 * not be freed.
 */
const char *Object_get_class_name(Object *self);

/**
 * Handy macro to zero the fields of an object minus its parent class (assuming
 * the parent class does this on its own fields).
 */
#define OBJECT_ZERO(obj, class, parent_class) memset(((void*)obj) + sizeof(parent_class), 0, sizeof(class) - sizeof(parent_class))

/**
 * @}
 */

#endif /* OBJECT_H_ */
