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


static void print_h264_fmtp(struct sdp_h264_fmtp *fmtp, const char *prefix)
{
	if (!fmtp)
		return;

	printf("%s-- H.264 format parameters\n", prefix);
	printf("%s   -- packetization mode: %d\n", prefix,
		fmtp->packetization_mode);
	printf("%s   -- profile_idc: %d\n", prefix,
		fmtp->profile_idc);
	printf("%s   -- profile-iop: 0x%02X\n", prefix,
		fmtp->profile_iop);
	printf("%s   -- level_idc: %d\n", prefix,
		fmtp->level_idc);
	if (fmtp->sps) {
		char sps[300];
		unsigned int i, len;
		for (i = 0, len = 0; i < fmtp->sps_size; i++) {
			len += snprintf(sps + len, sizeof(sps) - len,
				"%02X ", fmtp->sps[i]);
		}
		printf("%s   -- SPS (size %d): %s\n", prefix,
			fmtp->sps_size, sps);
	}
	if (fmtp->pps) {
		char pps[300];
		unsigned int i, len;
		for (i = 0, len = 0; i < fmtp->pps_size; i++) {
			len += snprintf(pps + len, sizeof(pps) - len,
				"%02X ", fmtp->pps[i]);
		}
		printf("%s   -- PPS (size %d): %s\n", prefix,
			fmtp->pps_size, pps);
	}
}


static void print_rtcp_xr_info(struct sdp_rtcp_xr *xr, const char *prefix)
{
	if (!xr)
		return;

	printf("%s-- RTCP XR\n", prefix);
	printf("%s   -- loss RLE report: %d (%d)\n", prefix,
		xr->loss_rle_report, xr->loss_rle_report_max_size);
	printf("%s   -- duplicate RLE report: %d (%d)\n", prefix,
		xr->dup_rle_report, xr->dup_rle_report_max_size);
	printf("%s   -- packet receipt times report: %d (%d)\n", prefix,
		xr->pkt_receipt_times_report,
		xr->pkt_receipt_times_report_max_size);
	printf("%s   -- receiver reference time report: %d (%d)\n", prefix,
		xr->rtt_report, xr->rtt_report_max_size);
	printf("%s   -- statistics summary report (loss): %d\n", prefix,
		xr->stats_summary_report_loss);
	printf("%s   -- statistics summary report (dup): %d\n", prefix,
		xr->stats_summary_report_dup);
	printf("%s   -- statistics summary report (jitter): %d\n", prefix,
		xr->stats_summary_report_jitter);
	printf("%s   -- statistics summary report (ttl): %d\n", prefix,
		xr->stats_summary_report_ttl);
	printf("%s   -- statistics summary report (hl): %d\n", prefix,
		xr->stats_summary_report_hl);
	printf("%s   -- VOIP metrics report: %d\n", prefix,
		xr->voip_metrics_report);
	printf("%s   -- de-jitter buffer metrics report: %d\n", prefix,
		xr->djb_metrics_report);
}


static void print_media_info(struct sdp_media *media)
{
	if (!media)
		return;

	printf("-- Media\n");
	printf("   -- type: %s\n",
		(media->type < SDP_MEDIA_TYPE_MAX) ?
		sdp_media_type_str[media->type] : "unknown");
	printf("   -- media title: %s\n", media->media_title);
	printf("   -- connection address: %s%s\n", media->connection_addr,
		(media->multicast) ? " (multicast)" : "");
	printf("   -- control URL: %s\n", media->control_url);
	printf("   -- start mode: %s\n",
		(media->start_mode < SDP_START_MODE_MAX) ?
		sdp_start_mode_str[media->start_mode] : "unknown");
	printf("   -- stream port: %d\n", media->dst_stream_port);
	printf("   -- control port: %d\n", media->dst_control_port);
	printf("   -- payload type: %d\n", media->payload_type);
	printf("   -- encoding name: %s\n", media->encoding_name);
	printf("   -- encoding params: %s\n", media->encoding_params);
	printf("   -- clock rate: %d\n", media->clock_rate);
	if (media->h264_fmtp.valid)
		print_h264_fmtp(&media->h264_fmtp, "   ");
	if (media->rtcp_xr.valid)
		print_rtcp_xr_info(&media->rtcp_xr, "   ");
	struct sdp_attr *attr = NULL;
	list_walk_entry_forward(&media->attrs, attr, node) {
		printf("   -- attribute %s%s%s\n", attr->key,
			(attr->value) ? ": " : "",
			(attr->value) ? attr->value : "");
	}
}


static void print_session_info(struct sdp_session *session)
{
	if (!session)
		return;

	printf("Session\n");
	printf("-- session ID: %" PRIu64 "\n", session->session_id);
	printf("-- session version: %" PRIu64 "\n", session->session_version);
	printf("-- server address: %s\n", session->server_addr);
	printf("-- session name: %s\n", session->session_name);
	printf("-- session info: %s\n", session->session_info);
	printf("-- URI: %s\n", session->uri);
	printf("-- email: %s\n", session->email);
	printf("-- phone: %s\n", session->phone);
	printf("-- tool: %s\n", session->tool);
	printf("-- type: %s\n", session->type);
	printf("-- charset: %s\n", session->charset);
	printf("-- connection address: %s%s\n", session->connection_addr,
		(session->multicast) ? " (multicast)" : "");
	printf("-- control URL: %s\n", session->control_url);
	printf("-- start mode: %s\n",
		(session->start_mode < SDP_START_MODE_MAX) ?
		sdp_start_mode_str[session->start_mode] : "unknown");
	if (session->rtcp_xr.valid)
		print_rtcp_xr_info(&session->rtcp_xr, "");
	struct sdp_attr *attr = NULL;
	list_walk_entry_forward(&session->attrs, attr, node) {
		printf("-- attribute %s%s%s\n", attr->key,
			(attr->value) ? ": " : "",
			(attr->value) ? attr->value : "");
	}
	struct sdp_media *media = NULL;
	list_walk_entry_forward(&session->medias, media, node) {
		print_media_info(media);
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
		session->session_id = 123456789;
		session->session_version = 1;
		session->server_addr = strdup("192.168.43.1");
		session->session_name = strdup("Bebop2");
		session->control_url = strdup("rtsp://192.168.43.1/video");
		session->start_mode = SDP_START_MODE_RECVONLY;
		session->tool = strdup(argv[0]);
		session->type = strdup("broadcast");
		session->rtcp_xr.valid = 1;
		session->rtcp_xr.loss_rle_report = 1;
		session->rtcp_xr.djb_metrics_report = 1;

		media1 = sdp_session_media_add(session);
		if (media1 == NULL) {
			fprintf(stderr, "failed to create media #1\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		media1->type = SDP_MEDIA_TYPE_VIDEO;
		media1->media_title = strdup("Front camera");
		media1->connection_addr = strdup("239.255.42.1");
		media1->dst_stream_port = 55004;
		media1->dst_control_port = 55005;
		media1->control_url = strdup("stream=0");
		media1->payload_type = 96;
		media1->encoding_name = strdup("H264");
		media1->clock_rate = 90000;
		media1->h264_fmtp.valid = 1;
		media1->h264_fmtp.packetization_mode = 1;
		media1->h264_fmtp.profile_idc = 66;
		media1->h264_fmtp.profile_iop = 0;
		media1->h264_fmtp.level_idc = 41;
		media1->h264_fmtp.sps = malloc(sizeof(sps));
		if (media1->h264_fmtp.sps) {
			memcpy(media1->h264_fmtp.sps, sps, sizeof(sps));
			media1->h264_fmtp.sps_size = sizeof(sps);
		}
		media1->h264_fmtp.pps = malloc(sizeof(pps));
		if (media1->h264_fmtp.pps) {
			memcpy(media1->h264_fmtp.pps, pps, sizeof(pps));
			media1->h264_fmtp.pps_size = sizeof(pps);
		}

		media2 = sdp_session_media_add(session);
		if (media2 == NULL) {
			fprintf(stderr, "failed to create media #2\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		media2->type = SDP_MEDIA_TYPE_VIDEO;
		media2->media_title = strdup("Vertical camera");
		media2->connection_addr = strdup("239.255.42.1");
		media2->dst_stream_port = 55006;
		media2->dst_control_port = 55007;
		media2->control_url = strdup("stream=1");
		media2->payload_type = 96;
		media2->encoding_name = strdup("H264");
		media2->clock_rate = 90000;
		media2->h264_fmtp.valid = 1;
		media2->h264_fmtp.packetization_mode = 1;
		media2->h264_fmtp.profile_idc = 66;
		media2->h264_fmtp.profile_iop = 0;
		media2->h264_fmtp.level_idc = 41;
		media2->h264_fmtp.sps = malloc(sizeof(sps));
		if (media2->h264_fmtp.sps) {
			memcpy(media2->h264_fmtp.sps, sps, sizeof(sps));
			media2->h264_fmtp.sps_size = sizeof(sps);
		}
		media2->h264_fmtp.pps = malloc(sizeof(pps));
		if (media2->h264_fmtp.pps) {
			memcpy(media2->h264_fmtp.pps, pps, sizeof(pps));
			media2->h264_fmtp.pps_size = sizeof(pps);
		}

		sdp = sdp_description_write(session, 0);
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
		int file_size = ftell(f);
		fseek(f, 0, SEEK_SET);
		sdp = malloc(file_size);
		if (!sdp) {
			fprintf(stderr, "allocation failed (size %d)\n",
				file_size);
			ret = EXIT_FAILURE;
			goto cleanup;
		}
		int err = fread(sdp, file_size, 1, f);
		if (err != 1) {
			fprintf(stderr, "failed to read from the input file\n");
			ret = EXIT_FAILURE;
			goto cleanup;
		}
	}

	session2 = sdp_description_read(sdp);
	if (session2 == NULL) {
		fprintf(stderr, "failed to parse the session description\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	printf("\n");
	print_session_info(session2);

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
