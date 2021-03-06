#include "genetlink.h"

#include <errno.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include "nat64/common/types.h"
#include "types.h"

static struct nl_sock *sk;
static int family;

struct response_handler_arg {
	jool_response_cb cb;
	void *arg;
	int jool_error;
};

static int set_socket_timeouts(void)
{
	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	if (setsockopt(nl_socket_get_fd(sk), SOL_SOCKET, SO_RCVTIMEO,
			(char *)&timeout, sizeof(timeout)) < 0) {
		perror("setsockopt(SO_RCVTIMEO) failed");
		return errno;
	}

	if (setsockopt(nl_socket_get_fd(sk), SOL_SOCKET, SO_SNDTIMEO,
			(char *)&timeout, sizeof(timeout)) < 0) {
		perror("setsockopt(SO_SNDTIMEO) failed");
		return errno;
	}

	return 0;
}

int nlsocket_init(char *family_name)
{
	int error;

	sk = nl_socket_alloc();
	if (!sk) {
		log_err("Could not allocate the socket to kernelspace.");
		log_err("(I guess we're out of memory.)");
		return -1;
	}

	/*
	 * We handle ACKs ourselves. The reason is that Netlink ACK errors do
	 * not contain the friendly error string, so they're useless to us.
	 * https://github.com/NICMx/Jool/issues/169
	 */
	nl_socket_disable_auto_ack(sk);

	error = genl_connect(sk);
	if (error) {
		log_err("Could not open the socket to kernelspace.");
		goto genl_fail;
	}

	error = set_socket_timeouts();
	if (error) {
		nl_socket_free(sk);
		return error;
	}

	family = genl_ctrl_resolve(sk, family_name);
	if (family < 0) {
		log_err("%s's socket family doesn't seem to exist.", family_name);
		log_err("(This probably means %s hasn't been modprobed.)", family_name);
		error = family;
		goto genl_fail;
	}

	return 0;

genl_fail:
	nl_socket_free(sk);
	return netlink_print_error(error);
}

void nlsocket_destroy()
{
	nl_socket_free(sk);
}

int nlsocket_create_msg(int cmd, struct nl_msg **result)
{
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg) {
		log_err("Out of memory!");
		return -ENOMEM;
	}

	if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family, 0, 0, cmd, 1)) {
		log_err("Unknown error building the packet to the kernel.");
		nlmsg_free(msg);
		return -EINVAL;
	}

	*result = msg;
	return 0;
}

/*
 * Heads up:
 * Netlink wants this function to return either a negative error code or an enum
 * nl_cb_action.
 * Because NL_SKIP == EPERM and NL_STOP == ENOENT, you should double-check
 * the sign of every return.
 */
static int response_handler(struct nl_msg *msg, void *arg_void)
{
	struct nlattr *attrs[__ATTR_MAX + 1];
	struct response_handler_arg *arg = arg_void;
	int error;

	error = genlmsg_parse(nlmsg_hdr(msg), 0, attrs, __ATTR_MAX, NULL);
	if (error)
		return -abs(error);

	/*
	 * From now on, all returns should be zero because the packet was
	 * delivered as far as Netlink is concerned.
	 */

	if (!attrs[ATTR_ERROR_CODE]) {
		log_err("Jool's response lacks a success/error code.");
		arg->jool_error = -EINVAL;
		return 0;
	}

	error = nla_get_u16(attrs[ATTR_ERROR_CODE]);
	arg->jool_error = error;
	if (error) {
		if (attrs[ATTR_ERROR_STRING]) {
			log_err("The module threw error %d: %s", error,
					nla_get_string(attrs[ATTR_ERROR_STRING]));
		} else {
			log_err("The module's response contains error %d.", error);
			log_err("(Sorry; the response lacks an error message.)");
		}
		return 0;
	}

	if (arg->cb)
		arg->jool_error = arg->cb(attrs, arg->arg);
	return 0;
}

int nlsocket_send(struct nl_msg *msg, jool_response_cb cb, void *cb_arg)
{
	struct response_handler_arg arg = {
			.cb = cb,
			.arg = cb_arg,
			.jool_error = 0,
	};
	int error;

	error = nl_socket_modify_cb(sk, NL_CB_MSG_IN, NL_CB_CUSTOM,
			response_handler, &arg);
	if (error < 0) {
		log_err("Could not register response handler.");
		log_err("I will not be able to parse the module's response, so I won't send the request.");
		return netlink_print_error(error);
	}

	error = nl_send_auto(sk, msg);
	if (error < 0) {
		log_err("Could not dispatch the request to kernelspace.");
		return netlink_print_error(error);
	}

	error = nl_recvmsgs_default(sk);
	if (error < 0) {
		log_err("Error receiving the kernel module's response.");
		return netlink_print_error(error);
	}

	return arg.jool_error;
}

int netlink_print_error(int error)
{
	log_err("Netlink error %d: %s", error, nl_geterror(error));
	return error;
}
