// C++ informative line for the emacs editor: -*- C++ -*-
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __H5ArrayType_H
#define __H5ArrayType_H

namespace H5 {

/*! \class ArrayType
    \brief Class ArrayType inherits from DataType and provides wrappers for
     the HDF5's Array Datatypes.

    Inheritance: DataType -> H5Object -> H5Location -> IdComponent
*/
class H5_DLLCPP ArrayType : public DataType {
   public:
	// Constructor that creates a new array data type based on the
	// specified base type.
	ArrayType(const DataType& base_type, int ndims, const hsize_t* dims);

	// Assignment operator
	ArrayType& operator=(const ArrayType& rhs);

	// Returns the number of dimensions of this array datatype.
	int getArrayNDims() const;
	//int getArrayNDims(); // removed 1.8.18 and 1.10.1

	// Returns the sizes of dimensions of this array datatype.
	int getArrayDims(hsize_t* dims) const;
	//int getArrayDims(hsize_t* dims); // removed 1.8.18 and 1.10.1

	///\brief Returns this class name.
	virtual H5std_string fromClass () const { return("ArrayType"); }

	// Copy constructor: makes copy of the original object.
	ArrayType( const ArrayType& original );

	// Constructor that takes an existing id
	ArrayType( const hid_t existing_id );

	// Noop destructor
	virtual ~ArrayType();

	// Default constructor
	ArrayType();
};
}
#endif // __H5ArrayType_H
