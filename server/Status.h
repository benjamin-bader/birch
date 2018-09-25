// birch - IRC daemon, built with Bazel
//
// Copyright (C) 2018 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef BIRCH_STATUS_H
#define BIRCH_STATUS_H

#include <cstdint>

namespace birch {

enum class ResponseStatus : uint16_t
{
    RPL_WELCOME = 1,
    RPL_YOURHOST = 2,
    RPL_CREATED = 3,
    RPL_MYINFO = 4,
    RPL_BOUNCE = 5,
    

    // 2XX tracing responses
    RPL_TRACELINK = 200,
    RPL_TRACECONNECTING = 201,
    RPL_TRACEHANDSHAKE = 202,
    RPL_TRACEUNKNOWN = 203,
    RPL_TRACEOPERATOR = 204,
    RPL_TRACEUSER = 205,
    RPL_TRACESERVER = 206,
    RPL_TRACESERVICE = 207,
    RPL_TRACENEWTYPE = 208,
    RPL_TRACECLASS = 209,
    RPL_TRACERECONNECT = 210, // unused?
    RPL_TRACELOG = 261,
    RPL_TRACEEND = 262,

    // 2XX stats responses
    RPL_STATSLINKINFO = 211,
    RPL_STATSCOMMANDS = 212,
    RPL_ENDOFSTATS = 219,
    RPL_STATSUPTIME = 242,
    RPL_STATSOLINE = 243,

    // 2XX usermode responses
    RPL_UMODEIS = 221,

    // 2XX LUSER responses
    RPL_SERVLIST = 234,
    RPL_SERVLISTEND = 235,
    RPL_LUSERCLIENT = 251,
    RPL_LUSEROP = 252,
    RPL_LUSERUNKNOWN = 253,
    RPL_LUSERCHANNELS = 254,
    RPL_USERNAME = 255,

    // 2XX ADMIN responses
    RPL_ADMINME = 256,
    RPL_ADMINLOC1 = 257,
    RPL_ADMINLOC2 = 258,
    RPL_ADMINEMAIL = 259,

    // 2XX Backoff response
    RPL_TRYAGAIN = 263,

    // 4XX Errors
    ERR_NOSUCHNICK = 401,
    ERR_NOSUCHSERVER = 402,
    ERR_NOSUCHCHANNEL = 403,
    ERR_CANNOTSENDTOCHAN = 404,
    ERR_TOOMANYCHANNELS = 405,
    ERR_WASNOSUCHNICK = 406,
    ERR_TOOMANYTARGETS = 407,
    ERR_NOSUCHSERVICE = 408,
    ERR_NOORIGIN = 409,

    // 4XX Ping/Pong errors
    ERR_NORECIPIENT = 411,
    ERR_NOTEXTTOSEND = 412,
    ERR_NOTOPLEVEL = 413,
    ERR_WILDTOPLEVEL = 414,
    ERR_BADMASK = 415,


};

}

#endif // BIRCH_STATUS_H