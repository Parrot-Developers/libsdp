/**
 * @file libsdp.h
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

#ifndef _LIBSDP_H_
#define _LIBSDP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>


enum sdp_media_type {
	SDP_MEDIA_TYPE_AUDIO = 0,
	SDP_MEDIA_TYPE_VIDEO,
	SDP_MEDIA_TYPE_TEXT,
	SDP_MEDIA_TYPE_APPLICATION,
	SDP_MEDIA_TYPE_MESSAGE,
	SDP_MEDIA_TYPE_MAX,
};


enum sdp_start_mode {
	SDP_START_MODE_UNSPECIFIED = 0,
	SDP_START_MODE_RECVONLY,
	SDP_START_MODE_SENDRECV,
	SDP_START_MODE_SENDONLY,
	SDP_START_MODE_INACTIVE,
	SDP_START_MODE_MAX,
};


enum sdp_rtcp_xr_rtt_report_mode {
	SDP_RTCP_XR_RTT_REPORT_NONE = 0,
	SDP_RTCP_XR_RTT_REPORT_ALL,
	SDP_RTCP_XR_RTT_REPORT_SENDER,
};


struct sdp_attr {
	char *key;
	char *value;

	struct sdp_attr *prev;
	struct sdp_attr *next;
};


/* RFC 3611 and RFC 7005 RTCP extended reports */
struct sdp_rtcp_xr {
	int lossRleReport;
	unsigned int lossRleReportMaxSize;
	int dupRleReport;
	unsigned int dupRleReportMaxSize;
	int pktReceiptTimesReport;
	unsigned int pktReceiptTimesReportMaxSize;
	enum sdp_rtcp_xr_rtt_report_mode rttReport;
	unsigned int rttReportMaxSize;
	int statsSummaryReportLoss;
	int statsSummaryReportDup;
	int statsSummaryReportJitter;
	int statsSummaryReportTtl;
	int statsSummaryReportHl;
	int djbMetricsReport;
};


struct sdp_media {
	enum sdp_media_type type;
	char *mediaTitle;
	char *connectionAddr;
	int isMulticast;
	unsigned int dstStreamPort;
	unsigned int dstControlPort;
	unsigned int payloadType;
	char *controlUrl;
	enum sdp_start_mode startMode;

	/* RTP/AVP rtpmap attribute */
	char *encodingName;
	char *encodingParams;
	unsigned int clockRate;

	struct sdp_rtcp_xr rtcpXr;

	unsigned int attrCount;
	struct sdp_attr *attr;

	struct sdp_media *prev;
	struct sdp_media *next;
};


struct sdp_session {
	uint64_t sessionId;
	uint64_t sessionVersion;
	char *serverAddr;
	char *sessionName;
	char *sessionInfo;
	char *uri;
	char *email;
	char *phone;
	char *tool;
	char *type;
	char *charset;
	char *connectionAddr;
	int isMulticast;
	char *controlUrl;
	enum sdp_start_mode startMode;
	struct sdp_rtcp_xr rtcpXr;
	unsigned int attrCount;
	struct sdp_attr *attr;
	unsigned int mediaCount;
	struct sdp_media *media;
};


struct sdp_session *sdp_session_new(
	void);


int sdp_session_destroy(
	struct sdp_session *session);


struct sdp_attr *sdp_session_add_attr(
	struct sdp_session *session);


int sdp_session_remove_attr(
	struct sdp_session *session,
	struct sdp_attr *attr);


struct sdp_media *sdp_session_add_media(
	struct sdp_session *session);


int sdp_session_remove_media(
	struct sdp_session *session,
	struct sdp_media *media);


struct sdp_attr *sdp_media_add_attr(
	struct sdp_media *media);


int sdp_media_remove_attr(
	struct sdp_media *media,
	struct sdp_attr *attr);


char *sdp_generate_session_description(
	const struct sdp_session *session,
	int deletion);


struct sdp_session *sdp_parse_session_description(
	const char *sessionDescription);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_LIBSDP_H_ */
