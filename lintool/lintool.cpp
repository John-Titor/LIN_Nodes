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
#include <iostream>
#include <iomanip>

#include <lin_defs.h>

#include "link.h"
#include "log.h"
#include "param.h"
#include "firmware.h"
#include "node.h"

EXCEPTION(Exception, ExUsageError);
EXCEPTION(Exception, ExArgumentError);
EXCEPTION(Exception, ExFailure);
EXCEPTION(Exception, ExSuccess);

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
            RAISE(ExUsageError, "unrecognised option '-" << (char)ch << "'");
        }
    }

    argc -= optind;
    argv += optind;

    Node::scan(node);

    for (auto n : Node::nodes()) {
        std::cout << n->params().identity().str() << std::endl;
    }
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
            RAISE(ExUsageError, "unrecognised option '-" << (char)ch << "'");
        }
    }

    argc -= optind;
    argv += optind;

    Node::scan(node);

    if (Node::exists(Bootloader::kNodeAddress) && (node == Node::kNoNode)) {
        RAISE(ExFailure, "cannot save all parameters, there is a node that needs recovery");
    }

    const char *pfile = (argc > 0) ? argv[0] : "/dev/stdout";
    ParamDB pdb;

    for (auto n : Node::nodes()) {
        auto &pset = n->params();
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
            RAISE(ExUsageError, "unrecognised option '-" << (char)ch << "'");
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
        RAISE(ExFailure, "cannot load all parameters, there is a node that needs recovery");
    }

    const char *pfile = (argc > 0) ? argv[0] : "/dev/stdin";
    ParamDB pdb;

    try {
        pdb.read(pfile);

    } catch (std::runtime_error &e) {
        RAISE(ExFailure, "reading " << pfile << ": " << e.what());
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
                std::cerr << "WARNING: node at "
                          << nodeAddress
                          << " does not match function "
                          << nodeFunction
                          << "."
                          << std::endl;

            } else {
                std::cerr << "WARNING: node at "
                          << nodeAddress
                          << " is not responding."
                          << std::endl;
            }

            std::cerr << "WARNING: Skipping parameter load for node at "
                      << nodeAddress
                      << "."
                      << std::endl;
            continue;
        }

        // update node parameter set from database
        auto &pset = node->params();

        for (auto dbparam : dbnode["parameters"].ToArray()) {
            pset.set(dbparam.ToObject());
            // allow exceptions to fail the operation
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
            RAISE(ExUsageError, "unrecognised option '-" << (char)ch << "'");
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
        RAISE(ExUsageError, "too many arguments");
    }

    if (node == Node::kNoNode) {
        RAISE(ExUsageError, "missing node address");
    }

    Node::scan(node);

    auto &pset = Node::nodes().front()->params();

    for (auto param : pset.list()) {
        if ((paramname != nullptr) && (strcmp(param->name(), paramname))) {
            continue;
        }

        param->sync();
        auto encoding = param->encoding();

        if (newvalue == nullptr) {
            if ((paramname != nullptr) || show_readonly || param->is_settable()) {
                std::cout << param->name() << "/";
                std::cout << "/" << param->get();

                auto encoding_name = Encoding::name(encoding);

                if (encoding_name != nullptr) {
                    std::cout << "/" << encoding_name;

                    auto info = Encoding::info(encoding, param->get());

                    if (info != nullptr) {
                        std::cout << "/" << info;
                    }
                }

                std::cout << std::endl;
            }

            if (want_info && param->is_settable()) {
                auto defval = param->get_default();

                for (uint16_t value = 0; value < 0xffff; value++) {
                    auto info = Encoding::info(encoding, value);

                    if (info != nullptr) {
                        std::cout << "        "
                                  << std::setw(5)
                                  << value
                                  << "/"
                                  << info;

                        if (value == defval) {
                            std::cout << " (default)";
                        }

                        std::cout << std::endl;
                    }
                }
            }

            continue;
        }

        if (!param->is_settable()) {
            RAISE(ExArgumentError, param->name() << " cannot be set");
        }

        uint16_t value;

        if (set_default) {
            value = param->get_default();

        } else {
            if (!Encoding::value(encoding, newvalue, value)) {
                char *cp;
                value = strtoul(newvalue, &cp, 0);

                if (*cp != '\0') {
                    RAISE(ExArgumentError, "bad parameter value '" << newvalue << "'");
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

        RAISE(ExArgumentError, pset.identity().str() << " parameter '" << paramname << "' does not exist");
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
                RAISE(ExArgumentError, "bad node address '" << optarg << "'");
            }

            break;

        case 'p':
            save_params = true;
            break;

        case 'v':
            verify = true;
            break;

        default:
            RAISE(ExUsageError, "unrecognised option '-" << (char)ch << "'");
        }
    }

    argc -= optind;
    argv += optind;

    Node::scan(node);

    for (unsigned arg = 0; arg < argc; arg++) {
        try {
            new Firmware(argv[arg]);

        } catch (std::runtime_error &e) {
            RAISE(ExFailure, "loading firmware from " << argv[arg] << ": " << e.what());
        }
    }

    for (auto n : Node::nodes()) {
        try {
            n->update(verify, save_params);

        } catch (Exception &e) {
            RAISE(ExFailure, "failed updating node@" << n->address() << ": " << e.what());
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
        "    Scan for a specific node or list all nodes.\n"
        "    Found nodes are printed in the format <node id>/<function id>/<function name>\n",
        scan
    },
    {
        "dump_params",
        "lintool [-l] dump_params [-n <node>] [<file>]\n"
        "    Dump parameters to file (or stdout if <file> not specified).\n"
        "    Parameters are formatted as a JSON array holding one dictionary per node. Each node\n"
        "    dictionary in turn holds four values; 'name', 'node' and 'function' describe\n"
        "    the node, while 'parameters' holds an array of parameter dictionaries. Each dictionary\n"
        "    describes one parameter, with 'name', 'address', 'value' and 'info' properties.\n",
        dump_params
    },
    {
        "load_params",
        "lintool [-l] load_params [-n <node>] [-d|<file>]\n"
        "    Load node parameters.\n"
        "        -d    If specified, node parameters will be reset to defaults.\n"
        "    If neither -d nor <file> are specified, parameters will be read from stdin.\n"
        "    Data should be in the same format as emitted by dump_params. Nodes not defined\n"
        "    in the data will not be updated. Notes defined in the data but not present will\n"
        "    result in a diagnostic message but other nodes will be updated.\n",
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
        "    If neither are supplied, all parameters from <node> are printed.\n\n"
        "    Parameters are printed in the format <name>/<value>[/<encoding name>[/<encoded name>]].\n"
        "    Allowed named values are printed in the format <value>/<encoded name>.\n",
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
        "    only one firmware file. No check that the firmware file corresponds to the node\n"
        "    is or can be performed in this case.\n",
        update
    }
};

void
usage()
{
    std::cerr << "Common options:" << std::endl;
    std::cerr << "        -l  enable logging (specify twice to print empty frames)." << std::endl;
    std::cerr << "        -A  enable automation mode" << std::endl << std::endl;

    for (auto cmd : commands) {
        std::cerr << cmd.help << std::endl;
    }

    exit(1);
}

int
main(int argc, char *argv[])
{
    bool automated = false;

    try {
        int ch;

        while ((ch = getopt(argc, argv, "lA")) != -1) {
            switch (ch) {
            case 'l':
                Log::enable++;
                break;

            case 'A':
                automated = true;
                break;

            default:
                RAISE(ExUsageError, "unrecognised option '-" << (char)ch << "'");

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
                cmd.func(argc, argv);
                RAISE(ExSuccess, "OK");
            }
        }

        RAISE(ExUsageError, "unrecognised command '" << argv[0] << "'");

    } catch (ExUsageError &e) {
        std::cerr << "USAGE: "
                  << e.what()
                  << std::endl;

        if (!automated) {
            usage();
        }

    } catch (ExSuccess &e) {
        Log::print();

        if (automated) {
            std::cerr << "SUCCESS: " << e.what() << std::endl;
        }

        exit(0);

    } catch (Exception &e) {
        Log::print();
        std::cerr << "ERROR: " << e.what() << std::endl;
        exit(1);
    }
}

