/* Copyright (C) 2001-2009 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied, modified
   or distributed except as expressly authorized under the terms of that
   license.  Refer to licensing information at http://www.artifex.com/
   or contact Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134,
   San Rafael, CA  94903, U.S.A., +1(415)492-9861, for further information.
*/

/*  Data type definitions when using the gscms  */

#ifndef gscms_INCLUDED
#  define gscms_INCLUDED

#include "std.h"
#include "stdpre.h"
#include "gstypes.h"
#include "gscspace.h"      /* for gs_color_space */
#include "gsdevice.h"     /* Need to carry pointer to clist reader */
#include "gxsync.h"       /* for semaphore and monitors */
#include "stdint_.h"

#define ICC_MAX_CHANNELS 15
#define NUM_DEVICE_PROFILES 4
#define NUM_SOURCE_PROFILES 3

/* Define the preferred size of the output by the CMS */
/* This can be different than the size of gx_color_value
   which can range between 8 and 16.  Here we can only
   have 8 or 16 bits */

typedef unsigned short icc_output_type;

#define icc_byte_count sizeof(icc_output_type)
#define icc_bits_count (icc_byte_count * 8)
#define icc_max_color_value ((gx_color_value)((1L << icc_bits_count) - 1))
#define icc_value_to_byte(cv)\
  ((cv) >> (icc_bits_count - 8))
#define icc_value_from_byte(cb)\
  (((cb) << (icc_bits_count - 8)) + ((cb) >> (16 - icc_bits_count)))

typedef struct gs_range_icc_s {
    gs_range_t ranges[ICC_MAX_CHANNELS];
} gs_range_icc_t;  /* ICC profile input could be up to 15 bands */

/* This object is used only for device post processing CM.  It and its objects
   must be allocated in non-gc memory. */

#ifndef cmm_profile_DEFINED
typedef struct cmm_profile_s cmm_profile_t;
#define cmm_profile_DEFINED
#endif

typedef struct gsicc_device_cm_s {
    cmm_profile_t *gray_profile;
    cmm_profile_t *rgb_profile;
    cmm_profile_t *cmyk_profile;
    cmm_profile_t *device_link_profile;
    gs_memory_t *memory;
} gsicc_device_cm_t;

/*  The buffer description.  We handle a variety of different types */
typedef enum {
    gsUNDEFINED = 0, 
    gsGRAY,
    gsRGB,
    gsCMYK,
    gsNCHANNEL,
    gsCIEXYZ,
    gsCIELAB,
    gsNAMED
} gsicc_colorbuffer_t;

typedef struct gsicc_bufferdesc_s {
    unsigned char num_chan;
    unsigned char bytes_per_chan;
    bool has_alpha;
    bool alpha_first;
    bool little_endian;
    bool is_planar;
    int plane_stride;
    int row_stride;
    int num_rows;
    int pixels_per_row;
} gsicc_bufferdesc_t;

/* Mapping procedures to allow easy vectoring depending upon if we are using
   the CMM or doing "dumb" color transforms */
typedef void (*gscms_trans_color_proc_t) (gx_device * dev, gsicc_link_t *icclink, 
                                          void *inputcolor, void *outputcolor,
                                          int num_bytes);

typedef void (*gscms_trans_buffer_proc_t) (gx_device * dev, gsicc_link_t *icclink,
                                           gsicc_bufferdesc_t *input_buff_desc,
                                           gsicc_bufferdesc_t *output_buff_desc,
                                           void *inputbuffer, void *outputbuffer);

typedef void (*gscms_link_free_proc_t) (gsicc_link_t *icclink);

typedef struct gscms_procs_s {
    gscms_trans_buffer_proc_t map_buffer;
    gscms_trans_color_proc_t  map_color;
    gscms_link_free_proc_t free_link;
} gscms_procs_t;

/* Enumerate the ICC rendering intents */
typedef enum {
    gsPERCEPTUAL = 0,
    gsRELATIVECOLORIMETRIC,
    gsSATURATION,
    gsABSOLUTECOLORIMETRIC,
    gsPERCEPTUAL_OR,            /* These are needed for keeping track  */                                   
    gsRELATIVECOLORIMETRIC_OR,  /* of when the source ri is going to */
    gsSATURATION_OR,            /* override the destination profile intent */
    gsABSOLUTECOLORIMETRIC_OR   /* in particular through the clist */
} gsicc_rendering_intents_t;

#define gsRI_OVERRIDE 0x4
#define gsRI_MASK 0x3;

/* Enumerate the types of profiles */
typedef enum {
    gsDEFAULTPROFILE = 0,
    gsGRAPHICPROFILE,
    gsIMAGEPROFILE,
    gsTEXTPROFILE,
    gsPROOFPROFILE,
    gsLINKPROFILE
} gsicc_profile_types_t;

typedef enum {
    gsSRC_GRAPPRO = 0,
    gsSRC_IMAGPRO,
    gsSRC_TEXTPRO,
} gsicc_profile_srctypes_t;

/* Source profiles for different objects.  only CMYK and RGB */
typedef struct cmm_srcgtag_profile_s {
        cmm_profile_t  *rgb_profiles[NUM_SOURCE_PROFILES];
        gsicc_rendering_intents_t rgb_intent[NUM_SOURCE_PROFILES];
        cmm_profile_t  *cmyk_profiles[NUM_SOURCE_PROFILES];
        gsicc_rendering_intents_t cmyk_intent[NUM_SOURCE_PROFILES];
        cmm_profile_t  *color_warp_profile;
        gs_memory_t *memory;
        int name_length;            /* Length of file name */
        char *name;                 /* Name of file name where this is found */
        rc_header rc;
} cmm_srcgtag_profile_t;

/* Destination profiles for different objects */
typedef struct cmm_dev_profile_s {
        cmm_profile_t  *device_profile[NUM_DEVICE_PROFILES];
        cmm_profile_t  *proof_profile;
        cmm_profile_t  *link_profile;
        gsicc_rendering_intents_t intent[NUM_DEVICE_PROFILES];
        bool devicegraytok;        /* Used for forcing gray to pure black */
        bool usefastcolor;         /* Used when we want to use no cm */
        gs_memory_t *memory;
        rc_header rc;
} cmm_dev_profile_t;

/*  Doing this an an enum type for now.  There is alot going on with respect
 *  to this and V2 versus V4 profiles
 */

typedef enum {
    BP_ON = 0,
    BP_OFF,
} gsicc_black_point_comp_t;

/*  Used so that we can specify if we want to link with Device input color spaces
    during the link creation process. For the DeviceN case, the DeviceN profile
    must match the DeviceN profile in colorant order and number of colorants.
    Also, used to indicate if the profile matches one of the default profiles in
    the icc manager.  This is useful for reducing clist size since we will encode
    this value instead of the ICC profile.
*/

typedef enum {
    DEFAULT_NONE,   /* A profile that was actually embedded in a doc */
    DEFAULT_GRAY,   /* The default DeviceGray profile */
    DEFAULT_RGB,    /* The default DeviceRGB profile */
    DEFAULT_CMYK,   /* The default DeviceCMYK profile */
    NAMED_TYPE,     /* The named color profile */
    LAB_TYPE,       /* The CIELAB profile */
    DEVICEN_TYPE,   /* A special device N profile */
    DEFAULT_GRAY_s, /* Same as default but a source profile from document */
    DEFAULT_RGB_s,  /* Same as default but a source profile from document */
    DEFAULT_CMYK_s, /* Same as default but a source profile from document */
    LAB_TYPE_s,     /* Same as our default CIELAB but a source profile from doc */
    CAL_GRAY,       /* Generated from PDF cal gray object */
    CAL_RGB,        /* Generated from PDF cal rgb object */
    CIE_A,          /* Generated from PS CIEA definition */
    CIE_ABC,        /* Generated from PS CIEABC definition */
    CIE_DEF,        /* Generated from PS CIEDEF definition */
    CIE_DEFG,       /* Generated from PS CIEDEFG definition */
    CIE_CRD        /* Generated from PS CRD definition */
} gsicc_profile_t;

#define gsicc_serial_data\
    unsigned char num_comps;		/* number of device dependent values */\
    unsigned char num_comps_out;	/* usually 3 but could be more if device link type */\
    bool islab;				/* Needed since we want to detect this to avoid */\
                                        /*  expensive decode on LAB images.  Is true */\
                                        /* if PDF color space is \Lab */\
    gsicc_profile_t default_match;	/* Used for detecting a match to a default space */\
    gsicc_colorbuffer_t data_cs;	/* The data color space of the profile (not the PCS) */\
    gs_range_icc_t Range;\
    int64_t hashcode;			/* A hash code for the icc profile */\
    bool hash_is_valid;			/* Is the code valid? */\
    int devicen_permute[ICC_MAX_CHANNELS];	/* Permutation vector for deviceN laydown order */\
    bool devicen_permute_needed;		/* Check if we need to permute the DeviceN values */\
    int buffer_size			/* size of ICC profile buffer */

/* A subset of the profile information which is used when writing and reading
 * out to the c-list
 */
typedef struct gsicc_serialized_profile_s {
    gsicc_serial_data;
} gsicc_serialized_profile_t;

typedef struct gsicc_colorname_s gsicc_colorname_t;

struct gsicc_colorname_s {
    char *name;
    int length;
    gsicc_colorname_t *next;
};

typedef struct gsicc_namelist_s gsicc_namelist_t;

struct gsicc_namelist_s {
    int count;
    gsicc_colorname_t *head;
};

/* A structure for holding profile information.  A member variable
 * of the ghostscript color structure.   The item is reference counted.
 */
struct cmm_profile_s {
    gsicc_serial_data;
    byte *buffer;               /* A buffer with ICC profile content */
    gx_device *dev;             /* A pointer to the clist device in which the ICC data may be contained */
    gsicc_namelist_t *spotnames;  /* Only used with NCLR ICC input profiles with named color tag */
    void *profile_handle;       /* The profile handle */
    rc_header rc;               /* Reference count.  So we know when to free */
    int name_length;            /* Length of file name */
    char *name;                 /* Name of file name (if there is one) where profile is found.
                                 * If it was embedded in the stream, there will not be a file
                                 * name.  This is primarily here for the system profiles, and
                                 * so that we avoid resetting them everytime the user params
                                 * are reloaded. */
    gs_memory_t *memory;        /* In case we have some in non-gc and some in gc memory */
    gx_monitor_t *lock;		/* handle for the monitor */
};

#ifndef cmm_profile_DEFINED
typedef struct cmm_profile_s cmm_profile_t;
#define cmm_profile_DEFINED
#endif

/* A linked list structure for storing profiles in a table in which we
   can store and refer to from the clist and also when creating icc profiles
   from ps object.  Right now it is not clear to me if we really need a
   cache in the traditional sense or a list since I believe the number of entries will
   in general be very small (i.e. there will not be at MOST more than 2 to 3 internal
   ICC profiles in a file).  The default GRAY, RGB, and CMYK profiles are not
   stored here but are maintained in the ICC manager.  This is for profiles
   that are in the content and for profiles we generate from PS and PDF CIE (NonICC)
   color spaces.
 */
typedef struct gsicc_profile_entry_s gsicc_profile_entry_t;

struct gsicc_profile_entry_s {
    gs_color_space *color_space;     /* The color space with the profile */
    gsicc_profile_entry_t *next;    /* next CS */
    int64_t key;                    /* Key based off dictionary location */
};

/* ProfileList. The size of the list is limited by max_memory_size.
   Profiles are added if there is sufficient memory. */
typedef struct gsicc_profile_cache_s {
    gsicc_profile_entry_t *head;
    int num_entries;
    rc_header rc;
    gs_memory_t *memory;
} gsicc_profile_cache_t;

/*  These are the types that we can potentially have linked together by the CMS.
 *  If the CMS does not have understanding of PS color space types, then we
 *  will need to convert them to an ICC type. */
typedef enum {
    DEVICETYPE,
    ICCTYPE,
    CRDTYPE,
    CIEATYPE,
    CIEABCTYPE,
    CIEDEFTYPE,
    CIEDEFGTYPE
} gs_colortype_t;

/* The link object. */

#ifndef gsicc_link_DEFINED
typedef struct gsicc_link_s gsicc_link_t;
#  define gsicc_link_DEFINED
#endif

typedef struct gsicc_hashlink_s {
    int64_t link_hashcode;
    int64_t src_hash;
    int64_t des_hash;
    int64_t rend_hash;
} gsicc_hashlink_t;

struct gsicc_link_s {
    void *link_handle;
    void *contextptr;
    gscms_procs_t procs;   
    gsicc_hashlink_t hashcode;
    struct gsicc_link_cache_s *icc_link_cache;
    int ref_count;
    gsicc_link_t *next;
    gx_semaphore_t *wait;		/* semaphore used by waiting threads */
    int num_waiting;
    bool includes_softproof;
    bool includes_devlink;
    bool is_identity;  /* Used for noting that this is an identity profile */
    bool valid;		/* true once link is completely built and usable */
};

/* ICC Cache. The size of the cache is limited by max_memory_size.
 * Links are added if there is sufficient memory and if the number
 * of links does not exceed a (soft) limit.
 */

typedef struct gsicc_link_cache_s {
    gsicc_link_t *head;
    int num_links;
    rc_header rc;
    gs_memory_t *memory;
    gx_monitor_t *lock;		/* handle for the monitor */
    gx_semaphore_t *wait;	/* somebody needs a link cache slot */
    int num_waiting;		/* number of threads waiting */
} gsicc_link_cache_t;

/* A linked list structure to keep DeviceN ICC profiles
 * that the user wishes to use to achieve accurate rendering
 * with DeviceN (typically non CMYK or CMYK + spot) colors.
 * The ICC profiles used for this will require a special
 * private tag in the ICC profile that defines the colorant
 * names and they must match those in the DeviceN color
 * space.  Note this is not to say that DeviceN color
 * management can only be achieved with ICC profiles.  If
 * a customer has a proprietary mixing model for inks, they
 * will be able to hook in their method in the location
 * in the code where the DeviceN colors are processed.  If
 * there is no ICC color management of the DeviceN colors
 * and the DeviceN colors are NOT the native colors
 * for the device, then the colors will be transformed to
 * the alternate CS using the alternate tint transform
 */

typedef struct gsicc_devicen_entry_s gsicc_devicen_entry_t;

struct gsicc_devicen_entry_s {
    cmm_profile_t *iccprofile;
    gsicc_devicen_entry_t *next;
};

typedef struct gsicc_devicen_s gsicc_devicen_t;

struct gsicc_devicen_s {
    gsicc_devicen_entry_t *head;
    gsicc_devicen_entry_t *final;
    int count;
};

typedef struct gsicc_smask_s {
    cmm_profile_t *smask_gray;
    cmm_profile_t *smask_rgb;
    cmm_profile_t *smask_cmyk;
    gs_memory_t *memory;
} gsicc_smask_t;

/* The manager object */

typedef struct gsicc_manager_s {
    cmm_profile_t *device_named;    /* The named color profile for the device */
    cmm_profile_t *default_gray;    /* Default gray profile for device gray */
    cmm_profile_t *default_rgb;     /* Default RGB profile for device RGB */
    cmm_profile_t *default_cmyk;    /* Default CMYK profile for device CMKY */
    cmm_profile_t *lab_profile;     /* Colorspace type ICC profile from LAB to LAB */
    cmm_profile_t *graytok_profile; /* A specialized profile for mapping gray to K */
    gsicc_devicen_t *device_n;      /* A linked list of profiles used for DeviceN support */
    gsicc_smask_t *smask_profiles;  /* Profiles used when we are in a softmask group */
    bool override_internal;         /* Set via the user params */
    bool override_ri;               /* Override rend intent. Set via the user params */
    cmm_srcgtag_profile_t *srcgtag_profile;
    gs_memory_t *memory;
    rc_header rc;
} gsicc_manager_t;

/* --------------- graphical object tags ------------ */

/* The default is "unknown" which has value 0 and by default devices don't encode tags */
typedef enum {
    GS_UNKNOWN_TAG = 0x0,
    GS_TEXT_TAG = 0x1,
    GS_IMAGE_TAG = 0x2,
    GS_PATH_TAG = 0x4,
    GS_UNTOUCHED_TAG = 0x8,
    GS_DEVICE_ENCODES_TAGS = 0x80
} gs_graphics_type_tag_t;

typedef struct gsicc_rendering_param_s {
    gsicc_rendering_intents_t rendering_intent;
    gs_graphics_type_tag_t    graphics_type_tag;
    gsicc_black_point_comp_t  black_point_comp;
} gsicc_rendering_param_t;

#endif /* ifndef gscms_INCLUDED */
