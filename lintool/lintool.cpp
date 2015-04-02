/*
 * Copyright (c) 2012-2015, Mike Smith, <msmith@purgatory.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
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
#include <unistd.h>
#include <err.h>
#include <getopt.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <lin_defs.h>

#include "link.h"
#include "log.h"
#include "param.h"
#include "firmware.h"
#include "node.h"

void usage();

void
status(int argc, char *argv[])
{
    Link::print_status();
}

void
history(int argc, char *argv[])
{
    Log::acquire();
    Log::print();
}

void
trace(int argc, char *argv[])
{
    std::vector<unsigned> filter;

    argv++;
    argc--;

    while (argc--) {
        unsigned fid = strtoul(*(argv++), nullptr, 0);
        filter.push_back(fid);
    }

    Log::trace(filter);
}

void
scan(int argc, char *argv[])
{
    unsigned node = Node::kNoNode;
    int ch;

    while ((ch = getopt(argc, argv, "n:")) != -1) {
        switch (ch) {
        case 'n':
            node = strtoul(optarg, nullptr, 0);
            break;

        default:
            warnx("ERROR: unrecognised option '-%c'", ch);
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    Node::scan(node);

    for (auto n : Node::nodes()) {
        auto pset = n->params();
        char *str = pset.identity();

        printf("%s\n", str);
        free(str);
    }

    Log::print();
}

void
dump_params(int argc, char *argv[])
{
    unsigned node = Node::kNoNode;
    int ch;

    while ((ch = getopt(argc, argv, "n:")) != -1) {
        switch (ch) {
        case 'n':
            node = strtoul(optarg, nullptr, 0);
            break;

        default:
            warnx("ERROR: unrecognised option '-%c'", ch);
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    Node::scan(node);

    if (Node::exists(Bootloader::kNodeAddress) && (node == Node::kNoNode)) {
        errx(1, "ERROR: cannot save all parameters, there is a node that needs recovery");
    }

    const char *pfile = (argc > 0) ? argv[0] : "/dev/stdout";
    ParamDB pdb;

    for (auto n : Node::nodes()) {
        auto pset = n->params();
        pset.sync();
        pdb.store(pset);
    }

    pdb.write(pfile);
}

void
load_params(int argc, char *argv[])
{
    unsigned node = Node::kNoNode;
    bool defaults = false;
    int ch;

    while ((ch = getopt(argc, argv, "dn:")) != -1) {
        switch (ch) {
        case 'd':
            defaults = true;
            break;

        case 'n':
            node = strtoul(optarg, nullptr, 0);
            break;

        default:
            warnx("ERROR: unrecognised option '-%c'", ch);
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    Node::scan(node);

    if (defaults) {
        for (auto n : Node::nodes()) {
            n->set_defaults();
        }
        return;
    }

    if (Node::exists(Bootloader::kNodeAddress) && (node == Node::kNoNode)) {
        errx(1, "ERROR: cannot load all parameters, there is a node that needs recovery");
    }

    const char *pfile = (argc > 0) ? argv[0] : "/dev/stdin";
    ParamDB pdb;

    try {
        pdb.read(pfile);

    } catch (std::runtime_error &e) {
        errx(1, "ERROR: reading %s: %s", pfile, e.what());
    }

    // iterate nodes within the database
    for (auto dbnode : pdb.nodes()) {

        // extract node address & function
        unsigned nodeAddress = dbnode["node"].ToInt();
        unsigned nodeFunction = dbnode["function"].ToInt();

        // look for a matching node in the scan results
        auto node = Node::matching(nodeAddress, nodeFunction);

        // if we didn't find a node present...
        if (node == nullptr) {
            if (Node::exists(nodeAddress)) {
                warnx("WARNING: node at %u does not match function %u.", nodeAddress, nodeFunction);

            } else {
                warnx("WARNING: node at %u is not responding.", nodeAddress);
            }

            warnx("WARNING: Skipping parameter load for node at %u.", nodeAddress);
            continue;
        }

        // update node parameter set from database
        auto pset = node->params();

        for (auto dbparam : dbnode["parameters"].ToArray()) {
            try {
                pset.set(dbparam.ToObject());

            } catch (Exception &e) {
                warnx("WARNING: %s.", e.what());
            }
        }

        pset.sync();
    }
}

void
edit_param(int argc, char *argv[])
{
    unsigned node = Node::kNoNode;
    bool show_readonly = false;
    bool want_info = false;
    bool set_default = false;
    int ch;

    while ((ch = getopt(argc, argv, "ain:")) != -1) {
        switch (ch) {
        case 'a':
            show_readonly = true;
            break;

        case 'i':
            want_info = true;
            break;

        case 'n':
            node = strtoul(optarg, nullptr, 0);
            break;

        default:
            warnx("ERROR: unrecognised option '-%c'", ch);
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    const char *newvalue = nullptr;
    const char *paramname = nullptr;

    switch (argc) {
    case 2:
        if (!strcmp(argv[1], "-d")) {
            set_default = true;
        } else {
            newvalue = argv[1];
        }

    // FALLTHROUGH
    case 1:
        paramname = argv[0];

    // FALLTHROUGH
    case 0:
        break;

    default:
        warnx("ERROR: too many arguments");
        usage();
    }

    if (node == Node::kNoNode) {
        errx(1, "missing node address");
    }

    Node::scan(node);

    auto np = Node::nodes().front();
    auto pset = np->params();

    for (auto param : pset.list()) {
        if ((paramname != nullptr) && (strcmp(param->name(), paramname))) {
            continue;
        }

        param->sync();
        auto encoding = param->encoding();

        if (newvalue == nullptr) {
            if ((paramname != nullptr) || show_readonly || param->is_settable()) {
                printf("%s %u 0x%x", param->name(), param->get(), param->get());

                auto encoding_name = Encoding::name(encoding);

                if (encoding_name != nullptr) {
                    printf(" %s", encoding_name);

                    auto info = Encoding::info(encoding, param->get());

                    if (info != nullptr) {
                        printf(" %s", info);
                    }
                }

                printf("\n");
            }

            if (want_info && param->is_settable()) {
                auto defval = param->get_default();

                for (uint16_t value = 0; value < 0xffff; value++) {
                    auto info = Encoding::info(encoding, value);

                    if (info != nullptr) {
                        printf("        %.5u / 0x%04x / %s%s\n", value, value, info,
                            (value == defval) ? " (default)" : "");
                    }
                }
            }

            continue;
        }

        if (!param->is_settable()) {
            errx(1, "%s cannot be set", param->name());
        }

        uint16_t value;

        if (set_default) {
            value = param->get_default();
        } else {
            if (!Encoding::value(encoding, newvalue, value)) {
                char *cp;
                value = strtoul(newvalue, &cp, 0);

                if (*cp != '\0') {
                    errx(1, "bad parameter value '%s'", newvalue);
                }
            }
        }

        param->set(value);
        param->sync();

        // parameter names are unique, so if we have set a parameter we are done here
        return;
    }

    // failing to find a parameter name when attempting to set a parameter is fatal
    if ((paramname != nullptr) && (newvalue != nullptr)) {
        char *str = pset.identity();

        errx(1, "%s parameter '%s' does not exist", str, paramname);
        free(str);
    }
}

void
update(int argc, char *argv[])
{
    unsigned node = Node::kNoNode;
    bool verify = false;
    bool save_params = false;
    int ch;

    while ((ch = getopt(argc, argv, "n:pv")) != -1) {
        switch (ch) {
        case 'n':
            node = strtoul(optarg, nullptr, 0);

            if (node == 0) {
                errx(1, "bad node address '%s'", optarg);
            }

            break;

        case 'p':
            save_params = true;
            break;

        case 'v':
            verify = true;
            break;

        default:
            warnx("ERROR: unrecognised option '-%c'", ch);
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    Node::scan(node);

    for (unsigned arg = 0; arg < argc; arg++) {
        try {
            new Firmware(argv[arg]);

        } catch (std::runtime_error &e) {
            errx(1, "ERROR: loading firmware from %s: %s", argv[arg], e.what());
        }
    }

    for (auto n : Node::nodes()) {
        try {
            n->update(verify, save_params);

        } catch (Exception &e) {
            warnx("WARNING: failed updating node@%u: %s", n->address(), e.what());
        }
    }
}

struct {
    const char  *cmd;
    const char *help;
    void (*func)(int argc, char *argv[]);
} commands[] = {
    {
        "status",
        "lintool status [-q]\n"
        "    Print link status.\n",
        status
    },
    {
        "history",
        "lintool history\n"
        "    Dump recent history from link.\n",
        history
    },
    {
        "trace",
        "lintool trace [<fid>...]\n"
        "    Trace LIN traffic (^C to exit).\n"
        "    Multiple <fid> parameters identifying frames by ID may be\n"
        "    supplied, in which case only matching frames will be displayed.\n",
        trace
    },
    {
        "scan",
        "lintool [-l] scan [-n <node>]\n"
        "    Scan for a specific node or list all nodes.\n",
        scan
    },
    {
        "dump_params",
        "lintool [-l] dump_params [-n <node>] [<file>]\n"
        "    Dump parameters to file (or stdout if <file> not specified).\n",
        dump_params
    },
    {
        "load_params",
        "lintool [-l] load_params [-n <node>] [-d|<file>]\n"
        "    Load node parameters.\n"
        "        -d    If specified, node parameters will be reset to defaults.\n"
        "    If neither -d nor <file> are specified, parameters will be read from stdin.\n"
        "    Data should be in the same format as emitted by dump_params. Nodes not defined\n"
        "    in the data will not be updated.\n",
        load_params
    },
    {
        "param",
        "lintool [-l] param -n <node> [-a][-i] [<param_name> [-d|<value>]]\n"
        "    Read or write or more parameters from <node>.\n"
        "        -a    When printing all node parameters, include read-only parameters.\n"
        "        -i    When printing a parameter that accepts named values, also print\n"
        "              a list of the allowed named values and the default value.\n"
        "        -d    Print the default value for the parameter.\n"
        "    If <param> and <value> are supplied, sets the named parameter to the given value.\n"
        "    The <value> parameter may either be a number, or a named value.\n"
        "    If only <param> is supplied, prints the value of the named parameter.\n"
        "    If neither are supplied, all parameters from <node> are printed.\n"
        "    Parameters are printed in the format <param> <value> [<encoding> <info>].\n",
        edit_param
    },
    {
        "update",
        "lintool [-l] update [-v][-p][-n <node>] <file> [<file> ...]\n"
        "    Update node firmware for one or more nodes from one or more firmware files.\n"
        "        -p    Preserve node configuration across update. Normally all parameters\n"
        "              are reset to defaults by the update.\n"
        "        -v    Perform read-after-write verification (slow).\n"
        "    Only nodes for which firmware is loaded can be updated. If -n is not specified,\n"
        "    all nodes will be updated.\n"
        "    To update a node in recovery mode that has lost its type, pass -n 32 and supply\n"
        "    only one firmware file. The node will take the identity specified in the file.\n",
        update
    }
};

void
usage()
{
    fprintf(stderr, "\nCommon options:\n");
    fprintf(stderr, "        -l  enable logging (specify twice to print empty frames).\n\n");

    for (auto cmd : commands) {
        fprintf(stderr, "%s\n", cmd.help);
    }

    exit(1);
}

int
main(int argc, char *argv[])
{
    int ch;

    while ((ch = getopt(argc, argv, "l")) != -1) {
        switch (ch) {
        case 'l':
            Log::enable++;
            break;

        default:
            warnx("ERROR: unrecognised option '-%c'", ch);

        // FALLTHROUGH
        case '?':
            usage();
        }
    }

    argc -= optind;
    argv += optind;
    optreset = 1;
    optind = 1;

    if (argc < 1) {
        usage();
    }

    try {
        Link::connect();

    } catch (Exception &e) {
        errx(1, "connection failed: %s", e.what());
    }

    for (auto cmd : commands) {
        if (!strcmp(cmd.cmd, argv[0])) {
            try {
                cmd.func(argc, argv);

            } catch (Exception &e) {
                errx(1, "%s failed: %s", cmd.cmd, e.what());
                Log::print();
            }

            Log::print();
            exit(0);
        }
    }

    warnx("ERROR: unrecognised command '%s'", argv[0]);
    usage();
}

