/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
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

/*
 * Programmer:  Quincey Koziol <koziol@hdfgroup.org>
 *              March, 2013
 *
 * Purpose:	Wrappers around existing HDF5 to support Exascale FastForward
 *              functionality.
 *              
 */


/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5FFprivate.h"        /* FastForward wrappers                 */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Pprivate.h"		/* Property lists			*/
#include "H5VLprivate.h"	/* VOL plugins				*/
#include "H5VLiod.h"		/* IOD plugin - tmp      		*/
#include "H5VLiod_client.h"	/* Client IOD - tmp			*/

#ifdef H5_HAVE_EFF
/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5Fcreate_ff
 *
 * Purpose:	Asynchronous wrapper around H5Fcreate().
 *
 * Return:	Success:	The placeholder ID for a new file.  When
 *                              the asynchronous operation completes, this
 *                              ID will transparently be modified to be a
 *                              "normal" ID.
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fcreate_ff(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t eq_id)
{
    void    *file = NULL;            /* file token from VOL plugin */
    H5VL_t  *vol_plugin;             /* VOL plugin information */
    hid_t    ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    H5F_init();

    /* Check/fix arguments */
    if(!filename || !*filename)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file name")
    /* In this routine, we only accept the following flags:
     *          H5F_ACC_EXCL, H5F_ACC_TRUNC and H5F_ACC_DEBUG
     */
    if(flags & ~(H5F_ACC_EXCL | H5F_ACC_TRUNC | H5F_ACC_DEBUG))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid flags")
    /* The H5F_ACC_EXCL and H5F_ACC_TRUNC flags are mutually exclusive */
    if((flags & H5F_ACC_EXCL) && (flags & H5F_ACC_TRUNC))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "mutually exclusive flags for file creation")

    /* Check file creation property list */
    if(H5P_DEFAULT == fcpl_id)
        fcpl_id = H5P_FILE_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(fcpl_id, H5P_FILE_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not file create property list")

    /* Check the file access property list */
    if(H5P_DEFAULT == fapl_id)
        fapl_id = H5P_FILE_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(fapl_id, H5P_FILE_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not file access property list")

    /* create a new file or truncate an existing file through the VOL */
    if(NULL == (file = H5VL_file_create(&vol_plugin, filename, flags, fcpl_id, fapl_id, 
                                        H5AC_dxpl_id, eq_id)))
	HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to create file")

    /* Get an atom for the file with the VOL information as the auxilary struct*/
    if((ret_value = H5I_register2(H5I_FILE, file, vol_plugin, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize file handle")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fcreate_ff() */


/*-------------------------------------------------------------------------
 * Function:	H5Fopen_ff
 *
 * Purpose:	Asynchronous wrapper around H5Fopen().
 *
 * Return:	Success:	The placeholder ID for a new file.  When
 *                              the asynchronous operation completes, this
 *                              ID will transparently be modified to be a
 *                              "normal" ID.
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Fopen_ff(const char *filename, unsigned flags, hid_t fapl_id, hid_t eq_id)
{
    void    *file = NULL;            /* file token from VOL plugin */
    H5VL_t  *vol_plugin;             /* VOL plugin information */
    hid_t    ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check/fix arguments. */
    if(!filename || !*filename)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file name")
    /* Reject undefined flags (~H5F_ACC_PUBLIC_FLAGS) and the H5F_ACC_TRUNC & H5F_ACC_EXCL flags */
    if((flags & ~H5F_ACC_PUBLIC_FLAGS) ||
            (flags & H5F_ACC_TRUNC) || (flags & H5F_ACC_EXCL))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file open flags")
    if(H5P_DEFAULT == fapl_id)
        fapl_id = H5P_FILE_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(fapl_id, H5P_FILE_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not file access property list")

    /* Open the file through the VOL layer */
    if(NULL == (file = H5VL_file_open(&vol_plugin, filename, flags, fapl_id, 
                                      H5AC_dxpl_id, eq_id)))
	HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to create file")

    /* Get an atom for the file with the VOL information as the auxilary struct*/
    if((ret_value = H5I_register2(H5I_FILE, file, vol_plugin, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize file handle")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Fopen_ff() */


/*-------------------------------------------------------------------------
 * Function:	H5Gcreate_ff
 *
 * Purpose:	Asynchronous wrapper around H5Gcreate().
 *
 * Return:	Success:	The placeholder ID for a group.  When
 *                              the asynchronous operation completes, this
 *                              ID will transparently be modified to be a
 *                              "normal" ID.
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gcreate_ff(hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id,
             uint64_t trans, hid_t eq_id)
{
    void    *grp = NULL;       /* dset token from VOL plugin */
    void    *obj = NULL;        /* object token of loc_id */
    H5VL_t  *vol_plugin;        /* VOL plugin information */
    H5VL_loc_params_t loc_params;
    H5P_genplist_t  *plist;            /* Property list pointer */
    hid_t       ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check arguments */
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Get correct property list */
    if(H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link creation property list")

    /* Check group creation property list */
    if(H5P_DEFAULT == gcpl_id)
        gcpl_id = H5P_GROUP_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gcpl_id, H5P_GROUP_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group create property list")

    /* Check the group access property list */
    if(H5P_DEFAULT == gapl_id)
        gapl_id = H5P_GROUP_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gapl_id, H5P_GROUP_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group access property list")

    /* Get the plist structure */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(gcpl_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    if(H5P_set(plist, H5VL_GRP_LCPL_ID, &lcpl_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set property value for lcpl id")

    loc_params.type = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* get the file object */
    if(NULL == (obj = (void *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")
    /* get the plugin pointer */
    if (NULL == (vol_plugin = (H5VL_t *)H5I_get_aux(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "ID does not contain VOL information")

    /* Create the group through the VOL */
    if(NULL == (grp = H5VL_group_create(obj, loc_params, vol_plugin, name, gcpl_id, gapl_id, 
                                        H5AC_dxpl_id, eq_id)))
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to create group")

    /* Get an atom for the group */
    if((ret_value = H5I_register2(H5I_GROUP, grp, vol_plugin, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize dataset handle")

done:
    if (ret_value < 0 && grp)
        if(H5VL_group_close (grp, vol_plugin, H5AC_dxpl_id, eq_id) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "unable to release group")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gcreate_ff() */


/*-------------------------------------------------------------------------
 * Function:	H5Gopen_ff
 *
 * Purpose:	Asynchronous wrapper around H5Gopen().
 *
 * Return:	Success:	The placeholder ID for a group.  When
 *                              the asynchronous operation completes, this
 *                              ID will transparently be modified to be a
 *                              "normal" ID.
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gopen_ff(hid_t loc_id, const char *name, hid_t gapl_id,
           uint64_t trans, hid_t eq_id)
{
    void    *grp = NULL;       /* dset token from VOL plugin */
    void    *obj = NULL;        /* object token of loc_id */
    H5VL_t  *vol_plugin;        /* VOL plugin information */
    H5VL_loc_params_t loc_params;
    hid_t       ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check args */
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Check the group access property list */
    if(H5P_DEFAULT == gapl_id)
        gapl_id = H5P_GROUP_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gapl_id, H5P_GROUP_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group access property list")

    loc_params.type = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* get the file object */
    if(NULL == (obj = (void *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* get the plugin pointer */
    if (NULL == (vol_plugin = (H5VL_t *)H5I_get_aux(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "ID does not contain VOL information")

    /* Create the group through the VOL */
    if(NULL == (grp = H5VL_group_open(obj, loc_params, vol_plugin, name, gapl_id, 
                                      H5AC_dxpl_id, eq_id)))
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to create group")

    /* Get an atom for the group */
    if((ret_value = H5I_register2(H5I_GROUP, grp, vol_plugin, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize dataset handle")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gopen_ff() */


/*-------------------------------------------------------------------------
 * Function:	H5Dcreate_ff
 *
 * Purpose:	Asynchronous wrapper around H5Dcreate().
 *
 * Return:	Success:	The placeholder ID for a new dataset.  When
 *                              the asynchronous operation completes, this
 *                              ID will transparently be modified to be a
 *                              "normal" ID.
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dcreate_ff(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, 
             hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id, uint64_t trans, hid_t eq_id)
{
    void    *dset = NULL;       /* dset token from VOL plugin */
    void    *obj = NULL;        /* object token of loc_id */
    H5VL_t  *vol_plugin;        /* VOL plugin information */
    H5VL_loc_params_t loc_params;
    H5P_genplist_t  *plist;     /* Property list pointer */
    hid_t       ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Get correct property list */
    if(H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link creation property list")

    /* Get correct property list */
    if(H5P_DEFAULT == dcpl_id)
        dcpl_id = H5P_DATASET_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dcpl_id, H5P_DATASET_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset create property list ID")

    /* Get correct property list */
    if(H5P_DEFAULT == dapl_id)
        dapl_id = H5P_DATASET_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dapl_id, H5P_DATASET_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset access property list")

    /* Get the plist structure */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(dcpl_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* set creation properties */
    if(H5P_set(plist, H5VL_DSET_TYPE_ID, &type_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set property value for datatype id")
    if(H5P_set(plist, H5VL_DSET_SPACE_ID, &space_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set property value for space id")
    if(H5P_set(plist, H5VL_DSET_LCPL_ID, &lcpl_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set property value for lcpl id")

    loc_params.type = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* get the file object */
    if(NULL == (obj = (void *)H5VL_get_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")
    /* get the plugin pointer */
    if (NULL == (vol_plugin = (H5VL_t *)H5I_get_aux(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "ID does not contain VOL information")

    /* Create the dataset through the VOL */
    if(NULL == (dset = H5VL_dataset_create(obj, loc_params, vol_plugin, name, dcpl_id, dapl_id, 
                                           H5AC_dxpl_id, eq_id)))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset")

    /* Get an atom for the dataset */
    if((ret_value = H5I_register2(H5I_DATASET, dset, vol_plugin, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize dataset handle")

done:
    if (ret_value < 0 && dset)
        if(H5VL_dataset_close(dset, vol_plugin, H5AC_dxpl_id, eq_id) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")

    FUNC_LEAVE_API(ret_value)
} /* end H5Dcreate_ff() */


/*-------------------------------------------------------------------------
 * Function:	H5Dopen_ff
 *
 * Purpose:	Asynchronous wrapper around H5Dopen().
 *
 * Return:	Success:	The placeholder ID for a dataset.  When
 *                              the asynchronous operation completes, this
 *                              ID will transparently be modified to be a
 *                              "normal" ID.
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dopen_ff(hid_t loc_id, const char *name, hid_t dapl_id, uint64_t trans, hid_t eq_id)
{
    void    *dset = NULL;       /* dset token from VOL plugin */
    void    *obj = NULL;        /* object token of loc_id */
    H5VL_t  *vol_plugin;        /* VOL plugin information */
    H5VL_loc_params_t loc_params;
    hid_t       ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check args */
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Get correct property list */
    if(H5P_DEFAULT == dapl_id)
        dapl_id = H5P_DATASET_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dapl_id, H5P_DATASET_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not dataset access property list")

    loc_params.type = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* get the file object */
    if(NULL == (obj = (void *)H5VL_get_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid file identifier")

    /* get the plugin pointer */
    if (NULL == (vol_plugin = (H5VL_t *)H5I_get_aux(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "ID does not contain VOL information")

    /* Create the dataset through the VOL */
    if(NULL == (dset = H5VL_dataset_open(obj, loc_params, vol_plugin, name, dapl_id, 
                                         H5AC_dxpl_id, eq_id)))
	HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, FAIL, "unable to create dataset")

    /* Get an atom for the dataset */
    if((ret_value = H5I_register2(H5I_DATASET, dset, vol_plugin, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to atomize dataset handle")

done:
    if (ret_value < 0 && dset)
        if(H5VL_dataset_close (dset, vol_plugin, H5AC_dxpl_id, eq_id) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataset")

    FUNC_LEAVE_API(ret_value)
} /* end H5Dopen_ff() */


/*-------------------------------------------------------------------------
 * Function:	H5Dwrite_ff
 *
 * Purpose:	Asynchronous wrapper around H5Dwrite().
 *
 * Return:	Success:	SUCCEED
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dwrite_ff(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
            hid_t file_space_id, hid_t dxpl_id, const void *buf,
            uint64_t trans, hid_t eq_id)
{
    H5VL_t     *vol_plugin;
    void       *dset;
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    /* check arguments */
    if(!dset_id)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if(H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* get the plugin pointer */
    if (NULL == (vol_plugin = (H5VL_t *)H5I_get_aux(dset_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "ID does not contain VOL information")
    /* get the dataset object */
    if(NULL == (dset = (void *)H5I_object(dset_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset identifier")

    /* Write the data through the VOL */
    if((ret_value = H5VL_dataset_write(dset, vol_plugin, mem_type_id, mem_space_id, 
                                       file_space_id, dxpl_id, buf, eq_id)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_WRITEERROR, FAIL, "can't write data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dwrite_ff() */


/*-------------------------------------------------------------------------
 * Function:	H5Dread_ff
 *
 * Purpose:	Asynchronous wrapper around H5Dread().
 *
 * Return:	Success:	SUCCEED
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dread_ff(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
           hid_t file_space_id, hid_t dxpl_id, void *buf/*out*/,
           uint64_t trans, hid_t eq_id)
{
    H5VL_t     *vol_plugin;
    void       *dset;
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)

    if(mem_space_id < 0 || file_space_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id= H5P_DATASET_XFER_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* get the plugin pointer */
    if (NULL == (vol_plugin = (H5VL_t *)H5I_get_aux(dset_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "ID does not contain VOL information")
    /* get the dataset object */
    if(NULL == (dset = (void *)H5I_object(dset_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset identifier")

    /* Read the data through the VOL */
    if((ret_value = H5VL_dataset_read(dset, vol_plugin, mem_type_id, mem_space_id, 
                                      file_space_id, dxpl_id, buf, eq_id)) < 0)
	HGOTO_ERROR(H5E_DATASET, H5E_READERROR, FAIL, "can't read data")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dread_ff() */

#if 0

/*-------------------------------------------------------------------------
 * Function:	H5AOtest
 *
 * Purpose:	Test for an asynchronous operation's completion
 *
 * Return:	Success:	SUCCEED
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AOtest(H5_request_t *req, H5_status_t *status)
{
    H5VL_iod_request_t *request = *((H5VL_iod_request_t **)req);
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)

    *status = request->status;

    if(H5VL_IOD_COMPLETED == request->state) {
        if(H5VL_iod_request_wait(request->obj->file, request) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTFREE, FAIL, "unable to wait for request")
                request->req = H5MM_xfree(request->req);
        request = H5MM_xfree(request);
        req = NULL;
    }

/* I believe that the VOL interface needs to be expanded with a 'test' callback,
    since the H5_request_t is pointing at a H5VL_iod_request_t [currently]
    and I can't get down to the VOL plugin from here.

    [And the 'test' client callback needs to release the request structure that
        the plugin allocated, if the operation has completed]
*/

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5AOtest() */


/*-------------------------------------------------------------------------
 * Function:	H5AOwait
 *
 * Purpose:	Wait for an asynchronous operation to complete
 *
 * Return:	Success:	SUCCEED
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AOwait(H5_request_t *req, H5_status_t *status)
{
    H5VL_iod_request_t *request = *((H5VL_iod_request_t **)req);
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)

    if(H5VL_IOD_PENDING == request->state) {
        if(H5VL_iod_request_wait(request->obj->file, request) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTFREE, FAIL, "unable to wait for request")
    }
    *status = request->status;

    request->req = H5MM_xfree(request->req);
    request = H5MM_xfree(request);
    req = NULL;

/* I believe that the VOL interface needs to be expanded with a 'wait' callback,
    since the H5_request_t is pointing at a H5VL_iod_request_t [currently]
    and I can't get down to the VOL plugin from here.

    [And the 'wait' client callback needs to release the request structure that
        the plugin allocated]
*/

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5AOwait() */



/*-------------------------------------------------------------------------
 * Function:	H5FF_set_async_flag
 *
 * Purpose:	Helper routine to set up asynchronous I/O properties
 *
 * Return:	Success:	0
 *		Failure:	<0
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FF_set_async_flag(H5P_genplist_t *plist, const H5_request_t *req)
{
    hbool_t     do_async;               /* Whether we're going to do async. I/O */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(plist);
    HDassert(req);

    /* Check if we are performing asynchronous I/O */
    do_async = req ? TRUE : FALSE;

    /* Set the async. I/O operation flag */
    if(H5P_set(plist, H5P_ASYNC_FLAG_NAME, &do_async) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set async flag")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FF_set_async_flag() */


/*-------------------------------------------------------------------------
 * Function:	H5FF_get_async_req
 *
 * Purpose:	Helper routine to get asynchronous I/O request
 *
 * Return:	Success:	0
 *		Failure:	<0
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FF_get_async_req(H5P_genplist_t *plist, H5_request_t *req)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(plist);
    HDassert(req);

    /* Retrieve the async. I/O operation request */
    if(H5P_get(plist, H5P_ASYNC_REQ_NAME, req) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't get async request")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FF_get_async_req() */


/*-------------------------------------------------------------------------
 * Function:	H5FF_reset_async_flag
 *
 * Purpose:	Helper routine to reset asynchronous I/O properties
 *
 * Return:	Success:	0
 *		Failure:	<0
 *
 * Programmer:	Quincey Koziol
 *		Wednesday, March 20, 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FF_reset_async_flag(H5P_genplist_t *plist)
{
    hbool_t     do_async = H5P_ASYNC_FLAG_DEF;  /* Default async I/O flag */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Reset the async. I/O operation flag */
    if(H5P_set(plist, H5P_ASYNC_FLAG_NAME, &do_async) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set async flag")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FF_reset_async_flag() */

#endif

#endif /* H5_HAVE_EFF */
