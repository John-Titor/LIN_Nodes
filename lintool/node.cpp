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

#include <err.h>
#include <unistd.h>
#include <iostream>

#include "node.h"
#include "link.h"
#include "upload.h"
#include "log.h"

Node::List  Node::_list;
static std::list<unsigned> knownAddresses = {
    Master::kNodeAddress,
    PowerV3::kNodeAddress + 0,
    PowerV3::kNodeAddress + 1,
    PowerV3::kNodeAddress + 2,
    PowerV3::kNodeAddress + 3,
    PowerV3::kNodeAddress + 4,
    PowerV3::kNodeAddress + 5,
    PowerV3::kNodeAddress + 6,
    PowerV3::kNodeAddress + 7,
    PowerV3::kNodeAddress + 8,
    PowerV3::kNodeAddress + 9,
    PowerV3::kNodeAddress + 10,
    PowerV3::kNodeAddress + 11,
    PowerV3::kNodeAddress + 12,
    PowerV3::kNodeAddress + 13,
    PowerV3::kNodeAddress + 14, // XXX get this from PowerV3::kNumAddresses or generate a full list from the NCF
    ECUBridge::kNodeAddress,
    Dashboard::kNodeAddress,
    Bootloader::kNodeAddress
};

Node::Node(unsigned address) :
    _params(address),
    _address(address)
{
    _list.push_back(this);
}

Node::~Node()
{
    _list.remove(this);
}

void
Node::scan(unsigned address)
{
    _list.clear();

    if (address == kNoNode) {

        for (auto addr : knownAddresses) {
            Node::_scan(addr);
        }

    } else {
        Node::_scan(address);
    }

    if (nodes().empty()) {
        RAISE(ExScanFailed, "no nodes found");
    }
}

void
Node::_scan(unsigned address)
{
    Link::set_node(address);

    try {
        Link::read_param(Generic::kParamProtocolVersion);
        new Node(address);

    } catch (Link::ExLINError &e) {
        // probably a timeout, ignore this
    } catch (Exception &e) {
        // fatal link error
        RAISE(ExScanFailed, "scan failed: " << e.what());

    } catch (...) {
        throw;
    }
}

bool
Node::exists(unsigned address)
{
    for (auto n : _list) {
        if (n->address() == address) {
            return true;
        }
    }

    return false;
}

Node *
Node::matching(unsigned address, unsigned function)
{
    for (auto n : _list) {
        if ((n->address() == address) && (n->function() == function)) {
            return n;
        }
    }

    return nullptr;
}

void
Node::update(bool verify, bool save_params)
{
    // get the firmware
    Firmware *fw;

    try {
        if (function() == board_function::kUnconfigured) {
            fw = Firmware::implied_firmware();

        } else {
            fw = Firmware::for_function(function());
        }

    } catch (Exception &e) {
        RAISE(ExUpdateFailed, e.what());
    }

    if (fw == nullptr) {
        std::cerr << "STATUS: "
                  << params().identity().str()
                  << "no firmware loaded, skipping"
                  << std::endl;

    } else {

        // make a copy of all the node's parameters for later use
        if (save_params) {
            params().sync();
        }

        std::cerr << "STATUS: "
                  << params().identity().str()
                  << " updating firmware"
                  << std::endl;

        // select the node
        Link::set_node(address());

        // drop it into bootloader mode
        enter_bootloader();

        // upload firmware to the selected node
        Upload::upload(fw, verify);

        // return to program mode and handle follow-up actions
        if (function() == board_function::kUnconfigured) {
            leave_bootloader(false);

        } else {
            leave_bootloader(true);

            // write parameters back if requested
            if (save_params) {
                std::cerr << "STATUS: "
                          << params().identity().str()
                          << "restoring configuration"
                          << std::endl;
                params().sync(true);
            }
        }
    }
}

void
Node::set_defaults()
{
    // check whether the node is already in the bootloader
    if (Link::read_param(Generic::kParamOperationMode) == operation_magic::kBootloader) {
        RAISE(ExDefaultFailed, "failed to request node reset");
    }

    try {
        // request reset to default parameters
        Link::write_param(Generic::kParamOperationMode, operation_magic::kSetDefaults);

        if (address() == Master::kNodeAddress) {
            // We just tried to reboot the master node, so it will need to be kicked off
            // the bus again. Tell the link that we have effectively lost master mode so that
            // it will re-claim the bus.
            //
            // Note that it's not necessary to re-enable master mode; it will happen automatically
            // next time we try to talk.
            //
            Link::enable_master(false);
            usleep(500000);             // 500ms seems to work OK
        }

    } catch (Exception &e) {
        RAISE(ExDefaultFailed, "trying to set defaults: " << e.what());
    }
}

void
Node::enter_bootloader()
{
    // check whether the node is already in the bootloader
    if (Link::read_param(Generic::kParamOperationMode) == operation_magic::kBootloader) {
        return;
    }

    try {
        // request entry to bootloader mode
        Link::write_param(Generic::kParamOperationMode, operation_magic::kEnterBootloader);

        if (address() == Master::kNodeAddress) {
            // We just tried to reboot the master node, and it might not have
            // taken, so go through the master setup process again just in case
            // we need to kick it off the bus once more. This will let us report
            // the error more accurately, rather than giving a cryptic link error
            // due to two masters trying to talk...
            //
            // Note that it's not necessary to re-enable master mode; it will happen automatically
            // when we try to read the bootloader mode parameter
            //
            Link::enable_master(false);
            usleep(500000);             // 500ms seems to work OK
        }

        // wait for the node to come back up in bootloader mode
        for (auto tries = 0; tries < 50; tries++) {

            try {
                auto state = Link::read_param(Generic::kParamOperationMode);

                if (state == operation_magic::kBootloader) {
                    return;
                }

                usleep(20000);

            } catch (Link::ExLINError &e) {
                // transfer failed; node is probably still rebooting
                continue;
            }
        }

        RAISE(ExUpdateFailed, "trying to enter bootloader mode");

    } catch (ExUpdateFailed &e) {
        throw;

    } catch (Exception &e) {
        RAISE(ExUpdateFailed, "trying to enter bootloader mode: " << e.what());
    }
}

void
Node::leave_bootloader(bool check)
{
    // check whether the node has already left the bootloader
    if (Link::read_param(Generic::kParamOperationMode) != operation_magic::kBootloader) {
        return;
    }

    try {
        // request return from bootloader mode
        Link::write_param(Generic::kParamOperationMode, operation_magic::kProgram);

        // if we aren't going to check that it came back, we're done here.
        if (!check) {
            return;
        }

        if (address() == Master::kNodeAddress) {
            // We just tried to reboot the master node, so go through the master
            // setup process again since we need to kick it off the bus once more.
            //
            // Note that it's not necessary to re-enable master mode; it will happen automatically
            // when we try to read the bootloader mode parameter
            //
            Link::enable_master(false);
            usleep(500000);             // 500ms seems to work OK
        }

        // wait for the node to come back up in program mode
        for (auto tries = 0; tries < 50; tries++) {

            try {
                auto state = Link::read_param(Generic::kParamOperationMode);

                if (state == operation_magic::kProgram) {
                    return;
                }

                usleep(20000);

            } catch (Link::ExLINError &e) {
                // transfer failed; node is probably still rebooting
                // don't print anything here as it's misleading...
                continue;
            }
        }

        RAISE(ExUpdateFailed, "trying to leave bootloader mode");

    } catch (ExUpdateFailed &e) {
        throw;

    } catch (Exception &e) {
        RAISE(ExUpdateFailed, "trying to leave bootloader mode: " << e.what());
    }
}
