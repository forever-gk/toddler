#ifndef __ARCH_PPC32_LOADER_INCLUDE_OFW__
#define __ARCH_PPC32_LOADER_INCLUDE_OFW__


/*
 * Arg list
 */
typedef __builtin_va_list va_list;
#define va_start(ap, last)  __builtin_va_start(ap, last)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
#define va_end(ap)          __builtin_va_end(ap)


/*
 * OFW data types
 */
#define MEMMAP_MAX_RECORDS              32
#define MAX_OFW_ARGS                    12
#define OFW_TREE_PATH_MAX_LEN           256
#define OFW_TREE_PROPERTY_MAX_NAMELEN   32
#define OFW_TREE_PROPERTY_MAX_VALUELEN  64

typedef unsigned long ofw_arg_t;
typedef unsigned long ofw_prop_t;
typedef unsigned long ofw_ihandle_t;
typedef unsigned long ofw_phandle_t;

struct ofw_args {
    ofw_arg_t service;              // Command name
    ofw_arg_t nargs;                // Number of in arguments
    ofw_arg_t nret;                 // Number of out arguments
    ofw_arg_t args[MAX_OFW_ARGS];   // List of arguments
};

typedef ofw_arg_t (*ofw_entry_t)(struct ofw_args *args);


/*
 * Memory representation of OpenFirmware device tree node and property
 */
struct ofw_tree_prop {
    char name[OFW_TREE_PROPERTY_MAX_NAMELEN];
    int size;
    void *value;
};

struct ofw_tree_node {
    struct ofw_tree_node *parent;
    struct ofw_tree_node *peer;
    struct ofw_tree_node *child;
    
    ofw_phandle_t handle;
    char *name;
    
    int num_props;
    struct ofw_tree_prop *prop;
};


/*
 * OFW services
 */
extern void ofw_init(ulong ofw_entry);
extern void ofw_printf(char *fmt, ...);

extern int ofw_mem_zone(int idx, ulong *start, ulong *size);

extern int ofw_screen_is_graphic();
extern void ofw_fb_info(void **addr, int *width, int *height, int *depth, int *bpl);
extern void ofw_escc_info(void **addr);

extern void *ofw_translate(void *virt);

extern void ofw_quiesce();

extern void ofw_alloc(void **virt, void **phys, const int size, int align);

extern struct ofw_tree_node *ofw_tree_build();


#endif
