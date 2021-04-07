#include <linux/etherdevice.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/udp.h>
#include <linux/version.h>
#include <net/arp.h>

MODULE_AUTHOR("P3402");
MODULE_DESCRIPTION("IOS Lab3 driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

static char* target_if = NULL;
module_param(target_if, charp, 0);
MODULE_PARM_DESC(target_if, "Target interface");

static char* target_str = NULL;
module_param(target_str, charp, 0);
MODULE_PARM_DESC(target_str, "Target string to intercept");

static char* target_proc_name = NULL;
module_param(target_proc_name, charp, 0);
MODULE_PARM_DESC(target_proc_name, "Proc entry name (example: lab4_stat)");

struct dev_state
{
	struct net_device* parent_dev;
	struct net_device_stats packet_stats;
	char data_buf[1500];
	struct proc_dir_entry* proc_entry;
};
static struct net_device* child_dev = NULL;

static struct net_device_stats* get_stats(struct net_device* dev)
{
	struct dev_state* state = netdev_priv(dev);
	return &state->packet_stats;
}

static char check_frame(struct sk_buff* skb, unsigned char data_shift)
{
	struct dev_state* state = netdev_priv(child_dev);

	unsigned char* user_data_ptr = NULL;
	struct iphdr* ip = (struct iphdr*)skb_network_header(skb);
	struct udphdr* udp = NULL;
	int data_len = 0;

	if (ip->protocol == IPPROTO_UDP)
	{
		udp = (struct udphdr*)((unsigned char*)ip + (ip->ihl * 4));
		data_len = ntohs(udp->len) - sizeof(struct udphdr);
		user_data_ptr = (unsigned char*)(skb->data + sizeof(struct iphdr) + sizeof(struct udphdr)) + data_shift;
		memcpy(state->data_buf, user_data_ptr, data_len);
		state->data_buf[data_len] = '\0';
		if (strcmp(state->data_buf, target_str) == 0)
		{
			printk(
				KERN_INFO "%s: intercepted matching UDP datagram, data: %s, saddr: %d.%d.%d.%d, daddr: %d.%d.%d.%d\n",
				THIS_MODULE->name, state->data_buf, ntohl(ip->saddr) >> 24, (ntohl(ip->saddr) >> 16) & 0x00FF,
				(ntohl(ip->saddr) >> 8) & 0x0000FF, (ntohl(ip->saddr)) & 0x000000FF, ntohl(ip->daddr) >> 24,
				(ntohl(ip->daddr) >> 16) & 0x00FF, (ntohl(ip->daddr) >> 8) & 0x0000FF, (ntohl(ip->daddr)) & 0x000000FF);
			return 1;
		}
		else
		{
			printk(KERN_INFO "%s: skipped UDP datagram, data: %s\n", THIS_MODULE->name, state->data_buf);
		}
	}
	return 0;
}

static netdev_tx_t inspect_tx_frame(struct sk_buff* skb, struct net_device* dev)
{
	struct dev_state* state = netdev_priv(dev);

	if (check_frame(skb, 14))
	{
		state->packet_stats.tx_packets++;
		state->packet_stats.tx_bytes += skb->len;
	}
	if (state->parent_dev)
	{
		skb->dev = state->parent_dev;
		skb->priority = 1;
		dev_queue_xmit(skb);
		return 0;
	}
	return NETDEV_TX_OK;
}

static rx_handler_result_t inspect_rx_frame(struct sk_buff** pskb)
{
	struct dev_state* state = netdev_priv(child_dev);

	if (check_frame(*pskb, 0))
	{
		state->packet_stats.rx_packets++;
		state->packet_stats.rx_bytes += (*pskb)->len;
	}
	(*pskb)->dev = child_dev;
	return RX_HANDLER_PASS;
}

static int lab3_dev_open(struct net_device* dev)
{
	netif_start_queue(dev);
	printk(KERN_INFO "%s: device %s opened\n", THIS_MODULE->name, dev->name);
	return 0;
}

static int lab3_dev_stop(struct net_device* dev)
{
	netif_stop_queue(dev);
	printk(KERN_INFO "%s: device %s closed\n", THIS_MODULE->name, dev->name);
	return 0;
}

static struct net_device_ops device_ops = {
	.ndo_open = lab3_dev_open,
	.ndo_stop = lab3_dev_stop,
	.ndo_get_stats = get_stats,
	.ndo_start_xmit = inspect_tx_frame,
};

// === PROC

ssize_t proc_read(struct file* f, char __user* buf, size_t count, loff_t* ppos)
{
	int buf_size = 256;
	char tmp[buf_size];
	struct dev_state* state;
	int out_len;

	if (*ppos > 0)
		return 0;

	state = netdev_priv(child_dev);

	out_len = snprintf(
		tmp, buf_size, "Packets received: %ld\nBytes received: %ld\nPackets transmitted: %ld\nBytes transmitted: %ld\n",
		state->packet_stats.rx_packets, state->packet_stats.rx_bytes, state->packet_stats.tx_packets,
		state->packet_stats.tx_bytes);
	if (out_len < 0 || out_len >= buf_size)
		return -EFAULT;

	if (copy_to_user(buf, tmp, out_len) != 0)
		return -EFAULT;
	*ppos += out_len;
	return out_len;
}

struct file_operations proc_file_ops = {
	.owner = THIS_MODULE,
	.read = proc_read,
};

// === PROC

static void setup_netdev(struct net_device* dev)
{
	int i;
	ether_setup(dev);
	dev->netdev_ops = &device_ops;

	// fill in the MAC address with a phoney
	for (i = 0; i < ETH_ALEN; i++)
		dev->dev_addr[i] = (char)i;
}

int __init lab3_init(void)
{
	int err = 0;
	struct dev_state* state;

	if (target_if == NULL || target_str == NULL || target_proc_name == NULL)
	{
		printk(KERN_ERR "%s: parameters target_if/target_str/target_proc_name are not specified\n", THIS_MODULE->name);
		return -EINVAL;
	}

	if ((child_dev = alloc_netdev(sizeof(struct dev_state), "vni%d", NET_NAME_UNKNOWN, setup_netdev)) == NULL)
	{
		printk(KERN_ERR "%s: alloc_netdev failed\n", THIS_MODULE->name);
		return -ENOMEM;
	}
	state = netdev_priv(child_dev);
	memset(state, 0, sizeof(struct dev_state));
	if ((state->parent_dev = __dev_get_by_name(&init_net, target_if)) == NULL)
	{
		printk(KERN_ERR "%s: no such net: %s\n", THIS_MODULE->name, target_if);
		free_netdev(child_dev);
		return -ENODEV;
	}
	if (state->parent_dev->type != ARPHRD_ETHER && state->parent_dev->type != ARPHRD_LOOPBACK)
	{
		printk(KERN_ERR "%s: illegal net type\n", THIS_MODULE->name);
		free_netdev(child_dev);
		return -EINVAL;
	}

	memcpy(child_dev->dev_addr, state->parent_dev->dev_addr, ETH_ALEN);
	memcpy(child_dev->broadcast, state->parent_dev->broadcast, ETH_ALEN);
	if ((err = dev_alloc_name(child_dev, child_dev->name)))
	{
		printk(KERN_ERR "%s: failed to allocate device name, error %i\n", THIS_MODULE->name, err);
		free_netdev(child_dev);
		return -EIO;
	}

	if ((state->proc_entry = proc_create(target_proc_name, 0444, NULL, &proc_file_ops)) == NULL)
	{
		printk(KERN_ERR "%s: failed to create proc device at /proc/%s\n", THIS_MODULE->name, target_proc_name);
		free_netdev(child_dev);
		return -EIO;
	}

	register_netdev(child_dev);
	rtnl_lock();
	netdev_rx_handler_register(state->parent_dev, &inspect_rx_frame, NULL);
	rtnl_unlock();
	printk(KERN_INFO "%s: loaded\n", THIS_MODULE->name);
	printk(KERN_INFO "%s: created link %s\n", THIS_MODULE->name, child_dev->name);
	printk(KERN_INFO "%s: registered rx handler for %s\n", THIS_MODULE->name, state->parent_dev->name);
	return 0;
}

void __exit lab3_exit(void)
{
	struct dev_state* state = netdev_priv(child_dev);
	if (state->parent_dev)
	{
		rtnl_lock();
		netdev_rx_handler_unregister(state->parent_dev);
		rtnl_unlock();
		printk(KERN_INFO "%s: unregistered rx handler for %s\n", THIS_MODULE->name, state->parent_dev->name);
	}
	proc_remove(state->proc_entry);
	unregister_netdev(child_dev);
	free_netdev(child_dev);
	printk(KERN_INFO "%s: unloaded\n", THIS_MODULE->name);
}

module_init(lab3_init);
module_exit(lab3_exit);
