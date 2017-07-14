/**
 * @file sdp.h
 * @brief Session Description Protocol library
 * @date 08/05/2017
 * @author aurelien.barre@akaaba.net
 *
 * Copyright (c) 2017 Aurelien Barre <aurelien.barre@akaaba.net>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *   * Neither the name of the copyright holder nor the names of the
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SDP_H_
#define _SDP_H_

#include <libsdp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sdp_log.h"


#define SDP_NTP_TO_UNIX_OFFSET 2208988800ULL

#define SDP_TYPE_VERSION 'v'
#define SDP_TYPE_ORIGIN 'o'
#define SDP_TYPE_SESSION_NAME 's'
#define SDP_TYPE_INFORMATION 'i'
#define SDP_TYPE_URI 'u'
#define SDP_TYPE_EMAIL 'e'
#define SDP_TYPE_PHONE 'p'
#define SDP_TYPE_CONNECTION 'c'
#define SDP_TYPE_BANDWIDTH 'b'
#define SDP_TYPE_TIME 't'
#define SDP_TYPE_REPEAT_TIME 'r'
#define SDP_TYPE_TIME_ZONE 'z'
#define SDP_TYPE_ENCRYPTION_KEY 'k'
#define SDP_TYPE_ATTRIBUTE 'a'
#define SDP_TYPE_MEDIA 'm'

#define SDP_VERSION 0

#define SDP_PROTO_RTPAVP "RTP/AVP"

#define SDP_DYNAMIC_PAYLOAD_TYPE_MIN 96
#define SDP_DYNAMIC_PAYLOAD_TYPE_MAX 127

#define SDP_ATTR_RTPAVP_RTPMAP "rtpmap"
#define SDP_ATTR_CONTROL_URL "control"
#define SDP_ATTR_RTCP_PORT "rtcp"
#define SDP_ATTR_RTCP_XR "rtcp-xr"
#define SDP_ATTR_RTCP_XR_LOSS_RLE "pkt-loss-rle"
#define SDP_ATTR_RTCP_XR_DUP_RLE "pkt-dup-rle"
#define SDP_ATTR_RTCP_XR_RCPT_TIMES "pkt-rcpt-times"
#define SDP_ATTR_RTCP_XR_RCVR_RTT "rcvr-rtt"
#define SDP_ATTR_RTCP_XR_STAT_SUMMARY "stat-summary"
#define SDP_ATTR_RTCP_XR_STAT_LOSS "loss"
#define SDP_ATTR_RTCP_XR_STAT_DUP "dup"
#define SDP_ATTR_RTCP_XR_STAT_JITT "jitt"
#define SDP_ATTR_RTCP_XR_STAT_TTL "TTL"
#define SDP_ATTR_RTCP_XR_STAT_HL "HL"
#define SDP_ATTR_RTCP_XR_VOIP_METRICS "voip-metrics"
#define SDP_ATTR_RTCP_XR_DJB_METRICS "de-jitter-buffer"

#define SDP_ENCODING_H264 "H264"
#define SDP_H264_CLOCKRATE 90000

#define SDP_MULTICAST_ADDR_MIN 224
#define SDP_MULTICAST_ADDR_MAX 239

#endif /* !_SDP_H_ */
