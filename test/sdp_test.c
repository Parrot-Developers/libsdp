/**
 * @file sdp_test.c
 * @brief Session Description Protocol library - test program
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

#include <libsdp.h>


static const char *const sdp_media_type_str[] = {
	"audio",
	"video",
	"text",
	"application",
	"message",
};


static const char *const sdp_start_mode_str[] = {
	"unspecified",
	"recvonly",
	"sendrecv",
	"sendonly",
	"inactive",
};


static void printH264Fmtp(struct sdp_h264_fmtp *fmtp, const char *prefix)
{
	if (!fmtp)
		return;

	printf("%s-- H.264 format parameters\n", prefix);
	printf("%s   -- packetization mode: %d\n", prefix,
		fmtp->packetizationMode);
	printf("%s   -- profile_idc: %d\n", prefix,
		fmtp->profileIdc);
	printf("%s   -- profile-iop: 0x%02X\n", prefix,
		fmtp->profileIop);
	printf("%s   -- level_idc: %d\n", prefix,
		fmtp->levelIdc);
	if (fmtp->sps) {
		char sps[300];
		unsigned int i, len;
		for (i = 0, len = 0; i < fmtp->spsSize; i++) {
			len += snprintf(sps + len, sizeof(sps) - len,
				"%02X ", fmtp->sps[i]);
		}
		printf("%s   -- SPS (size %d): %s\n", prefix,
			fmtp->spsSize, sps);
	}
	if (fmtp->pps) {
		char pps[300];
		unsigned int i, len;
		for (i = 0, len = 0; i < fmtp->ppsSize; i++) {
			len += snprintf(pps + len, sizeof(pps) - len,
				"%02X ", fmtp->pps[i]);
		}
		printf("%s   -- PPS (size %d): %s\n", prefix,
			fmtp->ppsSize, pps);
	}
}


static void printRtcpXrInfo(struct sdp_rtcp_xr *xr, const char *prefix)
{
	if (!xr)
		return;

	printf("%s-- RTCP XR\n", prefix);
	printf("%s   -- loss RLE report: %d (%d)\n", prefix,
		xr->lossRleReport, xr->lossRleReportMaxSize);
	printf("%s   -- duplicate RLE report: %d (%d)\n", prefix,
		xr->dupRleReport, xr->dupRleReportMaxSize);
	printf("%s   -- packet receipt times report: %d (%d)\n", prefix,
		xr->pktReceiptTimesReport, xr->pktReceiptTimesReportMaxSize);
	printf("%s   -- receiver reference time report: %d (%d)\n", prefix,
		xr->rttReport, xr->rttReportMaxSize);
	printf("%s   -- statistics summary report (loss): %d\n", prefix,
		xr->statsSummaryReportLoss);
	printf("%s   -- statistics summary report (dup): %d\n", prefix,
		xr->statsSummaryReportDup);
	printf("%s   -- statistics summary report (jitter): %d\n", prefix,
		xr->statsSummaryReportJitter);
	printf("%s   -- statistics summary report (ttl): %d\n", prefix,
		xr->statsSummaryReportTtl);
	printf("%s   -- statistics summary report (hl): %d\n", prefix,
		xr->statsSummaryReportHl);
	printf("%s   -- de-jitter buffer metrics report: %d\n", prefix,
		xr->djbMetricsReport);
}


static void printMediaInfo(struct sdp_media *media)
{
	if (!media)
		return;

	printf("-- Media\n");
	printf("   -- type: %s\n",
		((media->type >= 0) && (media->type < SDP_MEDIA_TYPE_MAX)) ?
		sdp_media_type_str[media->type] : "unknown");
	printf("   -- media title: %s\n", media->mediaTitle);
	printf("   -- connection address: %s%s\n", media->connectionAddr,
		(media->isMulticast) ? " (multicast)" : "");
	printf("   -- control URL: %s\n", media->controlUrl);
	printf("   -- start mode: %s\n",
		((media->startMode >= 0) &&
		(media->startMode < SDP_START_MODE_MAX)) ?
		sdp_start_mode_str[media->startMode] : "unknown");
	printf("   -- stream port: %d\n", media->dstStreamPort);
	printf("   -- control port: %d\n", media->dstControlPort);
	printf("   -- payload type: %d\n", media->payloadType);
	printf("   -- encoding name: %s\n", media->encodingName);
	printf("   -- encoding params: %s\n", media->encodingParams);
	printf("   -- clock rate: %d\n", media->clockRate);
	if (!strncmp(media->encodingName, "H264", strlen("H264")))
		printH264Fmtp(&media->h264Fmtp, "   ");
	printRtcpXrInfo(&media->rtcpXr, "   ");
	struct sdp_attr *attr = NULL;
	list_walk_entry_forward(&media->attrs, attr, node) {
		printf("   -- attribute %s%s%s\n", attr->key,
			(attr->value) ? ": " : "",
			(attr->value) ? attr->value : "");
	}
}


static void printSessionInfo(struct sdp_session *session)
{
	if (!session)
		return;

	printf("Session\n");
	printf("-- session ID: %" PRIu64 "\n", session->sessionId);
	printf("-- session version: %" PRIu64 "\n", session->sessionVersion);
	printf("-- server address: %s\n", session->serverAddr);
	printf("-- session name: %s\n", session->sessionName);
	printf("-- session info: %s\n", session->sessionInfo);
	printf("-- URI: %s\n", session->uri);
	printf("-- email: %s\n", session->email);
	printf("-- phone: %s\n", session->phone);
	printf("-- tool: %s\n", session->tool);
	printf("-- type: %s\n", session->type);
	printf("-- charset: %s\n", session->charset);
	printf("-- connection address: %s%s\n", session->connectionAddr,
		(session->isMulticast) ? " (multicast)" : "");
	printf("-- control URL: %s\n", session->controlUrl);
	printf("-- start mode: %s\n",
		((session->startMode >= 0) &&
		(session->startMode < SDP_START_MODE_MAX)) ?
		sdp_start_mode_str[session->startMode] : "unknown");
	printRtcpXrInfo(&session->rtcpXr, "");
	struct sdp_attr *attr = NULL;
	list_walk_entry_forward(&session->attrs, attr, node) {
		printf("-- attribute %s%s%s\n", attr->key,
			(attr->value) ? ": " : "",
			(attr->value) ? attr->value : "");
	}
	struct sdp_media *media = NULL;
	list_walk_entry_forward(&session->medias, media, node) {
		printMediaInfo(media);
	}
}


int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	FILE *f = NULL;
	struct sdp_session *session = NULL, *session2 = NULL;
	struct sdp_media *media1 = NULL, *media2 = NULL;
	char *sdp = NULL;
	uint8_t sps[] = {
		0x67, 0x64, 0x00, 0x28, 0xAC, 0xD9,
		0x80, 0x78, 0x06, 0x5B, 0x01, 0x10,
		0x00, 0x00, 0x3E, 0x90, 0x00, 0x0B,
		0xB8, 0x08, 0xF1, 0x83, 0x19, 0xA0
	};
	uint8_t pps[] = {
		0x68, 0xE9, 0x78, 0xF3, 0xC8, 0xF0
	};

	if (argc < 2) {
		session = sdp_session_new();
		if (session == NULL) {
			fprintf(stderr, "failed to create session\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		session->sessionId = 123456789;
		session->sessionVersion = 1;
		session->serverAddr = strdup("192.168.43.1");
		session->sessionName = strdup("Bebop2");
		session->controlUrl = strdup("rtsp://192.168.43.1/video");
		session->startMode = SDP_START_MODE_RECVONLY;
		session->tool = strdup(argv[0]);
		session->type = strdup("broadcast");
		session->rtcpXr.lossRleReport = 1;
		session->rtcpXr.djbMetricsReport = 1;

		media1 = sdp_session_add_media(session);
		if (media1 == NULL) {
			fprintf(stderr, "failed to create media #1\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		media1->type = SDP_MEDIA_TYPE_VIDEO;
		media1->mediaTitle = strdup("Front camera");
		media1->connectionAddr = strdup("239.255.42.1");
		media1->dstStreamPort = 55004;
		media1->dstControlPort = 55005;
		media1->controlUrl = strdup("stream=0");
		media1->payloadType = 96;
		media1->encodingName = strdup("H264");
		media1->clockRate = 90000;
		media1->h264Fmtp.packetizationMode = 1;
		media1->h264Fmtp.profileIdc = 66;
		media1->h264Fmtp.profileIop = 0;
		media1->h264Fmtp.levelIdc = 41;
		media1->h264Fmtp.sps = malloc(sizeof(sps));
		if (media1->h264Fmtp.sps) {
			memcpy(media1->h264Fmtp.sps, sps, sizeof(sps));
			media1->h264Fmtp.spsSize = sizeof(sps);
		}
		media1->h264Fmtp.pps = malloc(sizeof(pps));
		if (media1->h264Fmtp.pps) {
			memcpy(media1->h264Fmtp.pps, pps, sizeof(pps));
			media1->h264Fmtp.ppsSize = sizeof(pps);
		}

		media2 = sdp_session_add_media(session);
		if (media2 == NULL) {
			fprintf(stderr, "failed to create media #2\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		media2->type = SDP_MEDIA_TYPE_VIDEO;
		media2->mediaTitle = strdup("Vertical camera");
		media2->connectionAddr = strdup("239.255.42.1");
		media2->dstStreamPort = 55006;
		media2->dstControlPort = 55007;
		media2->controlUrl = strdup("stream=1");
		media2->payloadType = 96;
		media2->encodingName = strdup("H264");
		media2->clockRate = 90000;
		media2->h264Fmtp.packetizationMode = 1;
		media2->h264Fmtp.profileIdc = 66;
		media2->h264Fmtp.profileIop = 0;
		media2->h264Fmtp.levelIdc = 41;
		media2->h264Fmtp.sps = malloc(sizeof(sps));
		if (media2->h264Fmtp.sps) {
			memcpy(media2->h264Fmtp.sps, sps, sizeof(sps));
			media2->h264Fmtp.spsSize = sizeof(sps);
		}
		media2->h264Fmtp.pps = malloc(sizeof(pps));
		if (media2->h264Fmtp.pps) {
			memcpy(media2->h264Fmtp.pps, pps, sizeof(pps));
			media2->h264Fmtp.ppsSize = sizeof(pps);
		}

		sdp = sdp_generate_session_description(session, 0);
		if (sdp == NULL) {
			fprintf(stderr, "failed to generate"
				" the session desciption\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}

		printf("\n%s\n", sdp);
	} else {
		f = fopen(argv[1], "r");
		if (!f) {
			fprintf(stderr, "failed to open file '%s'\n", argv[1]);
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		fseek(f, 0, SEEK_END);
		int fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);
		sdp = malloc(fileSize);
		if (!sdp) {
			fprintf(stderr, "allocation failed (size %d)\n",
				fileSize);
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		int err = fread(sdp, fileSize, 1, f);
		if (err != 1) {
			fprintf(stderr, "failed to read from the input file\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}
	}

	session2 = sdp_parse_session_description(sdp);
	if (session2 == NULL) {
		fprintf(stderr, "failed to parse the session description\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	printf("\n");
	printSessionInfo(session2);

cleanup:
	free(sdp);
	if (session)
		sdp_session_destroy(session);
	if (session2)
		sdp_session_destroy(session2);
	if (f)
		fclose(f);

	exit(ret);
}
