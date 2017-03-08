/*
 * Copyright (c) 2010, Loughborough University - Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \defgroup	EMBARC_APP_CONTIKI_IPV6_MULTICAST_INTERMEDIATE	embARC Contiki IPv6 Multicast Example-Intermediate Node
 * \ingroup	EMBARC_APPS_TOTAL
 * \ingroup	EMBARC_APPS_OS_CONTIKI
 * \brief	embARC Example for IPv6 Multicast Intermediate Node on Contiki
 *
 * \details
 * ### Extra Required Tools
 *
 * ### Extra Required Peripherals
 *     * Digilent PMOD RF2(MRF24J40)
 *
 * ### Design Concept
 *     This node is part of the RPL multicast example. It basically
 *     represents a node that does not join the multicast group
 *     but still knows how to forward multicast packets
 *     The example will work with or without any number of these nodes.
 *
 *     Also, performs some sanity checks for the Contiki configuration
 *     and generates an error if the conf is bad.
 *
 *     See \ref EMBARC_APP_CONTIKI_IPV6_MULTICAST_ROOT
 *
 * ### Usage Manual
 *     See \ref EMBARC_APP_CONTIKI_IPV6_MULTICAST_ROOT
 *
 *
 * ### Extra Comments
 *
 */

/**
 * \file
 * \ingroup	EMBARC_APP_CONTIKI_IPV6_MULTICAST_INTERMEDIATE
 *
 * \author
 *         George Oikonomou - <oikonomou@users.sourceforge.net>
 */

/**
 * \addtogroup	EMBARC_APP_CONTIKI_IPV6_MULTICAST_INTERMEDIATE
 * @{
 */

#include "embARC.h"
#include "embARC_debug.h"

#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "init-net.h"

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_CONF_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif
/*---------------------------------------------------------------------------*/
PROCESS(mcast_intermediate_process, "Intermediate Process");
AUTOSTART_PROCESSES(&mcast_intermediate_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mcast_intermediate_process, ev, data)
{
	PROCESS_BEGIN();
	init_net(EMSK_ID);
	EMBARC_PRINTF("------IPv6 multcast demo (intermediate node)------\r\n");
	PROCESS_END();
}
/** @} */