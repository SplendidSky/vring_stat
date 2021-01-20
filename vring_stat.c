#include <linux/virtio.h>
#include <linux/virtio_ring.h>
#include <linux/virtio_config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>

struct vring_desc_state {
    void *data;            /* Data for callback. */
    struct vring_desc *indir_desc;    /* Indirect descriptor, if any. */
};

struct vring_virtqueue {
    struct virtqueue vq;

    /* Actual memory layout for this queue */
    struct vring vring;

    /* Can we use weak barriers? */
    bool weak_barriers;

    /* Other side has made a mess, don't try any more. */
    bool broken;

    /* Host supports indirect buffers */
    bool indirect;

    /* Host publishes avail event idx */
    bool event;

    /* Head of free buffer list. */
    unsigned int free_head;
    /* Number we've added since last sync. */
    unsigned int num_added;

    /* Last used index we've seen. */
    u16 last_used_idx;

    /* How to notify other side. FIXME: commonalize hcalls! */
    bool (*notify)(struct virtqueue *vq);

    /* DMA, allocation, and size information */
    bool we_own_ring;
    size_t queue_size_in_bytes;
    dma_addr_t queue_dma_addr;

#ifdef DEBUG
    /* They're supposed to lock for us. */
    unsigned int in_use;

    /* Figure out if their kicks are too delayed. */
    bool last_add_time_valid;
    ktime_t last_add_time;
#endif

    /* Per-descriptor state. */
    struct vring_desc_state desc_state[];
};

struct virtnet_info {
    struct virtio_device *vdev;
    struct virtqueue *cvq;
    struct net_device *dev;
    struct send_queue *sq;
    struct receive_queue *rq;
    unsigned int status;

    /* Max # of queue pairs supported by the device */
    u16 max_queue_pairs;

    /* # of queue pairs currently used by the driver */
    u16 curr_queue_pairs;

    /* I like... big packets and I cannot lie! */
    bool big_packets;

    /* Host will merge rx buffers for big packets (shake it! shake it!) */
    bool mergeable_rx_bufs;

    /* Has control virtqueue */
    bool has_cvq;

    /* Host can handle any s/g split between our header and packet data */
    bool any_header_sg;

    /* Packet virtio header size */
    u8 hdr_len;

    /* Active statistics */
    struct virtnet_stats __percpu *stats;

    /* Work struct for refilling if we run low on memory. */
    struct delayed_work refill;

    /* Work struct for config space updates */
    struct work_struct config_work;

    /* Does the affinity hint is set for virtqueues? */
    bool affinity_hint_set;

    /* Per-cpu variable to show the mapping from CPU to virtqueue */
    int __percpu *vq_index;

    /* CPU hot plug notifier */
    struct notifier_block nb;

    /* Maximum allowed MTU */
    u16 max_mtu;
};

struct send_queue {
    /* Virtqueue associated with this send _queue */
    struct virtqueue *vq;

    /* TX: fragments + linear part + virtio header */
    struct scatterlist sg[MAX_SKB_FRAGS + 2];

    /* Name of the send queue: output.$index */
    char name[40];
};

static int __init vring_test_init(void)
{
	struct net_device *dev;
	int i;

	read_lock(&dev_base_lock);

	dev = first_net_device(&init_net);
	while (dev) {
    		printk(KERN_INFO "found [%s]\n", dev->name);
		if (strcmp(dev->name, "lo")) {
			struct virtnet_info *vi = netdev_priv(dev);
			for (i = 0; i < vi->max_queue_pairs; i++) {
				if (i < vi->curr_queue_pairs) {
					struct vring_virtqueue *vq = vi->sq[i].vq;
					if (vq)
						printk(KERN_INFO "vq: %s, vring.used_idx: %d", vq->vq.name, virtio16_to_cpu(vq->vq.vdev, vq->vring.used->idx));
				}
			}
		}
    		dev = next_net_device(dev);
	}
	read_unlock(&dev_base_lock);

	//printk("vring.used->idx: %d", virtio16_to_cpu(vq->vq.vdev, vq->vring.used->idx));
	return 0;
}

static void __exit vring_test_exit(void)
{
}

module_init(vring_test_init);
module_exit(vring_test_exit);
MODULE_AUTHOR("Weichen Chen <weichen.chen@linux.alibaba.com>");
MODULE_LICENSE("GPL v2");
