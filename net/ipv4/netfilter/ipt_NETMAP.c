/* NETMAP - static NAT mapping of IP network addresses (1:1).
 * The mapping can be applied to source (POSTROUTING),
 * destination (PREROUTING), or both (with separate rules).
 */

/* (C) 2000-2001 Svenning Soerensen <svenning@post5.tele.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/ip.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter/x_tables.h>
#include <net/netfilter/nf_nat_rule.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Svenning Soerensen <svenning@post5.tele.dk>");
MODULE_DESCRIPTION("iptables 1:1 NAT mapping of IP networks target");

static bool
check(const char *tablename,
      const void *e,
      const struct xt_target *target,
      void *targinfo,
      unsigned int hook_mask)
{
	const struct nf_nat_multi_range_compat *mr = targinfo;

	if (!(mr->range[0].flags & IP_NAT_RANGE_MAP_IPS)) {
		pr_debug("NETMAP:check: bad MAP_IPS.\n");
		return false;
	}
	if (mr->rangesize != 1) {
		pr_debug("NETMAP:check: bad rangesize %u.\n", mr->rangesize);
		return false;
	}
	return true;
}

static unsigned int
target(struct sk_buff *skb,
       const struct net_device *in,
       const struct net_device *out,
       unsigned int hooknum,
       const struct xt_target *target,
       const void *targinfo)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	__be32 new_ip, netmask;
	const struct nf_nat_multi_range_compat *mr = targinfo;
	struct nf_nat_range newrange;

	NF_CT_ASSERT(hooknum == NF_IP_PRE_ROUTING
		     || hooknum == NF_IP_POST_ROUTING
		     || hooknum == NF_IP_LOCAL_OUT);
	ct = nf_ct_get(skb, &ctinfo);

	netmask = ~(mr->range[0].min_ip ^ mr->range[0].max_ip);

	if (hooknum == NF_IP_PRE_ROUTING || hooknum == NF_IP_LOCAL_OUT)
		new_ip = ip_hdr(skb)->daddr & ~netmask;
	else
		new_ip = ip_hdr(skb)->saddr & ~netmask;
	new_ip |= mr->range[0].min_ip & netmask;

	newrange = ((struct nf_nat_range)
		{ mr->range[0].flags | IP_NAT_RANGE_MAP_IPS,
		  new_ip, new_ip,
		  mr->range[0].min, mr->range[0].max });

	/* Hand modified range to generic setup. */
	return nf_nat_setup_info(ct, &newrange, hooknum);
}

static struct xt_target target_module __read_mostly = {
	.name 		= "NETMAP",
	.family		= AF_INET,
	.target 	= target,
	.targetsize	= sizeof(struct nf_nat_multi_range_compat),
	.table		= "nat",
	.hooks		= (1 << NF_IP_PRE_ROUTING) | (1 << NF_IP_POST_ROUTING) |
			  (1 << NF_IP_LOCAL_OUT),
	.checkentry 	= check,
	.me 		= THIS_MODULE
};

static int __init ipt_netmap_init(void)
{
	return xt_register_target(&target_module);
}

static void __exit ipt_netmap_fini(void)
{
	xt_unregister_target(&target_module);
}

module_init(ipt_netmap_init);
module_exit(ipt_netmap_fini);
