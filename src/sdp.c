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


#define SDP_ATTR_RTPAVP_RTPMAP "rtpmap"
#define SDP_ATTR_RTCP_PORT "rtcp"


static const char *const sdp_media_type_str[] = {
	"audio"
	"video"
	"text"
	"application"
	"message"
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
	free(session->connectionAddr);
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


static int sdp_generate_media_description(
	const struct sdp_media *media,
	char *sdp,
	int sdpMaxLen,
	int sessionLevelConnectionAddr)
{
	int sdpLen = 0;

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
		"m=%s %d RTP/AVP %d\r\n",
		sdp_media_type_str[media->type],
		media->dstStreamPort,
		media->payloadType);

	/* Media Title (i=<media title>) */
	if ((media->mediaTitle) && (strlen(media->mediaTitle)))
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"i=%s\r\n",
			media->mediaTitle);

	/* Connection Data (c=<nettype> <addrtype> <connection-address>) */
	if ((media->connectionAddr) && (strlen(media->connectionAddr))) {
		int isMulticast = 0;
		int addrFirst = atoi(media->connectionAddr);
		if ((addrFirst >= 224) && (addrFirst <= 239))
			isMulticast = 1;
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"c=IN IP4 %s%s\r\n",
			media->connectionAddr,
			(isMulticast) ? "/127" : "");
	}

	/* Attributes (a=<attribute>:<value> or a=<attribute>) */
	sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
		"a="SDP_ATTR_RTPAVP_RTPMAP":%d %s/%d%s%s\r\n",
		media->payloadType, media->encodingName, media->clockRate,
		((media->encodingParams) && (strlen(media->encodingParams))) ?
			"/" : "",
		((media->encodingParams) && (strlen(media->encodingParams))) ?
			media->encodingParams : "");

	if (media->dstControlPort != media->dstStreamPort + 1)
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"a="SDP_ATTR_RTCP_PORT":%d\r\n",
			media->dstControlPort);

	struct sdp_attr *attr;
	for (attr = media->attr; attr; attr = attr->next) {
		if ((attr->key) && (strlen(attr->key))) {
			if ((attr->value) && (strlen(attr->value)))
				sdpLen += snprintf(sdp + sdpLen,
					sdpMaxLen - sdpLen, "a=%s:%s\r\n",
					attr->key, attr->value);
			else
				sdpLen += snprintf(sdp + sdpLen,
					sdpMaxLen - sdpLen, "a=%s\r\n",
					attr->key);
		}
	}

	return sdpLen;
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

	int error = 0;
	int sdpMaxLen = 1024, sdpLen = 0;
	int sessionLevelConnectionAddr = 0;

	char *sdp = malloc(sdpMaxLen);
	SDP_RETURN_VAL_IF_FAILED(sdp != NULL, -ENOMEM, NULL);

	if (deletion) {
		/* Origin (o=<username> <sess-id> <sess-version>
		 * <nettype> <addrtype> <unicast-address>) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"o=- %"PRIu64" %"PRIu64" IN IP4 %s\r\n",
			session->sessionId,
			session->sessionVersion,
			session->serverAddr);
	} else {
		/* Protocol Version (v=0) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"v=0\r\n");

		/* Origin (o=<username> <sess-id> <sess-version>
		 * <nettype> <addrtype> <unicast-address>) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"o=- %"PRIu64" %"PRIu64" IN IP4 %s\r\n",
			session->sessionId,
			session->sessionVersion,
			session->serverAddr);

		/* Session Name (s=<session name>) */
		if ((session->sessionName) &&
			(strlen(session->sessionName)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"s=%s\r\n", session->sessionName);
		else
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"s= \r\n");

		/* Session Information (i=<session description>) */
		if ((session->sessionInfo) &&
			(strlen(session->sessionInfo)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"i=%s\r\n", session->sessionInfo);

		/* URI (u=<uri>) */
		if ((session->uri) &&
			(strlen(session->uri)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"u=%s\r\n", session->uri);

		/* Email Address (e=<email-address>) */
		if ((session->email) &&
			(strlen(session->email)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"e=%s\r\n", session->email);

		/* Phone Number (p=<phone-number>) */
		if ((session->phone) &&
			(strlen(session->phone)))
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"p=%s\r\n", session->phone);

		/* Connection Data (c=<nettype> <addrtype>
		 * <connection-address>) */
		if ((session->connectionAddr) &&
			(strlen(session->connectionAddr))) {
			sessionLevelConnectionAddr = 1;
			int isMulticast = 0;
			int addrFirst = atoi(session->connectionAddr);
			if ((addrFirst >= 224) && (addrFirst <= 239))
				isMulticast = 1;
			sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
				"c=IN IP4 %s%s\r\n",
				session->connectionAddr,
				(isMulticast) ? "/127" : "");
		}

		/* Attributes (a=<attribute>:<value> or a=<attribute>) */
		struct sdp_attr *attr;
		for (attr = session->attr; attr; attr = attr->next) {
			if ((attr->key) && (strlen(attr->key))) {
				if ((attr->value) && (strlen(attr->value)))
					sdpLen += snprintf(sdp + sdpLen,
						sdpMaxLen - sdpLen,
						"a=%s:%s\r\n",
						attr->key, attr->value);
				else
					sdpLen += snprintf(sdp + sdpLen,
						sdpMaxLen - sdpLen,
						"a=%s\r\n",
						attr->key);
			}
		}

		/* Timing (t=<start-time> <stop-time>) */
		sdpLen += snprintf(sdp + sdpLen, sdpMaxLen - sdpLen,
			"t=0 0\r\n");

		/* Media */
		struct sdp_media *media;
		for (media = session->media; media; media = media->next) {
			int ret = sdp_generate_media_description(
				media, sdp + sdpLen, sdpMaxLen - sdpLen,
				sessionLevelConnectionAddr);
			if (ret < 0) {
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

	int error = 0;
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
		case 'v':
		{
			int version = -1;
			if (sscanf(value, "%d", &version) == 1)
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
		case 'o':
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
		case 's':
		{
			session->sessionName = strdup(value);
			SDP_LOGD("SDP: session name=%s", session->sessionName);
			break;
		}
		case 'i':
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
		case 'u':
		{
			session->uri = strdup(value);
			SDP_LOGD("SDP: uri=%s", session->uri);
			break;
		}
		case 'e':
		{
			session->email = strdup(value);
			SDP_LOGD("SDP: email=%s", session->email);
			break;
		}
		case 'p':
		{
			session->phone = strdup(value);
			SDP_LOGD("SDP: phone=%s", session->phone);
			break;
		}
		case 'c':
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
				((addrFirst >= 224) && (addrFirst <= 239)) ?
				1 : 0;
			if (isMulticast) {
				char *p2 = strchr(connection_address, '/');
				if (p2 != NULL)
					*p2 = '\0';
			}
			if (media) {
				media->connectionAddr =
					strdup(connection_address);
				media->isMulticast = 1;
				SDP_LOGD("SDP: media nettype=%s addrtype=%s"
					" connection_address=%s",
					nettype, addrtype, connection_address);
			} else {
				session->connectionAddr =
					strdup(connection_address);
				session->isMulticast = 1;
				if ((!isMulticast) && (!session->serverAddr))
					session->serverAddr =
						strdup(connection_address);
				else if ((!isMulticast) &&
					(strcmp(session->serverAddr,
						connection_address))) {
					free(session->serverAddr);
					session->serverAddr =
						strdup(connection_address);
				}
				SDP_LOGD("SDP: nettype=%s addrtype=%s"
					" connection_address=%s",
					nettype, addrtype, connection_address);
			}
			break;
		}
		case 't':
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
		case 'm':
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
			if ((!proto) || (strcmp(proto, "RTP/AVP") != 0)) {
				SDP_LOGW("unsupported protocol '%s'",
					(proto) ? proto : "");
				error = 1;
				goto cleanup;
			}
			media->payloadType = (fmt) ? atoi(fmt) : 0;
			if ((media->payloadType < 96) ||
				(media->payloadType > 127)) {
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
		case 'a':
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
			char *attr_value = strtok_r(NULL, ":", &temp2);
			attr->key = strdup(attr_key);
			if (attr_value)
				attr->value = strdup(attr_value);

			if ((!strncmp(attr_key, SDP_ATTR_RTPAVP_RTPMAP, 6)) &&
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
				unsigned int clock_rate_int =
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
				if ((strncmp(encoding_name, "H264", 4)) &&
					(clock_rate_int != 90000)) {
					/* clock rate must be 90000 for H.264
					 * (RFC6184 ch. 8.2.1) */
					SDP_LOGW("unsupported clock rate %d",
						clock_rate_int);
					error = 1;
					goto cleanup;
				}
				media->encodingName = strdup(encoding_name);
				if (encoding_params)
					media->encodingParams =
						strdup(encoding_params);
				media->clockRate = clock_rate_int;
				SDP_LOGD("SDP: payload_type=%d"
					" encoding_name=%s clock_rate=%d"
					" encoding_params=%s",
					payload_type_int, encoding_name,
					clock_rate_int, encoding_params);
			} else if ((!strncmp(attr_key,
				SDP_ATTR_RTCP_PORT, 4)) &&
				(attr_value)) {
				if (!media) {
					SDP_LOGW("attribute 'rtpmap' not"
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
