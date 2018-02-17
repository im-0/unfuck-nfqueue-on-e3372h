#include <linux/kallsyms.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/notifier.h>
#include <linux/mutex.h>

#include <net/sock.h>
#include <net/netfilter/nf_queue.h>

#define LPREFIX "unfuck_nfqueue: "
#define N_AF 2

typedef int (*ipq_cmpfn)(struct nf_queue_entry *, unsigned long);

static const char *const proc_fs_names[N_AF] = {"ip_queue", "ip6_queue"};

struct unfuck_pointers {
    struct nf_queue_handler *nfqh[N_AF];
    void (*ipq_flush[N_AF])(ipq_cmpfn, unsigned long);
    struct ctl_table_header **ipq_sysctl_header[N_AF];
    struct notifier_block *ipq_dev_notifier[N_AF];
    struct sock **ipqnl[N_AF];
    struct mutex *ipqnl_mutex[N_AF];
    struct notifier_block *ipq_nl_notifier[N_AF];
};

struct find_sym_data {
    const char *name;
    int (*fn)(void **, void *);
    void **vec;
};

static int find_sym_cb(void *data, const char *name, struct module *module, unsigned long address)
{
    const struct find_sym_data *find_sym_data = data;
    int rc = 0;

    if (strcmp(name, find_sym_data->name) == 0) {
        rc = find_sym_data->fn(find_sym_data->vec, (void *)address);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

static int find_sym(const char *name, int (*fn)(void **, void *), void **vec)
{
    struct find_sym_data find_sym_data = {
        .name = name,
        .fn = fn,
        .vec = vec,
    };
    int rc = 0;

    printk(KERN_INFO LPREFIX "searching for \"%s\"...\n", name);
    rc = kallsyms_on_each_symbol(find_sym_cb, &find_sym_data);
    if (rc != 0) {
        return rc;
    }

    if (vec[N_AF - 1] == (void *)0) {
        printk(KERN_ERR LPREFIX "not enough symbols for \"%s\", exiting!\n", name);
        return -1;
    }
    printk(KERN_INFO LPREFIX "found %d symbols for \"%s\"\n", N_AF, name);

    return 0;
}

static int add_pointer(void **vec, void *address)
{
    for (int i = 0; i < N_AF; ++i) {
        if (vec[i] == (void *)0) {
            vec[i] = address;
            return 0;
        }
    }

    printk(KERN_ERR LPREFIX "too many symbols, exiting!\n");
    return -1;
}

static int find_nfqh_cb(void **vec, void *address)
{
    struct nf_queue_handler *nfqh = address;
    int rc;

    if (nfqh->name[0] == 'i') {
        printk(KERN_INFO LPREFIX "found nf_queue_handler with name \"%s\" at address 0x%p\n", nfqh->name, nfqh);
        rc = add_pointer(vec, address);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

struct sym_list {
    const char *name;
    int (*fn)(void **, void *);
    void *vec_ptr;
};

static int find_syms(struct sym_list *sym_list)
{
    int rc;
    while (sym_list->name != (void *)0) {
        rc = find_sym(sym_list->name, sym_list->fn, sym_list->vec_ptr);
        if (rc != 0) {
            return rc;
        }
        ++sym_list;
    }

    return 0;
}

static bool already_fixed(struct unfuck_pointers *unfuck_pointers)
{
    for (int i = 0; i < N_AF; ++i) {
        if (*unfuck_pointers->ipq_sysctl_header[i] != NULL) {
            return false;
        }
    }
    return true;
}

static void mark_already_fixed(struct unfuck_pointers *unfuck_pointers)
{
    for (int i = 0; i < N_AF; ++i) {
        *unfuck_pointers->ipq_sysctl_header[i] = NULL;
    }
}

#define FOR_EACH(call) for (int i = 0; i < N_AF; ++i) { \
        call; \
    }

static int __init unfuck_nfqueue_start(void)
{
    struct unfuck_pointers unfuck_pointers = {0};
    int rc = 0;

    struct sym_list sym_list[] = {
        {
            .name = "nfqh",
            .fn = find_nfqh_cb,
            .vec_ptr = unfuck_pointers.nfqh,
        }, {
            .name = "ipq_flush",
            .fn = add_pointer,
            .vec_ptr = unfuck_pointers.ipq_flush,
        }, {
            .name = "ipq_sysctl_header",
            .fn = add_pointer,
            .vec_ptr = unfuck_pointers.ipq_sysctl_header,
        }, {
            .name = "ipq_dev_notifier",
            .fn = add_pointer,
            .vec_ptr = unfuck_pointers.ipq_dev_notifier,
        }, {
            .name = "ipqnl",
            .fn = add_pointer,
            .vec_ptr = unfuck_pointers.ipqnl,
        }, {
            .name = "ipqnl_mutex",
            .fn = add_pointer,
            .vec_ptr = unfuck_pointers.ipqnl_mutex,
        }, {
            .name = "ipq_nl_notifier",
            .fn = add_pointer,
            .vec_ptr = unfuck_pointers.ipq_nl_notifier,
        }, {0},
    };

    printk(KERN_INFO LPREFIX "trying to find required symbols...\n");

    rc = find_syms(sym_list);
    if (rc != 0) {
        return rc;
    }

    if (already_fixed(&unfuck_pointers)) {
        printk(KERN_INFO LPREFIX "patching already done, exiting\n");
        return 0;
    }

    printk(KERN_INFO LPREFIX "patching...\n");

    /* Normally, ip_queue_fini() and ip6_queue_fini() are doing this on module unload. */
    FOR_EACH(nf_unregister_queue_handlers(unfuck_pointers.nfqh[i]))
    FOR_EACH(unfuck_pointers.ipq_flush[i](NULL, 0))
    FOR_EACH(unregister_sysctl_table(*unfuck_pointers.ipq_sysctl_header[i]))
    FOR_EACH(unregister_netdevice_notifier(unfuck_pointers.ipq_dev_notifier[i]))
    FOR_EACH(proc_net_remove(&init_net, proc_fs_names[i]))
    FOR_EACH(netlink_kernel_release(*unfuck_pointers.ipqnl[i]))
    FOR_EACH(mutex_lock(unfuck_pointers.ipqnl_mutex[i]))
    FOR_EACH(mutex_unlock(unfuck_pointers.ipqnl_mutex[i]))
    FOR_EACH(netlink_unregister_notifier(unfuck_pointers.ipq_nl_notifier[i]))

    mark_already_fixed(&unfuck_pointers);

    printk(KERN_INFO LPREFIX "patching done successfully\n");

    return 0;
}

static void __exit unfuck_nfqueue_end(void)
{
    printk(KERN_INFO LPREFIX "unloaded\n");
}

MODULE_DESCRIPTION("Fix NFQUEUE in e3372h kernel");
MODULE_AUTHOR("Ivan Mironov <mironov.ivan@gmail.com>");
MODULE_LICENSE("GPL");

module_init(unfuck_nfqueue_start);
module_exit(unfuck_nfqueue_end);
