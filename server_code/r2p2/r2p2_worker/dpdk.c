/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2015 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_arp.h>

#include "cfg.h"
#include "worker.h"

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

#define MBUF_MAIN_POOL "MBUF_POOL"
struct rte_mempool *mbuf_main_pool;

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	int i, retval;
	uint16_t q;
	bool assigned = false;

	if (port >= rte_eth_dev_count()) {
		return -1;
	}

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0) {
		return retval;
	}

	/* Allocate and set up the RX queues for each Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0) {
			return retval;
		}
	}

	/* Allocate and set up the TX queues for each Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL);
		if (retval < 0) {
			return retval;
		}
	}

	retval = rte_eth_dev_default_mac_addr_set(port, &CFG.workers[port]);
	if (retval < 0) {
		printf("Cannot set default mac addr, error %d\n", retval);
	};

	/* Display the port MAC address. */
	struct ether_addr addr;
	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			(unsigned)port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0) {
		return retval;
	}

	worker_eth_queues[port].port = port;
	worker_eth_queues[port].queue = 0;
	printf("Assigned to worker %d\n", port);

	return 0;
}

int init_dpdk() {
	unsigned nb_ports;
	uint8_t portid;

	/* Initialize the Environment Abstraction Layer (EAL). */
	char *app_name = CFG_APP_NAME;
	int ret = rte_eal_init(1, &app_name);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	}
	/* Check that there is at least one port to receive on. */
        nb_ports = rte_eth_dev_count();
        if (nb_ports < CFG.num_workers) {
		rte_exit(EXIT_FAILURE, "You need at least one port active per worker.\n");
	}

	printf("Number of ports %d\n", nb_ports);
	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_main_pool = rte_pktmbuf_pool_create(MBUF_MAIN_POOL, NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_main_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	}
	/* Initialize all ports. */
	for (portid = 0; portid < nb_ports; portid++) {
		if (port_init(portid, mbuf_main_pool) != 0) {
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
					portid);
		}
	}

	return 0;
}
