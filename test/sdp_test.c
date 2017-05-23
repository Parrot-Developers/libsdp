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


int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	FILE *fSdp = NULL;
	size_t sdpSize = 0, read = 0;
	char *sdp = NULL;
	struct sdp_session *session = NULL;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		exit(-1);
	}

	fSdp = fopen(argv[1], "r");
	if (fSdp == NULL) {
		fprintf(stderr, "failed to open file '%s'\n", argv[1]);
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	fseek(fSdp, 0, SEEK_END);
	sdpSize = ftell(fSdp);
	sdp = malloc(sdpSize);
	if (sdp == NULL) {
		fprintf(stderr, "allocation failed (size %zi)\n", sdpSize);
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	fseek(fSdp, 0, SEEK_SET);
	read = fread(sdp, sdpSize, 1, fSdp);
	if (read != 1) {
		fprintf(stderr, "failed to read from input file\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	session = sdp_parse_session_description(sdp);
	if (session == NULL) {
		fprintf(stderr, "sdp_parse_session_description() failed\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

cleanup:
	free(sdp);
	if (session)
		sdp_session_destroy(session);
	if (fSdp)
		fclose(fSdp);

	exit(ret);
}
