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

	return session;
}


int sdp_session_destroy(
	struct sdp_session *session)
{
	SDP_RETURN_ERR_IF_FAILED(session != NULL, -EINVAL);

	/* Remove all attibutes */
	struct sdp_attr *attr, *next_a;
	for (attr = session->attr, next_a = NULL; attr; attr = next_a) {
		next_a = attr->next;
		int ret = sdp_session_remove_attr(session, attr);
		if (ret != 0)
			SDP_LOGE("sdp_session_remove_attr() failed: %d(%s)",
				ret, strerror(-ret));
	}

	/* Remove all media */
	struct sdp_media *media, *next_m;
	for (media = session->media, next_m = NULL; media; media = next_m) {
		next_m = media->next;
		int ret = sdp_session_remove_media(session, media);
		if (ret != 0)
			SDP_LOGE("sdp_session_remove_media() failed: %d(%s)",
				ret, strerror(-ret));
	}

	free(session->serverAddr);
	free(session->sessionName);
	free(session->sessionInfo);
	free(session->uri);
	free(session->email);
	free(session->phone);
	free(session->type);
	free(session->charset);
	free(session->connectionAddr);
	free(session->controlUrl);
	free(session);

	return 0;
}


struct sdp_attr *sdp_session_add_attr(
	struct sdp_session *session)
{
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -EINVAL, NULL);

	struct sdp_attr *attr = calloc(1, sizeof(*attr));
	SDP_RETURN_VAL_IF_FAILED(attr != NULL, -ENOMEM, NULL);

	/* Add to the chained list */
	attr->prev = NULL;
	attr->next = session->attr;
	if (session->attr)
		session->attr->prev = attr;
	session->attr = attr;
	session->attrCount++;

	return attr;
}


int sdp_session_remove_attr(
	struct sdp_session *session,
	struct sdp_attr *attr)
{
	SDP_RETURN_ERR_IF_FAILED(session != NULL, -EINVAL);
	SDP_RETURN_ERR_IF_FAILED(attr != NULL, -EINVAL);

	int found = 0;
	struct sdp_attr *_attr;
	for (_attr = session->attr; _attr; _attr = _attr->next) {
		if (_attr == attr) {
			found = 1;
			break;
		}
	}

	if (!found) {
		SDP_LOGE("failed to find the attribute in the list");
		return -EINVAL;
	}

	/* Remove from the chained list */
	if (attr->next)
		attr->next->prev = attr->prev;
	if (attr->prev)
		attr->prev->next = attr->next;
	if (session->attr == attr)
		session->attr = attr->next;
	session->attrCount--;

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

	/* Add to the chained list */
	media->prev = NULL;
	media->next = session->media;
	if (session->media)
		session->media->prev = media;
	session->media = media;
	session->mediaCount++;

	return media;
}


int sdp_session_remove_media(
	struct sdp_session *session,
	struct sdp_media *media)
{
	SDP_RETURN_ERR_IF_FAILED(session != NULL, -EINVAL);
	SDP_RETURN_ERR_IF_FAILED(media != NULL, -EINVAL);

	int found = 0;
	struct sdp_media *_media;
	for (_media = session->media; _media; _media = _media->next) {
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
	struct sdp_attr *attr, *next;
	for (attr = media->attr, next = NULL; attr; attr = next) {
		next = attr->next;
		int ret = sdp_media_remove_attr(media, attr);
		if (ret != 0)
			SDP_LOGE("sdp_media_remove_attr() failed: %d(%s)",
				ret, strerror(-ret));
	}

	/* Remove from the chained list */
	if (media->next)
		media->next->prev = media->prev;
	if (media->prev)
		media->prev->next = media->next;
	if (session->media == media)
		session->media = media->next;
	session->mediaCount--;

	free(media->mediaTitle);
	free(media->connectionAddr);
	free(media->controlUrl);
	free(media->encodingName);
	free(media->encodingParams);
	free(media);

	return 0;
}


struct sdp_attr *sdp_media_add_attr(
	struct sdp_media *media)
{
	SDP_RETURN_VAL_IF_FAILED(media != NULL, -EINVAL, NULL);

	struct sdp_attr *attr = calloc(1, sizeof(*attr));
	SDP_RETURN_VAL_IF_FAILED(attr != NULL, -ENOMEM, NULL);

	/* Add to the chained list */
	attr->prev = NULL;
	attr->next = media->attr;
	if (media->attr)
		media->attr->prev = attr;
	media->attr = attr;
	media->attrCount++;

	return attr;
}


int sdp_media_remove_attr(
	struct sdp_media *media,
	struct sdp_attr *attr)
{
	SDP_RETURN_ERR_IF_FAILED(media != NULL, -EINVAL);
	SDP_RETURN_ERR_IF_FAILED(attr != NULL, -EINVAL);

	int found = 0;
	struct sdp_attr *_attr;
	for (_attr = media->attr; _attr; _attr = _attr->next) {
		if (_attr == attr) {
			found = 1;
			break;
		}
	}

	if (!found) {
		SDP_LOGE("failed to find the attribute in the list");
		return -EINVAL;
	}

	/* Remove from the chained list */
	if (attr->next)
		attr->next->prev = attr->prev;
	if (attr->prev)
		attr->prev->next = attr->next;
	if (media->attr == attr)
		media->attr = attr->next;
	media->attrCount--;

	free(attr);

	return 0;
}



static int sdp_generate_rtcp_xr_attribute(
	const struct sdp_rtcp_xr *xr,
	char *sdp,
	int sdpMaxLen)
{
	char xrFormat[100];
	int xrFormatLen = 0, isFirst = 1, sdpLen = 0;
	if (xr->lossRleReport) {
		if (xr->lossRleReportMaxSize > 0)
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s=%d", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_LOSS_RLE,
				xr->lossRleReportMaxSize);
		else
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_LOSS_RLE);
		isFirst = 0;
	}
	if (xr->dupRleReport) {
		if (xr->dupRleReportMaxSize > 0)
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s=%d", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_DUP_RLE,
				xr->dupRleReportMaxSize);
		else
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_DUP_RLE);
		isFirst = 0;
	}
	if (xr->pktReceiptTimesReport) {
		if (xr->pktReceiptTimesReportMaxSize > 0)
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s=%d", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCPT_TIMES,
				xr->pktReceiptTimesReportMaxSize);
		else
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCPT_TIMES);
		isFirst = 0;
	}
	if ((xr->rttReport > SDP_RTCP_XR_RTT_REPORT_NONE) &&
		(xr->rttReport <= SDP_RTCP_XR_RTT_REPORT_SENDER)) {
		if (xr->lossRleReportMaxSize > 0)
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s=%s:%d", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCVR_RTT,
				sdp_rtcp_xr_rtt_report_mode_str[
					xr->rttReport],
				xr->lossRleReportMaxSize);
		else
			xrFormatLen += snprintf(xrFormat + xrFormatLen,
				sizeof(xrFormat) - xrFormatLen,
				"%s%s=%s", (isFirst) ? "" : " ",
				SDP_ATTR_RTCP_XR_RCVR_RTT,
				sdp_rtcp_xr_rtt_report_mode_str[
					xr->rttReport]);
		isFirst = 0;
	}
	char statFlag[100];
	int statFlagLen = 0, isFirst2 = 1;
	if (xr->statsSummaryReportLoss) {
		statFlagLen += snprintf(statFlag + statFlagLen,
			sizeof(statFlag) - statFlagLen,
			"%s%s", (isFirst2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_LOSS);
		isFirst2 = 0;
	}
	if (xr->statsSummaryReportDup) {
		statFlagLen += snprintf(statFlag + statFlagLen,
			sizeof(statFlag) - statFlagLen,
			"%s%s", (isFirst2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_DUP);
		isFirst2 = 0;
	}
	if (xr->statsSummaryReportJitter) {
		statFlagLen += snprintf(statFlag + statFlagLen,
			sizeof(statFlag) - statFlagLen,
			"%s%s", (isFirst2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_JITT);
		isFirst2 = 0;
	}
	if (xr->statsSummaryReportTtl) {
		statFlagLen += snprintf(statFlag + statFlagLen,
			sizeof(statFlag) - statFlagLen,
			"%s%s", (isFirst2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_TTL);
		isFirst2 = 0;
	}
	if (xr->statsSummaryReportHl) {
		statFlagLen += snprintf(statFlag + statFlagLen,
			sizeof(statFlag) - statFlagLen,
			"%s%s", (isFirst2) ? "" : ",",
			SDP_ATTR_RTCP_XR_STAT_HL);
		isFirst2 = 0;
	}
	if (statFlagLen > 0) {
		xrFormatLen += snprintf(xrFormat + xrFormatLen,
			sizeof(xrFormat) - xrFormatLen,
			"%s%s", (isFirst) ? "" : " ",
			SDP_ATTR_RTCP_XR_STAT_SUMMARY);
		isFirst = 0;
	}
	if (xr->djbMetricsReport) {
		xrFormatLen += snprintf(xrFormat + xrFormatLen,
			sizeof(xrFormat) - xrFormatLen,
			"%s%s", (isFirst) ? "" : " ",
			SDP_ATTR_RTCP_XR_DJB_METRICS);
		isFirst = 0;
	}
	if (xrFormatLen > 0) {
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_RTCP_XR,
			xrFormat);
	}

	return sdpLen;
}


static int sdp_parse_rtcp_xr_attribute(
	struct sdp_rtcp_xr *xr,
	char *attrValue)
{
	int ret = 0;
	char *temp1 = NULL;
	char *xr_format = NULL;

	xr_format = strtok_r(attrValue, " ", &temp1);
	while (xr_format) {
		if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_LOSS_RLE,
			strlen(SDP_ATTR_RTCP_XR_LOSS_RLE))) {
			xr->lossRleReport = 1;
			char *p2 = strchr(xr_format, '=');
			if (p2 != NULL)
				xr->lossRleReportMaxSize = atoi(p2 + 1);
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_DUP_RLE,
			strlen(SDP_ATTR_RTCP_XR_DUP_RLE))) {
			xr->dupRleReport = 1;
			char *p2 = strchr(xr_format, '=');
			if (p2 != NULL)
				xr->dupRleReportMaxSize = atoi(p2 + 1);
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_RCPT_TIMES,
			strlen(SDP_ATTR_RTCP_XR_RCPT_TIMES))) {
			xr->pktReceiptTimesReport = 1;
			char *p2 = strchr(xr_format, '=');
			if (p2 != NULL)
				xr->pktReceiptTimesReportMaxSize = atoi(p2 + 1);
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_RCVR_RTT,
			strlen(SDP_ATTR_RTCP_XR_RCVR_RTT))) {
			char *p2 = strchr(xr_format, '=');
			if ((p2 != NULL) && (!strncmp(p2 + 1,
				sdp_rtcp_xr_rtt_report_mode_str[1],
				strlen(sdp_rtcp_xr_rtt_report_mode_str[1]))))
				xr->rttReport = SDP_RTCP_XR_RTT_REPORT_ALL;
			else if ((p2 != NULL) && (!strncmp(p2 + 1,
				sdp_rtcp_xr_rtt_report_mode_str[1],
				strlen(sdp_rtcp_xr_rtt_report_mode_str[1]))))
				xr->rttReport = SDP_RTCP_XR_RTT_REPORT_SENDER;
			else
				xr->rttReport = SDP_RTCP_XR_RTT_REPORT_NONE;
			p2 = strchr(xr_format, ':');
			if (p2 != NULL)
				xr->rttReportMaxSize = atoi(p2 + 1);
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
					xr->statsSummaryReportLoss = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_DUP,
					strlen(SDP_ATTR_RTCP_XR_STAT_DUP)))
					xr->statsSummaryReportDup = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_JITT,
					strlen(SDP_ATTR_RTCP_XR_STAT_JITT)))
					xr->statsSummaryReportJitter = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_TTL,
					strlen(SDP_ATTR_RTCP_XR_STAT_TTL)))
					xr->statsSummaryReportTtl = 1;
				else if (!strncmp(stat_flag,
					SDP_ATTR_RTCP_XR_STAT_HL,
					strlen(SDP_ATTR_RTCP_XR_STAT_HL)))
					xr->statsSummaryReportHl = 1;
				stat_flag = strtok_r(NULL, ",", &temp2);
			}
		} else if (!strncmp(xr_format, SDP_ATTR_RTCP_XR_DJB_METRICS,
			strlen(SDP_ATTR_RTCP_XR_DJB_METRICS)))
			xr->djbMetricsReport = 1;

		xr_format = strtok_r(NULL, " ", &temp1);
	};

	return ret;
}


static int sdp_generate_media_description(
	const struct sdp_media *media,
	char *sdp,
	int sdpMaxLen,
	int sessionLevelConnectionAddr)
{
	int sdpLen = 0, ret, error = 0;

	if (((!media->connectionAddr) || (!strlen(media->connectionAddr))) &&
		(!sessionLevelConnectionAddr)) {
		SDP_LOGE("invalid connection address");
		return -1;
	}
	if ((!media->dstStreamPort) || (!media->dstControlPort)) {
		SDP_LOGE("invalid port");
		return -1;
	}
	if (!media->payloadType) {
		SDP_LOGE("invalid payload type");
		return -1;
	}
	if ((media->type < 0) || (media->type >= SDP_MEDIA_TYPE_MAX)) {
		SDP_LOGE("invalid media type");
		return -1;
	}
	if ((!media->encodingName) || (!strlen(media->encodingName))) {
		SDP_LOGE("invalid encoding name");
		return -1;
	}

	/* Media Description (m=<media> <port> <proto> <fmt> ...) */
	sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
		"%c=%s %d " SDP_PROTO_RTPAVP " %d\r\n",
		SDP_TYPE_MEDIA,
		sdp_media_type_str[media->type],
		media->dstStreamPort,
		media->payloadType);

	/* Media Title (i=<media title>) */
	if ((media->mediaTitle) && (strlen(media->mediaTitle)))
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=%s\r\n",
			SDP_TYPE_INFORMATION,
			media->mediaTitle);

	/* Connection Data (c=<nettype> <addrtype> <connection-address>) */
	if ((media->connectionAddr) && (strlen(media->connectionAddr))) {
		int isMulticast = 0;
		int addrFirst = atoi(media->connectionAddr);
		if ((addrFirst >= SDP_MULTICAST_ADDR_MIN) &&
			(addrFirst <= SDP_MULTICAST_ADDR_MAX))
			isMulticast = 1;
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=IN IP4 %s%s\r\n",
			SDP_TYPE_CONNECTION,
			media->connectionAddr,
			(isMulticast) ? "/127" : "");
	}

	/* Start mode */
	if ((media->startMode > SDP_START_MODE_UNSPECIFIED) &&
		(media->startMode < SDP_START_MODE_MAX)) {
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			sdp_start_mode_str[media->startMode]);
	}

	/* Control URL for use with RTSP */
	if ((media->controlUrl) && (strlen(media->controlUrl))) {
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=%s:%s\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_CONTROL_URL,
			media->controlUrl);
	}

	/* RTP/AVP rtpmap attribute */
	sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
		"%c=%s:%d %s/%d%s%s\r\n",
		SDP_TYPE_ATTRIBUTE,
		SDP_ATTR_RTPAVP_RTPMAP,
		media->payloadType,
		media->encodingName,
		media->clockRate,
		((media->encodingParams) && (strlen(media->encodingParams))) ?
			"/" : "",
		((media->encodingParams) && (strlen(media->encodingParams))) ?
			media->encodingParams : "");

	/* RTCP destination port (if not RTP port + 1) */
	if (media->dstControlPort != media->dstStreamPort + 1)
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=%s:%d\r\n",
			SDP_TYPE_ATTRIBUTE,
			SDP_ATTR_RTCP_PORT,
			media->dstControlPort);

	/* RTCP extended reports attribute */
	ret = sdp_generate_rtcp_xr_attribute(&media->rtcpXr,
		sdp + sdpLen, sdpMaxLen - sdpLen);
	if (ret < 0) {
		SDP_LOGE("sdp_generate_rtcp_xr_attribute()"
			" failed (%d)", ret);
		error = 1;
	}

	/* Other attributes (a=<attribute>:<value> or a=<attribute>) */
	struct sdp_attr *attr;
	for (attr = media->attr; attr; attr = attr->next) {
		if ((attr->key) && (strlen(attr->key))) {
			if ((attr->value) && (strlen(attr->value)))
				sdpLen += snprintf(sdp + sdpLen,
					sdpMaxLen - sdpLen, "%c=%s:%s\r\n",
					SDP_TYPE_ATTRIBUTE,
					attr->key,
					attr->value);
			else
				sdpLen += snprintf(sdp + sdpLen,
					sdpMaxLen - sdpLen, "%c=%s\r\n",
					SDP_TYPE_ATTRIBUTE,
					attr->key);
		}
	}

	return (error) ? -error : sdpLen;
}


char *sdp_generate_session_description(
	const struct sdp_session *session,
	int deletion)
{
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -EINVAL, NULL);

	if ((!session->serverAddr) ||
		(!strlen(session->serverAddr))) {
		SDP_LOGE("invalid server address");
		SDP_RETURN_VAL_IF_FAILED(0, -EINVAL, NULL);
	}

	int error = 0, ret;
	int sdpMaxLen = 1024, sdpLen = 0;
	int sessionLevelConnectionAddr = 0;

	char *sdp = malloc(sdpMaxLen);
	SDP_RETURN_VAL_IF_FAILED(sdp != NULL, -ENOMEM, NULL);

	if (deletion) {
		/* Origin (o=<username> <sess-id> <sess-version>
		 * <nettype> <addrtype> <unicast-address>) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=- %"PRIu64" %"PRIu64" IN IP4 %s\r\n",
			SDP_TYPE_ORIGIN,
			session->sessionId,
			session->sessionVersion,
			session->serverAddr);
	} else {
		/* Protocol Version (v=0) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=%d\r\n",
			SDP_TYPE_VERSION,
			SDP_VERSION);

		/* Origin (o=<username> <sess-id> <sess-version>
		 * <nettype> <addrtype> <unicast-address>) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=- %"PRIu64" %"PRIu64" IN IP4 %s\r\n",
			SDP_TYPE_ORIGIN,
			session->sessionId,
			session->sessionVersion,
			session->serverAddr);

		/* Session Name (s=<session name>) */
		if ((session->sessionName) &&
			(strlen(session->sessionName)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s\r\n",
				SDP_TYPE_SESSION_NAME,
				session->sessionName);
		else
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c= \r\n",
				SDP_TYPE_SESSION_NAME);

		/* Session Information (i=<session description>) */
		if ((session->sessionInfo) &&
			(strlen(session->sessionInfo)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s\r\n",
				SDP_TYPE_INFORMATION,
				session->sessionInfo);

		/* URI (u=<uri>) */
		if ((session->uri) &&
			(strlen(session->uri)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s\r\n",
				SDP_TYPE_URI,
				session->uri);

		/* Email Address (e=<email-address>) */
		if ((session->email) &&
			(strlen(session->email)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s\r\n",
				SDP_TYPE_EMAIL,
				session->email);

		/* Phone Number (p=<phone-number>) */
		if ((session->phone) &&
			(strlen(session->phone)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s\r\n",
				SDP_TYPE_PHONE,
				session->phone);

		/* Connection Data (c=<nettype> <addrtype>
		 * <connection-address>) */
		if ((session->connectionAddr) &&
			(strlen(session->connectionAddr))) {
			sessionLevelConnectionAddr = 1;
			int isMulticast = 0;
			int addrFirst = atoi(session->connectionAddr);
			if ((addrFirst >= SDP_MULTICAST_ADDR_MIN) &&
				(addrFirst <= SDP_MULTICAST_ADDR_MAX))
				isMulticast = 1;
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=IN IP4 %s%s\r\n",
				SDP_TYPE_CONNECTION,
				session->connectionAddr,
				(isMulticast) ? "/127" : "");
		}

		/* Start mode */
		if ((session->startMode > SDP_START_MODE_UNSPECIFIED) &&
			(session->startMode < SDP_START_MODE_MAX)) {
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s\r\n",
				SDP_TYPE_ATTRIBUTE,
				sdp_start_mode_str[session->startMode]);
		}

		/* Session type */
		if ((session->type) && (strlen(session->type))) {
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s:%s\r\n",
				SDP_TYPE_ATTRIBUTE,
				SDP_ATTR_TYPE,
				session->type);
		}

		/* Charset */
		if ((session->charset) && (strlen(session->charset))) {
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s:%s\r\n",
				SDP_TYPE_ATTRIBUTE,
				SDP_ATTR_CHARSET,
				session->charset);
		}

		/* Control URL for use with RTSP */
		if ((session->controlUrl) && (strlen(session->controlUrl))) {
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"%c=%s:%s\r\n",
				SDP_TYPE_ATTRIBUTE,
				SDP_ATTR_CONTROL_URL,
				session->controlUrl);
		}

		/* RTCP extended reports attribute */
		ret = sdp_generate_rtcp_xr_attribute(&session->rtcpXr,
			sdp + sdpLen, sdpMaxLen - sdpLen);
		if (ret < 0) {
			SDP_LOGE("sdp_generate_rtcp_xr_attribute()"
				" failed (%d)", ret);
			error = 1;
		} else
			sdpLen += ret;

		/* Other attributes (a=<attribute>:<value> or a=<attribute>) */
		struct sdp_attr *attr;
		for (attr = session->attr; attr; attr = attr->next) {
			if ((attr->key) && (strlen(attr->key))) {
				if ((attr->value) && (strlen(attr->value)))
					sdpLen += snprintf(sdp + sdpLen,
						sdpMaxLen - sdpLen,
						"%c=%s:%s\r\n",
						SDP_TYPE_ATTRIBUTE,
						attr->key,
						attr->value);
				else
					sdpLen += snprintf(sdp + sdpLen,
						sdpMaxLen - sdpLen,
						"%c=%s\r\n",
						SDP_TYPE_ATTRIBUTE,
						attr->key);
			}
		}

		/* Timing (t=<start-time> <stop-time>) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"%c=0 0\r\n",
			SDP_TYPE_TIME);

		/* Media */
		struct sdp_media *media;
		for (media = session->media; media; media = media->next) {
			ret = sdp_generate_media_description(
				media, sdp + sdpLen, sdpMaxLen - sdpLen,
				sessionLevelConnectionAddr);
			if (ret < 0) {
				SDP_LOGE("sdp_generate_media_description()"
					" failed (%d)", ret);
				error = 1;
				break;
			} else
				sdpLen += ret;
		}
	}

	if (error) {
		free(sdp);
		SDP_RETURN_VAL_IF_FAILED(0, -EINVAL, NULL);
	} else
		return sdp;
}


struct sdp_session *sdp_parse_session_description(
	const char *sessionDescription)
{
	SDP_RETURN_VAL_IF_FAILED(sessionDescription != NULL, -EINVAL, NULL);

	int error = 0, ret;
	char *sdp;
	struct sdp_media *media = NULL;
	char *p, type, *value, *temp;

	struct sdp_session *session = sdp_session_new();
	SDP_RETURN_VAL_IF_FAILED(session != NULL, -ENOMEM, NULL);

	sdp = strdup(sessionDescription);
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
				session->serverAddr = strdup(unicast_address);
			session->sessionId = (sess_id) ? atoll(sess_id) : 0;
			session->sessionVersion = (sess_version) ?
				atoll(sess_version) : 0;
			SDP_LOGD("SDP: username=%s sess_id=%"PRIu64
				" sess_version=%"PRIu64" nettype=%s"
				" addrtype=%s unicast_address=%s",
				username, session->sessionId,
				session->sessionVersion, nettype,
				addrtype, unicast_address);
			break;
		}
		case SDP_TYPE_SESSION_NAME:
		{
			session->sessionName = strdup(value);
			SDP_LOGD("SDP: session name=%s", session->sessionName);
			break;
		}
		case SDP_TYPE_INFORMATION:
		{
			if (media) {
				media->mediaTitle = strdup(value);
				SDP_LOGD("SDP: media title=%s",
					media->mediaTitle);
			} else {
				session->sessionInfo = strdup(value);
				SDP_LOGD("SDP: session info=%s",
					session->sessionInfo);
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
			int addrFirst = atoi(connection_address);
			int isMulticast =
				((addrFirst >= SDP_MULTICAST_ADDR_MIN) &&
				(addrFirst <= SDP_MULTICAST_ADDR_MAX)) ?
				1 : 0;
			if (isMulticast) {
				char *p2 = strchr(connection_address, '/');
				if (p2 != NULL)
					*p2 = '\0';
			}
			if (media) {
				media->connectionAddr =
					strdup(connection_address);
				media->isMulticast = isMulticast;
				SDP_LOGD("SDP: media nettype=%s addrtype=%s"
					" connection_address=%s",
					nettype, addrtype, connection_address);
			} else {
				session->connectionAddr =
					strdup(connection_address);
				session->isMulticast = isMulticast;
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
			char *temp2 = NULL;
			char *smedia = strtok_r(value, " ", &temp2);
			char *port = strtok_r(NULL, " ", &temp2);
			char *proto = strtok_r(NULL, " ", &temp2);
			char *fmt = strtok_r(NULL, " ", &temp2);
			if (smedia) {
				int i;
				for (i = 0; i < SDP_MEDIA_TYPE_MAX; i++) {
					if (!strcmp(smedia,
						sdp_media_type_str[i]) != 0) {
						media->type = i;
						break;
					}
				}
				if (i == SDP_MEDIA_TYPE_MAX) {
					SDP_LOGW("unsupported media type '%s'",
						smedia);
					error = 1;
					goto cleanup;
				}
			} else {
				SDP_LOGW("null media type");
				error = 1;
				goto cleanup;
			}
			int port_int = (port) ? atoi(port) : 0;
			if (port_int) {
				media->dstStreamPort = port_int;
				media->dstControlPort = port_int + 1;
			}
			if ((!proto) || (strncmp(proto, SDP_PROTO_RTPAVP,
				strlen(SDP_PROTO_RTPAVP)) != 0)) {
				SDP_LOGW("unsupported protocol '%s'",
					(proto) ? proto : "");
				error = 1;
				goto cleanup;
			}
			media->payloadType = (fmt) ? atoi(fmt) : 0;
			if ((media->payloadType <
				SDP_DYNAMIC_PAYLOAD_TYPE_MIN) ||
				(media->payloadType >
				SDP_DYNAMIC_PAYLOAD_TYPE_MAX)) {
				/* payload type must be dynamic
				 * (RFC3551 ch. 6) */
				SDP_LOGW("unsupported payload type (%d)",
					media->payloadType);
				error = 1;
				goto cleanup;
			}
			SDP_LOGD("SDP: media=%s port=%d proto=%s"
				" payload_type=%d", smedia, port_int,
				proto, media->payloadType);
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

			char *temp2 = NULL;
			char *attr_key = strtok_r(value, ":", &temp2);
			char *attr_value = strtok_r(NULL, "\n", &temp2);
			attr->key = strdup(attr_key);
			if (attr_value)
				attr->value = strdup(attr_value);

			if ((!strncmp(attr_key, SDP_ATTR_RTPAVP_RTPMAP,
				strlen(SDP_ATTR_RTPAVP_RTPMAP))) &&
				(attr_value)) {
				if (!media) {
					SDP_LOGW("attribute 'rtpmap' not"
						" on media level");
					error = 1;
					goto cleanup;
				}
				char *temp3 = NULL;
				char *payload_type = NULL;
				char *encoding_name = NULL;
				char *clock_rate = NULL;
				char *encoding_params = NULL;
				payload_type =
					strtok_r(attr_value, " ", &temp3);
				unsigned int payload_type_int =
					(payload_type) ? atoi(payload_type) : 0;
				encoding_name = strtok_r(NULL, "/", &temp3);
				clock_rate = strtok_r(NULL, "/", &temp3);
				unsigned int i_clock_rate =
					(clock_rate) ? atoi(clock_rate) : 0;
				encoding_params = strtok_r(NULL, "/", &temp3);
				if (payload_type_int != media->payloadType) {
					SDP_LOGW("invalid payload type"
						" (%d vs. %d)",
						payload_type_int,
						media->payloadType);
					error = 1;
					goto cleanup;
				}
				if (!encoding_name) {
					SDP_LOGW("invalid encoding name");
					error = 1;
					goto cleanup;
				}
				if ((!strncmp(encoding_name, SDP_ENCODING_H264,
					strlen(SDP_ENCODING_H264))) &&
					(i_clock_rate != SDP_H264_CLOCKRATE)) {
					/* clock rate must be 90000 for H.264
					 * (RFC6184 ch. 8.2.1) */
					SDP_LOGW("unsupported clock rate %d",
						i_clock_rate);
					error = 1;
					goto cleanup;
				}
				media->encodingName = strdup(encoding_name);
				if (encoding_params)
					media->encodingParams =
						strdup(encoding_params);
				media->clockRate = i_clock_rate;
				SDP_LOGD("SDP: payload_type=%d"
					" encoding_name=%s clock_rate=%d"
					" encoding_params=%s",
					payload_type_int, encoding_name,
					i_clock_rate, encoding_params);
			} else if ((!strncmp(attr_key, SDP_ATTR_TYPE,
				strlen(SDP_ATTR_TYPE))) &&
				(attr_value)) {
				if (media) {
					SDP_LOGW("attribute 'type' not"
						" on session level");
				} else {
					session->type =
						strdup(attr_value);
				}
			} else if ((!strncmp(attr_key, SDP_ATTR_CHARSET,
				strlen(SDP_ATTR_CHARSET))) &&
				(attr_value)) {
				if (media) {
					SDP_LOGW("attribute 'charset' not"
						" on session level");
				} else {
					session->charset =
						strdup(attr_value);
				}
			} else if ((!strncmp(attr_key, SDP_ATTR_CONTROL_URL,
				strlen(SDP_ATTR_CONTROL_URL))) &&
				(attr_value)) {
				if (media) {
					media->controlUrl =
						strdup(attr_value);
				} else {
					session->controlUrl =
						strdup(attr_value);
				}
			} else if (!strncmp(attr_key, SDP_ATTR_RECVONLY,
				strlen(SDP_ATTR_RECVONLY))) {
				if (media) {
					media->startMode =
						SDP_START_MODE_RECVONLY;
				} else {
					session->startMode =
						SDP_START_MODE_RECVONLY;
				}
			} else if (!strncmp(attr_key, SDP_ATTR_SENDRECV,
				strlen(SDP_ATTR_SENDRECV))) {
				if (media) {
					media->startMode =
						SDP_START_MODE_SENDRECV;
				} else {
					session->startMode =
						SDP_START_MODE_SENDRECV;
				}
			} else if (!strncmp(attr_key, SDP_ATTR_SENDONLY,
				strlen(SDP_ATTR_SENDONLY))) {
				if (media) {
					media->startMode =
						SDP_START_MODE_SENDONLY;
				} else {
					session->startMode =
						SDP_START_MODE_SENDONLY;
				}
			} else if (!strncmp(attr_key, SDP_ATTR_INACTIVE,
				strlen(SDP_ATTR_INACTIVE))) {
				if (media) {
					media->startMode =
						SDP_START_MODE_INACTIVE;
				} else {
					session->startMode =
						SDP_START_MODE_INACTIVE;
				}
			} else if ((!strncmp(attr_key, SDP_ATTR_RTCP_XR,
				strlen(SDP_ATTR_RTCP_XR))) && (attr_value)) {
				if (media)
					ret = sdp_parse_rtcp_xr_attribute(
						&media->rtcpXr, attr_value);
				else
					ret = sdp_parse_rtcp_xr_attribute(
						&session->rtcpXr, attr_value);
				if (ret < 0) {
					SDP_LOGW("sdp_parse_rtcp_xr_attribute()"
						" failed (%d)", ret);
					error = 1;
					goto cleanup;
				}
			} else if ((!strncmp(attr_key, SDP_ATTR_RTCP_PORT,
				strlen(SDP_ATTR_RTCP_PORT))) && (attr_value)) {
				if (!media) {
					SDP_LOGW("attribute 'rtcp' not"
						" on media level");
					error = 1;
					goto cleanup;
				}
				int port = atoi(attr_value);
				if (port > 0) {
					media->dstControlPort = port;
					SDP_LOGD("SDP: rtcp_dst_port=%d", port);
				}
			}
			break;
		}
		default:
			break;
		}
		p = strtok_r(NULL, "\n", &temp);
	}

cleanup:
	free(sdp);

	if (error) {
		free(session);
		return NULL;
	} else
		return session;
}
