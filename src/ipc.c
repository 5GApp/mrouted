/*
 * Copyright (c) 2018-2019 Joachim Nilsson <troglobit@gmail.com>
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "defs.h"

static struct sockaddr_un sun;
static int ipc_socket = -1;
static int detail = 0;

static int ipc_write(int sd, struct ipc *msg)
{
	ssize_t len;

	while ((len = write(sd, msg, sizeof(*msg)))) {
		if (-1 == len && EINTR == errno)
			continue;
		break;
	}
		
	if (len != sizeof(*msg))
		return -1;

	return 0;
}

static int ipc_close(int sd, struct ipc *msg)
{
	msg->cmd = IPC_EOF_CMD;
	if (ipc_write(sd, msg)) {
		logit(LOG_WARNING, errno, "Failed sending EOF/ACK to client");
		return -1;
	}

	return 0;
}

static int ipc_send(int sd, struct ipc *msg, FILE *fp)
{
	msg->cmd = IPC_OK_CMD;
	while (fgets(msg->buf, sizeof(msg->buf), fp)) {
		if (!ipc_write(sd, msg))
			continue;

		logit(LOG_WARNING, errno, "Failed communicating with client");
		return -1;
	}

	return ipc_close(sd, msg);
}

static void show_dump(FILE *fp)
{
	dump_vifs(fp);
	dump_routes(fp);
	dump_cache(fp);
}

static void show_igmp_groups(FILE *fp)
{
	struct listaddr *group, *source;
	struct uvif *uv;
	vifi_t vifi;

	fprintf(fp, "Interface         Group            Source           Last Reported    Timeout=\n");
	for (vifi = 0, uv = uvifs; vifi < numvifs; vifi++, uv++) {
		for (group = uv->uv_groups; group; group = group->al_next) {
			char pre[40], post[40];

			snprintf(pre, sizeof(pre), "%-16s  %-15s  ",
				 uv->uv_name, inet_fmt(group->al_addr, s1, sizeof(s1)));

			snprintf(post, sizeof(post), "%-15s  %7u",
				 inet_fmt(group->al_reporter, s1, sizeof(s1)),
				 group->al_timer);

//			if (!group->al_sources) {
				fprintf(fp, "%s%-15s  %s\n", pre, "ANY", post);
//				continue;
//			}

//			for (source = group->al_sources; source; source = source->al_next)
//				fprintf(fp, "%s%-15s  %s\n",
//					pre, inet_fmt(source->al_addr, s1, sizeof(s1)), post);
		}
	}
}

static const char *ifstate(struct uvif *uv)
{
	if (uv->uv_flags & VIFF_DOWN)
		return "Down";

	if (uv->uv_flags & VIFF_DISABLED)
		return "Disabled";

	return "Up";
}

static void show_igmp_iface(FILE *fp)
{
	struct listaddr *group;
	struct uvif *uv;
	vifi_t vifi;

	fprintf(fp, "Interface         State     Querier          Timeout Version  Groups=\n");

	for (vifi = 0, uv = uvifs; vifi < numvifs; vifi++, uv++) {
		size_t num = 0;
		char timeout[10];
		int version;

		if (!uv->uv_querier) {
			strlcpy(s1, "Local", sizeof(s1));
			snprintf(timeout, sizeof(timeout), "None");
		} else {
			inet_fmt(uv->uv_querier->al_addr, s1, sizeof(s1));
			snprintf(timeout, sizeof(timeout), "%u", IGMP_QUERY_INTERVAL - uv->uv_querier->al_timer);
		}

		for (group = uv->uv_groups; group; group = group->al_next)
			num++;

		if (uv->uv_flags & VIFF_IGMPV1)
			version = 1;
//		else if (uv->uv_flags & VIFF_IGMPV2)
//			version = 2;
		else
			version = 2;

		fprintf(fp, "%-16s  %-8s  %-15s  %7s %7d  %6zd\n", uv->uv_name,
			ifstate(uv), s1, timeout, version, num);
	}
}

static void ipc_show(int sd, struct ipc *msg, void (*cb)(FILE *))
{
	FILE *fp;

	fp = tmpfile();
	if (!fp) {
		logit(LOG_WARNING, errno, "Failed opening temporary file");
		return;
	}

	cb(fp);

	rewind(fp);
	ipc_send(sd, msg, fp);
	fclose(fp);
}

static void ipc_handle(int sd)
{
	socklen_t socklen = 0;
	struct ipc msg;
	ssize_t len;
	int client;

	client = accept(sd, NULL, &socklen);
	if (client < 0)
		return;

	len = read(client, &msg, sizeof(msg));
	if (len < 0) {
		logit(LOG_WARNING, errno, "Failed reading IPC command");
		close(client);
		return;
	}

	switch (msg.cmd) {
	case IPC_SHOW_DUMP_CMD:
		ipc_show(client, &msg, show_dump);
		break;

	case IPC_SHOW_IGMP_CMD:
		ipc_show(client, &msg, show_igmp_groups);
		break;

	case IPC_SHOW_IFACE_CMD:
		ipc_show(client, &msg, show_igmp_iface);
		break;

	default:
		break;
	}

	close(client);
}

void ipc_init(void)
{
	socklen_t len;
	int sd;

	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0) {
		logit(LOG_ERR, errno, "Failed creating IPC socket");
		return;
	}

	if (register_input_handler(sd, ipc_handle) < 0)
		logit(LOG_ERR, 0, "Failed registering IPC handler");

#ifdef HAVE_SOCKADDR_UN_SUN_LEN
	sun.sun_len = 0;	/* <- correct length is set by the OS */
#endif
	sun.sun_family = AF_UNIX;
	strlcpy(sun.sun_path, _PATH_MROUTED_SOCK, sizeof(sun.sun_path));

	unlink(sun.sun_path);
	logit(LOG_DEBUG, 0, "Binding IPC socket to %s", sun.sun_path);

	len = offsetof(struct sockaddr_un, sun_path) + strlen(sun.sun_path);
	if (bind(sd, (struct sockaddr *)&sun, len) < 0 || listen(sd, 1)) {
		logit(LOG_WARNING, errno, "Failed binding IPC socket, client disabled");
		close(sd);
		return;
	}

	ipc_socket = sd;
}

void ipc_exit(void)
{
	if (ipc_socket > -1)
		close(ipc_socket);

	unlink(sun.sun_path);
	ipc_socket = -1;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
