/**
 * @file sdp.c
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

#include "sdp.h"


static const char *const sdp_media_type_str[] = {
	"audio",
	"video",
	"text",
	"application",
	"message",
};


static const char *const sdp_start_mode_str[] = {
	"unspecified",
	SDP_ATTR_RECVONLY,
	SDP_ATTR_SENDRECV,
	SDP_ATTR_SENDONLY,
	SDP_ATTR_INACTIVE,
};


static const char *const sdp_rtcp_xr_rtt_report_mode_str[] = {
	"none",
	"all",
	"sender",
};


struct sdp_session *sdp_session_new(
	void)
{
	struct sdp_session *session = calloc(1, sizeof(*session));
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -ENOMEM, NULL);
	list_init(&session->attrs);
	list_init(&session->medias);

	return session;
}


int sdp_session_destroy(
	struct sdp_session *session)
{
	SDP_RETURN_ERR_IF_FAILED(session != NULL, -EINVAL);

	/* Remove all attibutes */
	struct sdp_attr *attr = NULL, *tmp_attr = NULL;
	list_walk_entry_forward_safe(&session->attrs, attr, tmp_attr, node) {
		int ret = sdp_session_remove_attr(session, attr);
		if (ret != 0)
			SDP_LOGE("sdp_session_remove_attr() failed: %d(%s)",
				ret, strerror(-ret));
	}

	/* Remove all media */
	struct sdp_media *media = NULL, *tmp_media = NULL;
	list_walk_entry_forward_safe(&session->medias, media, tmp_media, node) {
		int ret = sdp_session_remove_media(session, media);
		if (ret != 0)
			SDP_LOGE("sdp_session_remove_media() failed: %d(%s)",
				ret, strerror(-ret));
	}

	free(session->server_addr);
	free(session->session_name);
	free(session->session_info);
	free(session->uri);
	free(session->email);
	free(session->phone);
	free(session->tool);
	free(session->type);
	free(session->charset);
	free(session->connection_addr);
	free(session->control_url);
	free(session);

	return 0;
}


struct sdp_attr *sdp_session_add_attr(
	struct sdp_session *session)
{
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -EINVAL, NULL);

	struct sdp_attr *attr = calloc(1, sizeof(*attr));
	SDP_RETURN_VAL_IF_FAILED(attr != NULL, -ENOMEM, NULL);
	list_node_unref(&attr->node);

	/* Add to the list */
	list_add_after(list_last(&session->attrs), &attr->node);
	session->attr_count++;

	return attr;
}


int sdp_session_remove_attr(
	struct sdp_session *session,
	struct sdp_attr *attr)
{
	SDP_RETURN_ERR_IF_FAILED(session != NULL, -EINVAL);
	SDP_RETURN_ERR_IF_FAILED(attr != NULL, -EINVAL);

	int found = 0;
	struct sdp_attr *_attr = NULL;
	list_walk_entry_forward(&session->attrs, _attr, node) {
		if (_attr == attr) {
			found = 1;
			break;
		}
	}

	if (!found) {
		SDP_LOGE("failed to find the attribute in the list");
		return -EINVAL;
	}

	/* Remove from the list */
	list_del(&attr->node);
	session->attr_count--;

	free(attr->key);
	free(attr->value);
	free(attr);

	return 0;
}


struct sdp_media *sdp_session_add_media(
	struct sdp_session *session)
{
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -EINVAL, NULL);

	struct sdp_media *media = calloc(1, sizeof(*media));
	SDP_RETURN_VAL_IF_FAILED(media != NULL, -ENOMEM, NULL);
	list_node_unref(&media->node);
	list_init(&media->attrs);

	/* Add to the list */
	list_add_after(list_last(&session->medias), &media->node);
	session->media_count++;

	return media;
}


int sdp_session_remove_media(
	struct sdp_session *session,
	struct sdp_media *media)
{
	SDP_RETURN_ERR_IF_FAILED(session != NULL, -EINVAL);
	SDP_RETURN_ERR_IF_FAILED(media != NULL, -EINVAL);

	int found = 0;
	struct sdp_media *_media = NULL;
	list_walk_entry_forward(&session->medias, _media, node) {
		if (_media == media) {
			found = 1;
			break;
		}
	}

	if (!found) {
		SDP_LOGE("failed to find the media in the list");
		return -EINVAL;
	}

	/* Remove all attibutes */
	struct sdp_attr *attr = NULL, *tmp_attr = NULL;
	list_walk_entry_forward_safe(&media->attrs, attr, tmp_attr, node) {
		int ret = sdp_media_remove_attr(media, attr);
		if (ret != 0)
			SDP_LOGE("sdp_media_remove_attr() failed: %d(%s)",
				ret, strerror(-ret));
	}

	/* Remove from the list */
	list_del(&media->node);
	session->media_count--;

	free(media->media_title);
	free(media->connection_addr);
	free(media->control_url);
	free(media->encoding_name);
	free(media->encoding_params);
	free(media->h264_fmtp.sps);
	free(media->h264_fmtp.pps);
	free(media);

	return 0;
}


struct sdp_attr *sdp_media_add_attr(
	struct sdp_media *media)
{
	SDP_RETURN_VAL_IF_FAILED(media != NULL, -EINVAL, NULL);

	struct sdp_attr *attr = calloc(1, sizeof(*attr));
	SDP_RETURN_VAL_IF_FAILED(attr != NULL, -ENOMEM, NULL);

	/* Add to the list */
	list_add_after(list_last(&media->attrs), &attr->node);
	media->attr_count++;

	return attr;
}


int sdp_media_remove_attr(
	struct sdp_media *media,
	struct sdp_attr *attr)
{
	SDP_RETURN_ERR_IF_FAILED(media != NULL, -EINVAL);
	SDP_RETURN_ERR_IF_FAILED(attr != NULL, -EINVAL);

	int found = 0;
	struct sdp_attr *_attr = NULL;
	list_walk_entry_forward(&media->attrs, _attr, node) {
		if (_attr == attr) {
			found = 1;
			break;
		}
	}

	if (!found) {
		SDP_LOGE("failed to find the attribute in the list");
		return -EINVAL;
	}

	/* Remove from the list */
	list_del(&attr->node);
	media->attr_count--;

	free(attr);

	return 0;
}


static int sdp_generate_h264_fmtp(
	const struct sdp_h264_fmtp *fmtp,
	unsigned int payload_type,
	char *sdp,
	int sdp_max_len)
{
	char h264_format[200];
	int h264_format_len = 0, sdp_len = 0;

	h264_format_len += snprintf(h264_format + h264_format_len,
		sizeof(h264_format) - h264_format_len,
		"%s=%d;", SDP_FMTP_H264_PACKETIZATION,
		fmtp->packetization_mode);
	h264_format_len += snprintf(h264_format + h264_format_len,
		sizeof(h264_format) - h264_format_len,
		"%s=%02X%02X%02X;", SDP_FMTP_H264_PROFILE_LEVEL,
		fmtp->profile_idc, fmtp->profile_iop, fmtp->level_idc);
	if ((fmtp->sps) && (fmtp->sps_size) &&
		(fmtp->pps) && (fmtp->pps_size)) {
		int ret = 0, err;
		char *sps_b64 = NULL;
		char *pps_b64 = NULL;
		err = sdp_base64_encode((void *)fmtp->sps,
			(size_t)fmtp->sps_size, &sps_b64);
		if (err != 0)
			ret = err;
		err = sdp_base64_encode((void *)fmtp->pps,
			(size_t)fmtp->pps_size, &pps_b64);
		if (err != 0)
			ret = err;
		if (ret == 0) {
			h264_format_len += snprintf(
				h264_format + h264_format_len,
				sizeof(h264_format) - h264_format_len,
				"%s=%s,%s;", SDP_FMTP_H264_PARAM_SETS,
				sps_b64, pps_b64);
		}
		free(sps_b64);
		free(pps_b64);
	}
	if (h264_format_len > 0) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%d %s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_FMTP,
			payload_type,
			h264_format);
	}

	return sdp_len;
}


static int sdp_parse_h264_fmtp(
	struct sdp_h264_fmtp *fmtp,
	char *value)
{
	int ret = 0;
	char *temp1 = NULL;
	char *param = NULL;

	fmtp->valid = 0;
	param = strtok_r(value, ";", &temp1);
	while (param) {
		if (!strncmp(param, SDP_FMTP_H264_PROFILE_LEVEL,
			strlen(SDP_FMTP_H264_PROFILE_LEVEL))) {
			char *p2 = strchr(param, '=');
			uint32_t profile_level_id = 0;
			if (p2 != NULL)
				sscanf(p2 + 1, "%6X", &profile_level_id);
			fmtp->profile_idc = (profile_level_id >> 16) & 0xFF;
			fmtp->profile_iop = (profile_level_id >> 8) & 0xFF;
			fmtp->level_idc = profile_level_id & 0xFF;
		} else if (!strncmp(param, SDP_FMTP_H264_PACKETIZATION,
			strlen(SDP_FMTP_H264_PACKETIZATION))) {
			char *p2 = strchr(param, '=');
			if (p2 != NULL)
				fmtp->packetization_mode = atoi(p2 + 1);
		} else if (!strncmp(param, SDP_FMTP_H264_PARAM_SETS,
			strlen(SDP_FMTP_H264_PARAM_SETS))) {
			char *p2 = strchr(param, '=');
			char *p3 = NULL;
			if (p2 != NULL)
				p3 = strchr(p2 + 1, ',');
			if ((p2 != NULL) && (p3 != NULL)) {
				*p3 = '\0';
				char *sps_b64 = p2 + 1;
				char *pps_b64 = p3 + 1;
				void *sps = NULL;
				size_t sps_size = 0;
				void *pps = NULL;
				size_t pps_size = 0;
				int err = sdp_base64_decode(sps_b64,
					&sps, &sps_size);
				if (err != 0)
					ret = err;
				err = sdp_base64_decode(pps_b64,
					&pps, &pps_size);
				if (err != 0)
					ret = err;
				if (ret != 0) {
					free(sps);
					free(pps);
				} else {
					fmtp->sps = (uint8_t *)sps;
					fmtp->sps_size = (unsigned int)sps_size;
					fmtp->pps = (uint8_t *)pps;
					fmtp->pps_size = (unsigned int)pps_size;
				}
			}
		}

		param = strtok_r(NULL, ";", &temp1);
	};

	fmtp->valid = 1;
	return ret;
}


static int sdp_generate_rtcp_xr_attr(
	const struct sdp_rtcp_xr *xr,
	char *sdp,
	int sdp_max_len)
{
	char xr_format[100];
	int xr_format_len = 0, is_first = 1, sdp_len = 0;
	if (xr->loss_rle_report) {
		if (xr->loss_rle_report_max_size > 0)
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s=%d", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_LOSS_RLE,
				xr->loss_rle_report_max_size);
		else
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_LOSS_RLE);
		is_first = 0;
	}
	if (xr->dup_rle_report) {
		if (xr->dup_rle_report_max_size > 0)
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s=%d", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_DUP_RLE,
				xr->dup_rle_report_max_size);
		else
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_DUP_RLE);
		is_first = 0;
	}
	if (xr->pkt_receipt_times_report) {
		if (xr->pkt_receipt_times_report_max_size > 0)
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s=%d", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCPT_TIMES,
				xr->pkt_receipt_times_report_max_size);
		else
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCPT_TIMES);
		is_first = 0;
	}
	if ((xr->rtt_report > SDP_RTCP_XR_RTT_REPORT_NONE) &&
		(xr->rtt_report <= SDP_RTCP_XR_RTT_REPORT_SENDER)) {
		if (xr->loss_rle_report_max_size > 0)
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s=%s:%d", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCVR_RTT,
				sdp_rtcp_xr_rtt_report_mode_str[
					xr->rtt_report],
				xr->loss_rle_report_max_size);
		else
			xr_format_len += snprintf(xr_format + xr_format_len,
				sizeof(xr_format) - xr_format_len,
				"%s%s=%s", (is_first) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCVR_RTT,
				sdp_rtcp_xr_rtt_report_mode_str[
					xr->rtt_report]);
		is_first = 0;
	}
	char stat_flag[100];
	int stat_flag_len = 0, is_first2 = 1;
	if (xr->stats_summary_report_loss) {
		stat_flag_len += snprintf(stat_flag + stat_flag_len,
			sizeof(stat_flag) - stat_flag_len,
			"%s%s", (is_first2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_LOSS);
		is_first2 = 0;
	}
	if (xr->stats_summary_report_dup) {
		stat_flag_len += snprintf(stat_flag + stat_flag_len,
			sizeof(stat_flag) - stat_flag_len,
			"%s%s", (is_first2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_DUP);
		is_first2 = 0;
	}
	if (xr->stats_summary_report_jitter) {
		stat_flag_len += snprintf(stat_flag + stat_flag_len,
			sizeof(stat_flag) - stat_flag_len,
			"%s%s", (is_first2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_JITT);
		is_first2 = 0;
	}
	if (xr->stats_summary_report_ttl) {
		stat_flag_len += snprintf(stat_flag + stat_flag_len,
			sizeof(stat_flag) - stat_flag_len,
			"%s%s", (is_first2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_TTL);
		is_first2 = 0;
	}
	if (xr->stats_summary_report_hl) {
		stat_flag_len += snprintf(stat_flag + stat_flag_len,
			sizeof(stat_flag) - stat_flag_len,
			"%s%s", (is_first2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_HL);
		is_first2 = 0;
	}
	if (stat_flag_len > 0) {
		xr_format_len += snprintf(xr_format + xr_format_len,
			sizeof(xr_format) - xr_format_len,
			"%s%s=%s", (is_first) ? "" : " ",
			SDP_ATTR_RTCP_XR_STAT_SUMMARY,
			stat_flag);
		is_first = 0;
	}

	if (xr->voip_metrics_report) {
		xr_format_len += snprintf(xr_format + xr_format_len,
			sizeof(xr_format) - xr_format_len,
			"%s%s", (is_first) ? "" : " ",
			SDP_ATTR_RTCP_XR_VOIP_METRICS);
		is_first = 0;
	}

	if (xr->djb_metrics_report) {
		xr_format_len += snprintf(xr_format + xr_format_len,
			sizeof(xr_format) - xr_format_len,
			"%s%s", (is_first) ? "" : " ",
			SDP_ATTR_RTCP_XR_DJB_METRICS);
		is_first = 0;
	}
	if (xr_format_len > 0) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_RTCP_XR,
			xr_format);
	}

	return sdp_len;
}


static int sdp_parse_rtcp_xr_attr(
	struct sdp_rtcp_xr *xr,
	char *value)
{
	int ret = 0;
	char *temp1 = NULL;
	char *xr_format = NULL;

	xr->valid = 0;
	xr_format = strtok_r(value, " ", &temp1);
	while (xr_format) {
		if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_LOSS_RLE,
			strlen(SDP_ATTR_RTCP_XR_LOSS_RLE))) {
			xr->loss_rle_report = 1;
			char *p2 = strchr(xr_format, '=');
			if (p2 != NULL)
				xr->loss_rle_report_max_size = atoi(p2 + 1);
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_DUP_RLE,
			strlen(SDP_ATTR_RTCP_XR_DUP_RLE))) {
			xr->dup_rle_report = 1;
			char *p2 = strchr(xr_format, '=');
			if (p2 != NULL)
				xr->dup_rle_report_max_size = atoi(p2 + 1);
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_RCPT_TIMES,
			strlen(SDP_ATTR_RTCP_XR_RCPT_TIMES))) {
			xr->pkt_receipt_times_report = 1;
			char *p2 = strchr(xr_format, '=');
			if (p2 != NULL)
				xr->pkt_receipt_times_report_max_size =
					atoi(p2 + 1);
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_RCVR_RTT,
			strlen(SDP_ATTR_RTCP_XR_RCVR_RTT))) {
			char *p2 = strchr(xr_format, '=');
			if ((p2 != NULL) && (!strncmp(p2 + 1,
				sdp_rtcp_xr_rtt_report_mode_str[1],
				strlen(sdp_rtcp_xr_rtt_report_mode_str[1]))))
				xr->rtt_report = SDP_RTCP_XR_RTT_REPORT_ALL;
			else if ((p2 != NULL) && (!strncmp(p2 + 1,
				sdp_rtcp_xr_rtt_report_mode_str[1],
				strlen(sdp_rtcp_xr_rtt_report_mode_str[1]))))
				xr->rtt_report = SDP_RTCP_XR_RTT_REPORT_SENDER;
			else
				xr->rtt_report = SDP_RTCP_XR_RTT_REPORT_NONE;
			p2 = strchr(xr_format, ':');
			if (p2 != NULL)
				xr->rtt_report_max_size = atoi(p2 + 1);
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_STAT_SUMMARY,
			strlen(SDP_ATTR_RTCP_XR_STAT_SUMMARY))) {
			char *p2 = strchr(xr_format, '=');
			if (p2 == NULL)
				continue;
			p2++;
			char *temp2 = NULL;
			char *stat_flag = NULL;
			stat_flag = strtok_r(p2, ",", &temp2);
			while (stat_flag) {
				if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_LOSS,
					strlen(SDP_ATTR_RTCP_XR_STAT_LOSS)))
					xr->stats_summary_report_loss = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_DUP,
					strlen(SDP_ATTR_RTCP_XR_STAT_DUP)))
					xr->stats_summary_report_dup = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_JITT,
					strlen(SDP_ATTR_RTCP_XR_STAT_JITT)))
					xr->stats_summary_report_jitter = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_TTL,
					strlen(SDP_ATTR_RTCP_XR_STAT_TTL)))
					xr->stats_summary_report_ttl = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_HL,
					strlen(SDP_ATTR_RTCP_XR_STAT_HL)))
					xr->stats_summary_report_hl = 1;
				stat_flag = strtok_r(NULL, ",", &temp2);
			}

		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_VOIP_METRICS,
			strlen(SDP_ATTR_RTCP_XR_VOIP_METRICS))) {
			xr->voip_metrics_report = 1;

		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_DJB_METRICS,
			strlen(SDP_ATTR_RTCP_XR_DJB_METRICS)))
			xr->djb_metrics_report = 1;

		xr_format = strtok_r(NULL, " ", &temp1);
	};

	xr->valid = 1;
	return ret;
}


static int sdp_parse_attr(
	struct sdp_attr *attr,
	struct sdp_session *session,
	struct sdp_media *media,
	char *value)
{
	int ret = 0;
	char *temp2 = NULL;
	char *attr_key = strtok_r(value, ":", &temp2);
	char *attr_value = strtok_r(NULL, "\n", &temp2);
	attr->key = strdup(attr_key);
	if (attr_value)
		attr->value = strdup(attr_value);

	if ((!strncmp(attr_key, SDP_ATTR_RTPAVP_RTPMAP,
		strlen(SDP_ATTR_RTPAVP_RTPMAP))) && (attr_value)) {
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(media != NULL, -1,
			"attribute 'rtpmap' not on media level");
		char *temp3 = NULL;
		char *payload_type = NULL;
		char *encoding_name = NULL;
		char *clock_rate = NULL;
		char *encoding_params = NULL;
		payload_type = strtok_r(attr_value, " ", &temp3);
		unsigned int payload_type_int =
			(payload_type) ? atoi(payload_type) : 0;
		encoding_name = strtok_r(NULL, "/", &temp3);
		clock_rate = strtok_r(NULL, "/", &temp3);
		unsigned int i_clock_rate =
			(clock_rate) ? atoi(clock_rate) : 0;
		encoding_params = strtok_r(NULL, "/", &temp3);
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(
			payload_type_int == media->payload_type, -1,
			"invalid payload type (%d vs. %d)",
			payload_type_int, media->payload_type);
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(encoding_name != NULL, -1,
			"invalid encoding name");
		/* clock rate must be 90000 for H.264
		 * (RFC6184 ch. 8.2.1) */
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(
			((strncmp(encoding_name, SDP_ENCODING_H264,
			strlen(SDP_ENCODING_H264))) ||
			(i_clock_rate == SDP_H264_CLOCKRATE)), -1,
			"unsupported clock rate %d", i_clock_rate);
		media->encoding_name = strdup(encoding_name);
		if (encoding_params)
			media->encoding_params = strdup(encoding_params);
		media->clock_rate = i_clock_rate;
		SDP_LOGD("SDP: payload_type=%d"
			" encoding_name=%s clock_rate=%d"
			" encoding_params=%s",
			payload_type_int, encoding_name,
			i_clock_rate, encoding_params);
	} else if ((!strncmp(attr_key, SDP_ATTR_FMTP,
		strlen(SDP_ATTR_FMTP))) && (attr_value)) {
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(media != NULL, -1,
			"attribute 'fmtp' not on media level");
		char *temp3 = NULL;
		char *payload_type = NULL;
		char *fmtp = NULL;
		payload_type = strtok_r(attr_value, " ", &temp3);
		unsigned int payload_type_int =
			(payload_type) ? atoi(payload_type) : 0;
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(
			payload_type_int == media->payload_type, -1,
			"invalid payload type (%d vs. %d)",
			payload_type_int, media->payload_type);
		if ((media->encoding_name) && (!strncmp(media->encoding_name,
			SDP_ENCODING_H264, strlen(SDP_ENCODING_H264)))) {
			fmtp = strtok_r(NULL, "", &temp3);
			ret = sdp_parse_h264_fmtp(&media->h264_fmtp, fmtp);
			if (ret < 0) {
				SDP_LOGW("sdp_parse_h264_fmtp()"
					" failed (%d)", ret);
			}
		}
	} else if ((!strncmp(attr_key, SDP_ATTR_TOOL,
		strlen(SDP_ATTR_TOOL))) && (attr_value)) {
		if (media)
			SDP_LOGW("attribute 'tool' not on session level");
		else
			session->tool = strdup(attr_value);
	} else if ((!strncmp(attr_key, SDP_ATTR_TYPE,
		strlen(SDP_ATTR_TYPE))) && (attr_value)) {
		if (media)
			SDP_LOGW("attribute 'type' not on session level");
		else
			session->type = strdup(attr_value);
	} else if ((!strncmp(attr_key, SDP_ATTR_CHARSET,
		strlen(SDP_ATTR_CHARSET))) && (attr_value)) {
		if (media)
			SDP_LOGW("attribute 'charset' not on session level");
		else
			session->charset = strdup(attr_value);
	} else if ((!strncmp(attr_key, SDP_ATTR_CONTROL_URL,
		strlen(SDP_ATTR_CONTROL_URL))) && (attr_value)) {
		if (media)
			media->control_url = strdup(attr_value);
		else
			session->control_url = strdup(attr_value);
	} else if (!strncmp(attr_key, SDP_ATTR_RECVONLY,
		strlen(SDP_ATTR_RECVONLY))) {
		if (media)
			media->start_mode = SDP_START_MODE_RECVONLY;
		else
			session->start_mode = SDP_START_MODE_RECVONLY;
	} else if (!strncmp(attr_key, SDP_ATTR_SENDRECV,
		strlen(SDP_ATTR_SENDRECV))) {
		if (media)
			media->start_mode = SDP_START_MODE_SENDRECV;
		else
			session->start_mode = SDP_START_MODE_SENDRECV;
	} else if (!strncmp(attr_key, SDP_ATTR_SENDONLY,
		strlen(SDP_ATTR_SENDONLY))) {
		if (media)
			media->start_mode = SDP_START_MODE_SENDONLY;
		else
			session->start_mode = SDP_START_MODE_SENDONLY;
	} else if (!strncmp(attr_key, SDP_ATTR_INACTIVE,
		strlen(SDP_ATTR_INACTIVE))) {
		if (media)
			media->start_mode = SDP_START_MODE_INACTIVE;
		else
			session->start_mode = SDP_START_MODE_INACTIVE;
	} else if ((!strncmp(attr_key, SDP_ATTR_RTCP_XR,
		strlen(SDP_ATTR_RTCP_XR))) && (attr_value)) {
		if (media)
			ret = sdp_parse_rtcp_xr_attr(
				&media->rtcp_xr, attr_value);
		else
			ret = sdp_parse_rtcp_xr_attr(
				&session->rtcp_xr, attr_value);
		if (ret < 0) {
			SDP_LOGW("sdp_parse_rtcp_xr_attr()"
				" failed (%d)", ret);
		}
	} else if ((!strncmp(attr_key, SDP_ATTR_RTCP_PORT,
		strlen(SDP_ATTR_RTCP_PORT))) && (attr_value)) {
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(media != NULL, -1,
			"attribute 'rtcp' not on media level");
		int port = atoi(attr_value);
		if (port > 0) {
			media->dst_control_port = port;
			SDP_LOGD("SDP: rtcp_dst_port=%d", port);
		}
	}

	return ret;
}


static int sdp_generate_media(
	const struct sdp_media *media,
	char *sdp,
	int sdp_max_len,
	int session_level_connection_addr)
{
	int sdp_len = 0, ret, error = 0;

	if (((!media->connection_addr) || (!strlen(media->connection_addr))) &&
		(!session_level_connection_addr)) {
		SDP_LOGE("invalid connection address");
		return -1;
	}
	if ((!media->dst_stream_port) || (!media->dst_control_port)) {
		SDP_LOGE("invalid port");
		return -1;
	}
	if (!media->payload_type) {
		SDP_LOGE("invalid payload type");
		return -1;
	}
	if ((media->type < 0) || (media->type >= SDP_MEDIA_TYPE_MAX)) {
		SDP_LOGE("invalid media type");
		return -1;
	}
	if ((!media->encoding_name) || (!strlen(media->encoding_name))) {
		SDP_LOGE("invalid encoding name");
		return -1;
	}

	/* Media Description (m=<media> <port> <proto> <fmt> ...) */
	sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
		"%c=%s %d " SDP_PROTO_RTPAVP " %d\r\n",
		SDP_TYPE_MEDIA,
		sdp_media_type_str[media->type],
		media->dst_stream_port,
		media->payload_type);

	/* Media Title (i=<media title>) */
	if ((media->media_title) && (strlen(media->media_title)))
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_INFORMATION,
			media->media_title);

	/* Connection Data (c=<nettype> <addrtype> <connection-address>) */
	if ((media->connection_addr) && (strlen(media->connection_addr))) {
		int multicast = 0;
		int addr_first = atoi(media->connection_addr);
		if ((addr_first >= SDP_MULTICAST_ADDR_MIN) &&
			(addr_first <= SDP_MULTICAST_ADDR_MAX))
			multicast = 1;
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=IN IP4 %s%s\r\n",
			SDP_TYPE_CONNECTION,
			media->connection_addr,
			(multicast) ? "/127" : "");
	}

	/* Start mode */
	if ((media->start_mode > SDP_START_MODE_UNSPECIFIED) &&
		(media->start_mode < SDP_START_MODE_MAX)) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			sdp_start_mode_str[media->start_mode]);
	}

	/* Control URL for use with RTSP */
	if ((media->control_url) && (strlen(media->control_url))) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_CONTROL_URL,
			media->control_url);
	}

	/* RTP/AVP rtpmap attribute */
	sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
		"%c=%s:%d %s/%d%s%s\r\n",
		SDP_TYPE_ATTRIBUTE,
		SDP_ATTR_RTPAVP_RTPMAP,
		media->payload_type,
		media->encoding_name,
		media->clock_rate,
		((media->encoding_params) && (strlen(media->encoding_params))) ?
			"/" : "",
		((media->encoding_params) && (strlen(media->encoding_params))) ?
			media->encoding_params : "");

	/* H.264 payload format parameters */
	if (!strncmp(media->encoding_name,
		SDP_ENCODING_H264, strlen(SDP_ENCODING_H264)) &&
		(media->h264_fmtp.valid)) {
		ret = sdp_generate_h264_fmtp(
			&media->h264_fmtp, media->payload_type,
			sdp + sdp_len, sdp_max_len - sdp_len);
		if (ret < 0) {
			SDP_LOGE("sdp_generate_h264_fmtp()"
				" failed (%d)", ret);
			error = 1;
		} else
			sdp_len += ret;
	}

	/* RTCP destination port (if not RTP port + 1) */
	if (media->dst_control_port != media->dst_stream_port + 1)
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%d\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_RTCP_PORT,
			media->dst_control_port);

	/* RTCP extended reports attribute */
	if (media->rtcp_xr.valid) {
		ret = sdp_generate_rtcp_xr_attr(&media->rtcp_xr,
			sdp + sdp_len, sdp_max_len - sdp_len);
		if (ret < 0) {
			SDP_LOGE("sdp_generate_rtcp_xr_attr()"
				" failed (%d)", ret);
			error = 1;
		} else
			sdp_len += ret;
	}

	/* Other attributes (a=<attribute>:<value> or a=<attribute>) */
	struct sdp_attr *attr = NULL;
	list_walk_entry_forward(&media->attrs, attr, node) {
		if ((attr->key) && (strlen(attr->key))) {
			if ((attr->value) && (strlen(attr->value)))
				sdp_len += snprintf(sdp + sdp_len,
					sdp_max_len - sdp_len, "%c=%s:%s\r\n",
					SDP_TYPE_ATTRIBUTE,
					attr->key,
					attr->value);
			else
				sdp_len += snprintf(sdp + sdp_len,
					sdp_max_len - sdp_len, "%c=%s\r\n",
					SDP_TYPE_ATTRIBUTE,
					attr->key);
		}
	}

	return (error) ? -error : sdp_len;
}


static int sdp_parse_media(
	struct sdp_media *media,
	char *value)
{
	int ret = 0;
	char *temp2 = NULL;
	char *smedia = strtok_r(value, " ", &temp2);
	char *port = strtok_r(NULL, " ", &temp2);
	char *proto = strtok_r(NULL, " ", &temp2);
	char *fmt = strtok_r(NULL, " ", &temp2);
	if (smedia) {
		int i;
		for (i = 0; i < SDP_MEDIA_TYPE_MAX; i++) {
			if (!strcmp(smedia, sdp_media_type_str[i]) != 0) {
				media->type = i;
				break;
			}
		}
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(i != SDP_MEDIA_TYPE_MAX,
			-1, "unsupported media type '%s'", smedia);
	} else
		SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(0, -1, "null media type");
	int port_int = (port) ? atoi(port) : 0;
	if (port_int) {
		media->dst_stream_port = port_int;
		media->dst_control_port = port_int + 1;
	}
	SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(((proto != NULL) &&
		(!strncmp(proto, SDP_PROTO_RTPAVP,
		strlen(SDP_PROTO_RTPAVP)))), -1,
		"unsupported protocol '%s'", (proto) ? proto : "");
	media->payload_type = (fmt) ? atoi(fmt) : 0;
	/* payload type must be dynamic
	 * (RFC3551 ch. 6) */
	SDP_LOG_ERR_AND_RETURN_ERR_IF_FAILED(
		((media->payload_type >= SDP_DYNAMIC_PAYLOAD_TYPE_MIN) &&
		(media->payload_type <= SDP_DYNAMIC_PAYLOAD_TYPE_MAX)), -1,
		"unsupported payload type (%d)", media->payload_type);

	SDP_LOGD("SDP: media=%s port=%d proto=%s payload_type=%d",
		smedia, port_int, proto, media->payload_type);

	return ret;
}


char *sdp_generate_session_description(
	const struct sdp_session *session,
	int deletion)
{
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -EINVAL, NULL);

	if ((!session->server_addr) ||
		(!strlen(session->server_addr))) {
		SDP_LOGE("invalid server address");
		SDP_RETURN_VAL_IF_FAILED(0, -EINVAL, NULL);
	}

	int error = 0, ret;
	int sdp_max_len = 1024, sdp_len = 0;
	int session_level_connection_addr = 0;

	char *sdp = malloc(sdp_max_len);
	SDP_RETURN_VAL_IF_FAILED(sdp != NULL, -ENOMEM, NULL);

	if (deletion) {
		/* Origin (o=<username> <sess-id> <sess-version>
		 * <nettype> <addrtype> <unicast-address>) */
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=- %"PRIu64" %"PRIu64" IN IP4 %s\r\n",
			SDP_TYPE_ORIGIN,
			session->session_id,
			session->session_version,
			session->server_addr);

		return sdp;
	}

	/* Protocol Version (v=0) */
	sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
		"%c=%d\r\n",
		SDP_TYPE_VERSION,
		SDP_VERSION);

	/* Origin (o=<username> <sess-id> <sess-version>
	 * <nettype> <addrtype> <unicast-address>) */
	sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
		"%c=- %"PRIu64" %"PRIu64" IN IP4 %s\r\n",
		SDP_TYPE_ORIGIN,
		session->session_id,
		session->session_version,
		session->server_addr);

	/* Session Name (s=<session name>) */
	if ((session->session_name) && (strlen(session->session_name)))
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_SESSION_NAME,
			session->session_name);
	else
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c= \r\n",
			SDP_TYPE_SESSION_NAME);

	/* Session Information (i=<session description>) */
	if ((session->session_info) && (strlen(session->session_info)))
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_INFORMATION,
			session->session_info);

	/* URI (u=<uri>) */
	if ((session->uri) && (strlen(session->uri)))
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_URI,
			session->uri);

	/* Email Address (e=<email-address>) */
	if ((session->email) && (strlen(session->email)))
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_EMAIL,
			session->email);

	/* Phone Number (p=<phone-number>) */
	if ((session->phone) && (strlen(session->phone)))
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_PHONE,
			session->phone);

	/* Connection Data (c=<nettype> <addrtype>
	 * <connection-address>) */
	if ((session->connection_addr) && (strlen(session->connection_addr))) {
		session_level_connection_addr = 1;
		int multicast = 0;
		int addr_first = atoi(session->connection_addr);
		if ((addr_first >= SDP_MULTICAST_ADDR_MIN) &&
			(addr_first <= SDP_MULTICAST_ADDR_MAX))
			multicast = 1;
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=IN IP4 %s%s\r\n",
			SDP_TYPE_CONNECTION,
			session->connection_addr,
			(multicast) ? "/127" : "");
	}

	/* timing (t=<start-time> <stop-time>) */
	/*TODO*/
	sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
		"%c=0 0\r\n",
		SDP_TYPE_TIME);

	/* tool (a=tool) */
	if ((session->tool) && (strlen(session->tool))) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_TOOL,
			session->tool);
	}

	/* Start mode */
	if ((session->start_mode > SDP_START_MODE_UNSPECIFIED) &&
		(session->start_mode < SDP_START_MODE_MAX)) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			sdp_start_mode_str[session->start_mode]);
	}

	/* Session type */
	if ((session->type) && (strlen(session->type))) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_TYPE,
			session->type);
	}

	/* Charset */
	if ((session->charset) && (strlen(session->charset))) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_CHARSET,
			session->charset);
	}

	/* Control URL for use with RTSP */
	if ((session->control_url) && (strlen(session->control_url))) {
		sdp_len += snprintf(sdp + sdp_len, sdp_max_len - sdp_len,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_CONTROL_URL,
			session->control_url);
	}

	/* RTCP extended reports attribute */
	if (session->rtcp_xr.valid) {
		ret = sdp_generate_rtcp_xr_attr(&session->rtcp_xr,
			sdp + sdp_len, sdp_max_len - sdp_len);
		if (ret < 0) {
			SDP_LOGE("sdp_generate_rtcp_xr_attr()"
				" failed (%d)", ret);
			error = 1;
		} else
			sdp_len += ret;
	}

	/* Other attributes (a=<attribute>:<value> or a=<attribute>) */
	struct sdp_attr *attr = NULL;
	list_walk_entry_forward(&session->attrs, attr, node) {
		if ((attr->key) && (strlen(attr->key))) {
			if ((attr->value) && (strlen(attr->value)))
				sdp_len += snprintf(sdp + sdp_len,
					sdp_max_len - sdp_len,
					"%c=%s:%s\r\n",
					SDP_TYPE_ATTRIBUTE,
					attr->key,
					attr->value);
			else
				sdp_len += snprintf(sdp + sdp_len,
					sdp_max_len - sdp_len,
					"%c=%s\r\n",
					SDP_TYPE_ATTRIBUTE,
					attr->key);
		}
	}

	/* Media */
	struct sdp_media *media = NULL;
	list_walk_entry_forward(&session->medias, media, node) {
		ret = sdp_generate_media(
			media, sdp + sdp_len, sdp_max_len - sdp_len,
			session_level_connection_addr);
		if (ret < 0) {
			SDP_LOGE("sdp_generate_media()"
				" failed (%d)", ret);
			error = 1;
			break;
		} else
			sdp_len += ret;
	}

	if (error) {
		free(sdp);
		SDP_RETURN_VAL_IF_FAILED(0, -EINVAL, NULL);
	}

	return sdp;
}


struct sdp_session *sdp_parse_session_description(
	const char *session_desc)
{
	SDP_RETURN_VAL_IF_FAILED(session_desc != NULL, -EINVAL, NULL);

	int error = 0, ret;
	char *sdp;
	struct sdp_media *media = NULL;
	char *p, type, *value, *temp;

	struct sdp_session *session = sdp_session_new();
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -ENOMEM, NULL);

	sdp = strdup(session_desc);
	if (sdp == NULL) {
		SDP_LOG_ERRNO("allocation failed", -ENOMEM);
		error = 1;
		goto cleanup;
	}

	p = strtok_r(sdp, "\n", &temp);
	while (p) {
		/* each line should be more than 2 chars long and in the form
		 * "<type>=<value>" with <type> being a single char */
		if ((strlen(p) <= 2) || (p[1] != '=')) {
			p = strtok_r(NULL, "\n", &temp);
			continue;
		}

		/* remove the '\r' before '\n' if present */
		if (p[strlen(p) - 1] == '\r')
			p[strlen(p) - 1] = '\0';

		/* <type>=<value>, value is always at offset 2 */
		type = *p;
		value = p + 2;

		switch (type) {
		case SDP_TYPE_VERSION:
		{
			int version = -1;
			if (sscanf(value, "%d", &version) == SDP_VERSION)
				SDP_LOGD("SDP: version=%d", version);
			if (version != 0) {
				/* SDP version must be 0 (RFC4566) */
				SDP_LOGW("unsupported SDP version (%d)",
					version);
				error = 1;
				goto cleanup;
			}
			break;
		}
		case SDP_TYPE_ORIGIN:
		{
			char *temp2 = NULL;
			char *username = strtok_r(value, " ", &temp2);
			char *sess_id = strtok_r(NULL, " ", &temp2);
			char *sess_version = strtok_r(NULL, " ", &temp2);
			char *nettype = strtok_r(NULL, " ", &temp2);
			if ((!nettype) || (strcmp(nettype, "IN") != 0))	{
				/* network type must be 'IN'
				 * (RFC4566 ch. 5.2) */
				SDP_LOGW("unsupported network type '%s'",
					(nettype) ? nettype : "");
				error = 1;
				goto cleanup;
			}
			char *addrtype = strtok_r(NULL, " ", &temp2);
			if ((!addrtype) || (strcmp(addrtype, "IP4") != 0)) {
				/* only IPv4 is supported */
				SDP_LOGW("unsupported address type '%s'",
					(addrtype) ? addrtype : "");
				error = 1;
				goto cleanup;
			}
			char *unicast_address = strtok_r(NULL, " ", &temp2);
			if (unicast_address)
				session->server_addr = strdup(unicast_address);
			session->session_id = (sess_id) ? atoll(sess_id) : 0;
			session->session_version = (sess_version) ?
				atoll(sess_version) : 0;
			SDP_LOGD("SDP: username=%s sess_id=%"PRIu64
				" sess_version=%"PRIu64" nettype=%s"
				" addrtype=%s unicast_address=%s",
				username, session->session_id,
				session->session_version, nettype,
				addrtype, unicast_address);
			break;
		}
		case SDP_TYPE_SESSION_NAME:
		{
			session->session_name = strdup(value);
			SDP_LOGD("SDP: session name=%s", session->session_name);
			break;
		}
		case SDP_TYPE_INFORMATION:
		{
			if (media) {
				media->media_title = strdup(value);
				SDP_LOGD("SDP: media title=%s",
					media->media_title);
			} else {
				session->session_info = strdup(value);
				SDP_LOGD("SDP: session info=%s",
					session->session_info);
			}
			break;
		}
		case SDP_TYPE_URI:
		{
			session->uri = strdup(value);
			SDP_LOGD("SDP: uri=%s", session->uri);
			break;
		}
		case SDP_TYPE_EMAIL:
		{
			session->email = strdup(value);
			SDP_LOGD("SDP: email=%s", session->email);
			break;
		}
		case SDP_TYPE_PHONE:
		{
			session->phone = strdup(value);
			SDP_LOGD("SDP: phone=%s", session->phone);
			break;
		}
		case SDP_TYPE_CONNECTION:
		{
			char *temp2 = NULL;
			char *nettype = strtok_r(value, " ", &temp2);
			if ((!nettype) || (strcmp(nettype, "IN") != 0)) {
				/* network type must be 'IN'
				 * (RFC4566 ch. 5.7) */
				SDP_LOGW("unsupported network type '%s'",
					(nettype) ? nettype : "");
				error = 1;
				goto cleanup;
			}
			char *addrtype = strtok_r(NULL, " ", &temp2);
			if ((!addrtype) || (strcmp(addrtype, "IP4") != 0)) {
				/* only IPv4 is supported */
				SDP_LOGW("unsupported address type '%s'",
					(addrtype) ? addrtype : "");
				error = 1;
				goto cleanup;
			}
			char *connection_address = strtok_r(NULL, " ", &temp2);
			if (!connection_address)
				continue;
			int addr_first = atoi(connection_address);
			int multicast =
				((addr_first >= SDP_MULTICAST_ADDR_MIN) &&
				(addr_first <= SDP_MULTICAST_ADDR_MAX)) ?
				1 : 0;
			if (multicast) {
				char *p2 = strchr(connection_address, '/');
				if (p2 != NULL)
					*p2 = '\0';
			}
			if (media) {
				media->connection_addr =
					strdup(connection_address);
				media->multicast = multicast;
				SDP_LOGD("SDP: media nettype=%s addrtype=%s"
					" connection_address=%s",
					nettype, addrtype, connection_address);
			} else {
				session->connection_addr =
					strdup(connection_address);
				session->multicast = multicast;
				SDP_LOGD("SDP: nettype=%s addrtype=%s"
					" connection_address=%s",
					nettype, addrtype, connection_address);
			}
			break;
		}
		case SDP_TYPE_TIME:
		{
			char *temp2 = NULL;
			char *start_time = NULL;
			start_time = strtok_r(value, " ", &temp2);
			uint64_t start_time_int =
				(start_time) ? atoll(start_time) : 0;
			char *stop_time = NULL;
			stop_time = strtok_r(NULL, " ", &temp2);
			uint64_t stop_time_int =
				(stop_time) ? atoll(stop_time) : 0;
			SDP_LOGD("SDP: start_time=%"PRIu64" stop_time=%"PRIu64,
				start_time_int, stop_time_int);
			/*TODO*/
			break;
		}
		case SDP_TYPE_MEDIA:
		{
			media = sdp_session_add_media(session);
			if (media == NULL) {
				SDP_LOGW("sdp_session_add_media() failed");
				error = 1;
				goto cleanup;
			}
			ret = sdp_parse_media(media, value);
			if (ret < 0) {
				SDP_LOGW("sdp_parse_media()"
					" failed (%d)", ret);
				error = 1;
				goto cleanup;
			}
			break;
		}
		case SDP_TYPE_ATTRIBUTE:
		{
			struct sdp_attr *attr = NULL;
			if (media) {
				attr = sdp_media_add_attr(media);
				if (attr == NULL) {
					SDP_LOGW("sdp_media_add_attr() failed");
					error = 1;
					goto cleanup;
				}
			} else {
				attr = sdp_session_add_attr(session);
				if (attr == NULL) {
					SDP_LOGW("sdp_session_add_attr()"
						" failed");
					error = 1;
					goto cleanup;
				}
			}

			ret = sdp_parse_attr(attr, session, media, value);
			if (ret < 0) {
				SDP_LOGW("sdp_parse_attr() failed (%d)", ret);
				error = 1;
				goto cleanup;
			}
			break;
		}
		default:
			break;
		}
		p = strtok_r(NULL, "\n", &temp);
	}

	/* copy session-level parameters to media-level if undefined */
	list_walk_entry_forward(&session->medias, media, node) {
		if ((!media->connection_addr) && (session->connection_addr)) {
			media->connection_addr =
				strdup(session->connection_addr);
			media->multicast = session->multicast;
		}
		if (media->start_mode == SDP_START_MODE_UNSPECIFIED)
			media->start_mode = session->start_mode;
		if ((!media->rtcp_xr.valid) && (session->rtcp_xr.valid))
			media->rtcp_xr = session->rtcp_xr;
	}

cleanup:
	free(sdp);

	if (error) {
		free(session);
		return NULL;
	} else
		return session;
}
