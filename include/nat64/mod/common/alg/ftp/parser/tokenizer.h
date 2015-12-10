#ifndef _JOOL_MOD_ALG_FTP_TOKENIZER_H
#define _JOOL_MOD_ALG_FTP_TOKENIZER_H

#include "nat64/common/types.h"
#include "nat64/mod/common/alg/ftp/parser/parser.h"

enum ftp_client_code {
	FTP_AUTH,
	FTP_EPSV,
	FTP_EPRT,
	FTP_ALGS,
	/* Something else. */
	FTP_CLIENT_UNRECOGNIZED,
};

/** See http://tools.ietf.org/html/rfc2428#section-3 */
enum epsv_type {
	/** The EPSV command contains no arguments. */
	EPSV_EMPTY,
	/** The EPSV command contains the "net-prt" field. */
	EPSV_CONTAINS_PROTO,
	/** Command is an "EPSV ALL". */
	EPSV_ALL,
};

enum alg_argument {
	ALGS_STATUS64,
	ALGS_ENABLE64,
	ALGS_DISABLE64,
};

struct ftp_client_msg {
	enum ftp_client_code code;

	union {
		struct {
			enum epsv_type type;
			/*
			 * RFC 2448  calls this "net-prt".
			 * This is only relevant if @type is
			 * EPSV_CONTAINS_PROTO.
			 */
			unsigned int proto;
		} epsv;
		struct {
			unsigned int proto;
			/* RFC 2448 calls this "net-addr" and "tcp-port". */
			union {
				struct ipv4_transport_addr addr4;
				struct ipv6_transport_addr addr6;
			};
		} eprt;
		struct {
			enum alg_argument arg;
		} algs;
	};
};

enum ftp_server_code {
	/* Entering passive mode. */
	FTP_227,
	/* 4xx or 5xx. */
	FTP_REJECT,
	/* Something else. */
	FTP_SERVER_UNRECOGNIZED,
};

struct ftp_server_msg {
	enum ftp_server_code code;
	union {
		struct {
			struct ipv4_transport_addr addr;
		} epsv_227;
	};
};

int parser_server_next(struct ftp_parser *parser, struct ftp_server_msg *msg);
int parser_client_next(struct ftp_parser *parser, struct ftp_client_msg *msg);

#endif /* _JOOL_MOD_ALG_FTP_TOKENIZER_H */