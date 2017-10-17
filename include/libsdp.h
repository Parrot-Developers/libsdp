/**
* Copyright (c) 2017 Parrot Drones SAS
* Copyright (c) 2017 Aurelien Barre
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of the Parrot Drones SAS Company nor the
*     names of its contributors may be used to endorse or promote products
*     derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE PARROT DRONES SAS COMPANY BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _LIBSDP_H_
#define _LIBSDP_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>
#include <futils/list.h>


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
	SDP_RTCP_XR_RTT_REPORT_MAX,
};


struct sdp_attr {
	char *key;
	char *value;

	struct list_node node;
};


/* H.264 payload format parameters (see RFC 6184) */
struct sdp_h264_fmtp {
	int valid;
	unsigned int packetization_mode;
	unsigned int profile_idc;
	unsigned int profile_iop;
	unsigned int level_idc;
	uint8_t *sps;
	unsigned int sps_size;
	uint8_t *pps;
	unsigned int pps_size;
};


/* RFC 3611 and RFC 7005 RTCP extended reports */
struct sdp_rtcp_xr {
	int valid;
	int loss_rle_report;
	unsigned int loss_rle_report_max_size;
	int dup_rle_report;
	unsigned int dup_rle_report_max_size;
	int pkt_receipt_times_report;
	unsigned int pkt_receipt_times_report_max_size;
	enum sdp_rtcp_xr_rtt_report_mode rtt_report;
	unsigned int rtt_report_max_size;
	int stats_summary_report_loss;
	int stats_summary_report_dup;
	int stats_summary_report_jitter;
	int stats_summary_report_ttl;
	int stats_summary_report_hl;
	int voip_metrics_report;
	int djb_metrics_report;
};


struct sdp_media {
	enum sdp_media_type type;
	char *media_title;
	char *connection_addr;
	int multicast;
	unsigned int dst_stream_port;
	unsigned int dst_control_port;
	unsigned int payload_type;
	char *control_url;
	enum sdp_start_mode start_mode;

	/* RTP/AVP rtpmap attribute */
	char *encoding_name;
	char *encoding_params;
	unsigned int clock_rate;

	/* H.264 payload format parameters */
	struct sdp_h264_fmtp h264_fmtp;

	/* RTCP extended reports */
	struct sdp_rtcp_xr rtcp_xr;

	unsigned int attr_count;
	struct list_node attrs;

	struct list_node node;
};


struct sdp_session {
	uint64_t session_id;
	uint64_t session_version;
	char *server_addr;
	char *session_name;
	char *session_info;
	char *uri;
	char *email;
	char *phone;
	char *tool;
	char *type;
	char *charset;
	char *connection_addr;
	int multicast;
	char *control_url;
	enum sdp_start_mode start_mode;

	/* RTCP extended reports */
	struct sdp_rtcp_xr rtcp_xr;

	unsigned int attr_count;
	struct list_node attrs;
	unsigned int media_count;
	struct list_node medias;
};


struct sdp_session *sdp_session_new(
	void);


int sdp_session_destroy(
	struct sdp_session *session);


struct sdp_attr *sdp_session_attr_add(
	struct sdp_session *session);


int sdp_session_attr_remove(
	struct sdp_session *session,
	struct sdp_attr *attr);


struct sdp_media *sdp_session_media_add(
	struct sdp_session *session);


int sdp_session_media_remove(
	struct sdp_session *session,
	struct sdp_media *media);


struct sdp_attr *sdp_media_attr_add(
	struct sdp_media *media);


int sdp_media_attr_remove(
	struct sdp_media *media,
	struct sdp_attr *attr);


struct sdp_session *sdp_description_read(
	const char *session_desc);


char *sdp_description_write(
	const struct sdp_session *session,
	int deletion);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_LIBSDP_H_ */
